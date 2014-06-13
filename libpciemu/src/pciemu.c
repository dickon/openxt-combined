/*
 * Copyright (c) 2013 Citrix Systems, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "io.h"
#include "libpciemu_int.h"
#include "logging.h"

#define PCI_WARN(pci, fmt, a...)                                    \
    warning (PCI_BDF_FMT": "fmt, PCI_BDF_ARGS(pci), ## a)
#define PCI_DEBUG(pci, fmt, a...)                                   \
    debug (PCI_BDF_FMT": "fmt, PCI_BDF_ARGS(pci), ## a)

EXTERNAL libpciemu_pci_t
libpciemu_pci_device_init (const libpciemu_pci_info_t *info, void *priv)
{
    libpciemu_pci_t pci = NULL;

    if (!(pci = calloc (1, sizeof (*pci))))
        return NULL;

    memcpy (&pci->info, info, sizeof (*info));

    pci->priv = priv;

    /* Initialize config space */
    libpciemu_pci_config_set_word (pci, PCI_VENDOR_ID, info->vendor_id);
    libpciemu_pci_config_set_word (pci, PCI_DEVICE_ID, info->device_id);
    libpciemu_pci_config_set_byte (pci, PCI_REVISION_ID, info->revision);
    libpciemu_pci_config_set_byte (pci, PCI_CLASS_PROG, info->prog_if);
    libpciemu_pci_config_set_word (pci, PCI_CLASS_DEVICE, info->class);
    libpciemu_pci_config_set_word (pci, PCI_HEADER_TYPE, info->header);
    libpciemu_pci_set_command (pci, info->command);
    libpciemu_pci_config_set_word (pci, PCI_SUBSYSTEM_VENDOR_ID, info->subvendor_id);
    libpciemu_pci_config_set_word (pci, PCI_SUBSYSTEM_ID, info->subdevice_id);

    /* Initialize cwrite */
    pci->cwrite[PCI_CACHE_LINE_SIZE] = 0xff;
    pci->cwrite[PCI_INTERRUPT_LINE] = 0xff;
    *((uint16_t *)&pci->cwrite[PCI_COMMAND]) = PCI_COMMAND_IO
        | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER
        | PCI_COMMAND_INTX_DISABLE;

    memset (pci->cwrite + PCI_CONFIG_HEADER_SIZE, 0xff,
            PCI_CONFIG_SIZE - PCI_CONFIG_HEADER_SIZE);

    PCI_DEBUG (pci, "device init");

    pci->bdf |= ((uint16_t)pci->info.bus) << 8;
    pci->bdf |= ((uint16_t)pci->info.device) << 3;
    pci->bdf |= ((uint16_t)pci->info.function);

    pci->irq_state = IRQ_LOW;

    return pci;
}

EXTERNAL void
libpciemu_pci_device_release (libpciemu_pci_t pci)
{
    free (pci);
}

EXTERNAL int
libpciemu_pci_register_bar (libpciemu_pci_t pci, uint32_t id,
                            int is_mmio, uint8_t type,
                            uint64_t size, const libpciemu_io_ops_t *ops)
{
    libpciemu_pci_bar_t *bar = NULL;
    uint32_t val = 0;

    if (id >= PCI_NUM_BAR)
        return 1;

    bar = &pci->bars[id];

    if (bar->enable)
    {
        PCI_WARN (pci, "bar %u is already registered", id);
        return 1;
    }

    // read and write are mandatory
    if (!ops || !ops->write || !ops->read)
    {
        PCI_WARN (pci, "bar %u: read and/or write are not implemented", id);
        return 1;
    }

    memcpy (&bar->ops, ops, sizeof (bar->ops));
    bar->is_mmio = is_mmio;
    bar->size = size;

    if (is_mmio)
    {
        type &= PCI_BASE_ADDRESS_MEM_TYPE_MASK | PCI_BASE_ADDRESS_MEM_PREFETCH;
        val = type | PCI_BASE_ADDRESS_SPACE_MEMORY;
    }
    else
        val = PCI_BASE_ADDRESS_SPACE_IO;

    *((uint32_t *)&pci->cwrite[PCI_BASE_ADDRESS_0 + id * 4]) = ~(size - 1);

    libpciemu_pci_config_set_dword (pci, PCI_BASE_ADDRESS_0 + id * 4, val);

    bar->type = type;
    bar->enable = 1;
    bar->addr = PCI_BAR_UNMAPPED;

    PCI_DEBUG (pci, "register bar %u", id);

    return 0;
}

EXTERNAL uint64_t
libpciemu_pci_default_config_read (uint64_t addr, uint32_t size,
                                   libpciemu_pci_t pci)
{
    uint64_t res;

    if (addr >= PCI_CONFIG_SIZE)
    {
        PCI_WARN (pci, "Invalid config space read addr = %#"PRIx64" size = %u",
                  addr, size);
        return (~(uint64_t)0);
    }

    if ((addr + size) > PCI_CONFIG_SIZE)
        size = PCI_CONFIG_SIZE - addr;

    memcpy (&res, &pci->config[addr], size);

    return res;
}

/* Retrieve BAR */
static const libpciemu_pci_bar_t *
libpciemu_pci_bar_get (const libpciemu_pci_t pci, uint64_t addr, int is_mmio)
{
    uint32_t i;
    const libpciemu_pci_bar_t *bar = NULL;

    for (i = 0; i < PCI_NUM_BAR; i++)
    {
        bar = &pci->bars[i];

        if (!bar->enable || bar->is_mmio != is_mmio)
            continue;

        if (bar->addr <= addr && addr < (bar->addr + bar->size))
            return bar;
    }

    return NULL;
}

static uint64_t
libpciemu_pci_bar_read (libpciemu_pci_t pci, uint64_t addr,
                        uint32_t size, int is_mmio)
{
    const libpciemu_pci_bar_t *bar;

    bar = libpciemu_pci_bar_get (pci, addr, is_mmio);

    if (!bar)
    {
        PCI_WARN (pci, "Unable to find addr = 0x%"PRIx64" is_mmio = %d",
                  addr, is_mmio);
    }

    addr -= bar->addr;

    return bar->ops.read (addr, size, pci->priv);
}

static void
libpciemu_pci_bar_write (libpciemu_pci_t pci, uint64_t addr,
                         uint64_t val,  uint32_t size, int is_mmio)
{
    const libpciemu_pci_bar_t *bar = libpciemu_pci_bar_get (pci, addr, is_mmio);

    if (!bar)
    {
        PCI_WARN (pci, "Unable to find addr = 0x%"PRIx64" is_mmio = %d",
                  addr, is_mmio);
    }

    addr -= bar->addr;

    bar->ops.write (addr, val, size, pci->priv);
}

#define PCI_BAR_OPS(prefix, is_mmio)                            \
static void                                                     \
libpciemu_pci_##is_mmio##_write (uint64_t addr, uint64_t data,  \
                                 uint32_t size, void *priv)     \
{                                                               \
    libpciemu_pci_bar_write (priv, addr, data, size, is_mmio);  \
}                                                               \
                                                                \
static uint64_t                                                 \
libpciemu_pci_##is_mmio##_read (uint64_t addr, uint32_t size,   \
                                void *priv)                     \
{                                                               \
    return libpciemu_pci_bar_read (priv, addr, size, is_mmio);  \
}                                                               \
                                                                \
static const libpciemu_io_ops_t pci_##prefix##_ops = {          \
    .read = libpciemu_pci_##is_mmio##_read,                     \
    .write = libpciemu_pci_##is_mmio##_write,                   \
};

PCI_BAR_OPS(pio, 0)
PCI_BAR_OPS(mmio, 1)

/* Update BAR */
static void
libpciemu_pci_update_bar (libpciemu_pci_t pci, uint32_t id)
{
    libpciemu_pci_bar_t *bar = &pci->bars[id];
    uint32_t addr = libpciemu_pci_config_get_dword (pci, PCI_BASE_ADDRESS_0 + id * 4);
    uint16_t cmd = libpciemu_pci_get_command (pci);
    uint32_t mask = ~(bar->size - 1);

    if (!bar->enable)
        return;

    if (bar->is_mmio)
        addr &= PCI_BASE_ADDRESS_MEM_MASK;
    else
        addr &= PCI_BASE_ADDRESS_IO_MASK;

    if ((!(cmd & PCI_COMMAND_IO) && !bar->is_mmio)
        || (!(cmd & PCI_COMMAND_MEMORY) && bar->is_mmio))
      addr = PCI_BAR_UNMAPPED;

    if (!addr || (addr == mask))
        addr = PCI_BAR_UNMAPPED;

    if (bar->addr == addr)
        return;

    if (addr == PCI_BAR_UNMAPPED)
    {
        PCI_DEBUG (pci, "bar %u unmapped", id);
        if (bar->mapped)
            libpciemu_handle_remove_iorange (pci->iohdl, bar->addr,
                                             bar->is_mmio);
        bar->mapped = 0;
    }
    else
    {
        PCI_DEBUG (pci, "bar %u mapped in 0x%x", id, addr);
        if (bar->mapped)
            libpciemu_handle_remove_iorange (pci->iohdl, bar->addr,
                                             bar->is_mmio);
        libpciemu_handle_add_iorange (pci->iohdl, addr, bar->size,
                                      bar->is_mmio,
                                      (bar->is_mmio) ? &pci_mmio_ops : &pci_pio_ops,
                                      pci);
        bar->mapped = 1;
    }

    if (pci->info.bar_update)
        pci->info.bar_update (id, addr, pci->priv);

    /* FIX: add callback update bar */
    bar->addr = addr;
}

/* Update pci config */
static void
libpciemu_pci_update_config (libpciemu_pci_t pci)
{
    uint32_t i = 0;

    /* Update BARs */
    for (i = 0; i < PCI_NUM_BAR; i++)
        libpciemu_pci_update_bar (pci, i);
}

EXTERNAL void
libpciemu_pci_default_config_write (uint64_t addr, uint64_t data,
                                    uint32_t size, libpciemu_pci_t pci)
{
    uint32_t i;
    uint8_t mask;

    if (addr >= PCI_CONFIG_SIZE)
    {
        PCI_WARN (pci, "Invalid config space write addr = %#"PRIx64" data = %#"
                  PRIx64" size = %u", addr, data, size);
        return;
    }

    if ((addr + size) > PCI_CONFIG_SIZE)
        size = PCI_CONFIG_SIZE - addr;

    for (i = 0; i < size; i++)
    {
        /* Mask is used to know which bit we are allowed to write */
        mask = pci->cwrite[addr + i];
        pci->config[addr + i] &= ~mask;
        pci->config[addr + i] |= ((data >> (i * 8)) & 0xff) & mask;
    }

    libpciemu_pci_update_config (pci);
}

EXTERNAL void
libpciemu_pci_config_set_byte (libpciemu_pci_t pci, uint32_t offset,
                               uint8_t byte)
{
    if (offset >= PCI_CONFIG_SIZE)
       return;

    pci->config[offset] = byte;
}

EXTERNAL void
libpciemu_pci_config_set_word (libpciemu_pci_t pci, uint32_t offset,
                               uint16_t word)
{
    libpciemu_pci_config_set_byte (pci, offset, word & 0xff);
    libpciemu_pci_config_set_byte (pci, offset + 1, word >> 8);
}

EXTERNAL void
libpciemu_pci_config_set_dword (libpciemu_pci_t pci, uint32_t offset,
                                uint32_t dword)
{
    libpciemu_pci_config_set_word (pci, offset, dword & 0xffff);
    libpciemu_pci_config_set_word (pci, offset + 2, dword >> 16);
}

EXTERNAL uint8_t
libpciemu_pci_config_get_byte (libpciemu_pci_t pci, uint32_t offset)
{
    if (offset >= PCI_CONFIG_SIZE)
        return 0xff;

    return pci->config[offset];
}

EXTERNAL uint16_t
libpciemu_pci_config_get_word (libpciemu_pci_t pci, uint32_t offset)
{
    uint16_t res = 0;

    res = libpciemu_pci_config_get_byte (pci, offset + 1);
    res = (res << 8) | libpciemu_pci_config_get_byte (pci, offset);

    return res;
}

EXTERNAL uint32_t
libpciemu_pci_config_get_dword (libpciemu_pci_t pci, uint32_t offset)
{
    uint32_t res = 0;

    res = libpciemu_pci_config_get_word (pci, offset + 2);
    res = (res << 16) | libpciemu_pci_config_get_word (pci, offset);

    return res;
}

EXTERNAL uint16_t
libpciemu_pci_get_status (libpciemu_pci_t pci)
{
    return libpciemu_pci_config_get_dword (pci, PCI_STATUS);
}

EXTERNAL void
libpciemu_pci_set_command (libpciemu_pci_t pci, uint16_t command)
{
    libpciemu_pci_config_set_word (pci, PCI_COMMAND, command);
}

EXTERNAL uint16_t
libpciemu_pci_get_command (libpciemu_pci_t pci)
{
    return libpciemu_pci_config_get_word (pci, PCI_COMMAND);
}

EXTERNAL int
libpciemu_pci_set_interrupt_pin (libpciemu_pci_t pci, uint8_t pin)
{
    if (pin <= 0 || pin > 4)
    {
        PCI_WARN (pci, "Invalid interrupt pin %u", pin);
        return 1;
    }

    libpciemu_pci_config_set_byte (pci, PCI_INTERRUPT_PIN, pin);
    pci->irq_pin = pin;

    return 0;
}

EXTERNAL int
libpciemu_pci_interrupt_disabled (libpciemu_pci_t pci)
{
    return libpciemu_pci_get_command (pci) & PCI_COMMAND_INTX_DISABLE;
}

EXTERNAL int
libpciemu_pci_interrupt_set (libpciemu_pci_t pci, uint8_t level)
{
    int ret = 0;
    enum e_irqstate state = (level) ? IRQ_HIGH : IRQ_LOW;

    /* The IRQ level has not changed, don't notify Xen */
    if (pci->irq_state == state)
        return 0;

    ret = xc_hvm_set_pci_intx_level (xch, pci->iohdl->domid, pci->info.domain,
                                     pci->info.bus, pci->info.device,
                                     pci->info.function, level);
    if (!ret)
        pci->irq_state = state;

    return ret;
}

EXTERNAL int
libpciemu_pci_device_register (libpciemu_handle_t iohdl, libpciemu_pci_t pci)
{
    int rc = 0;

    if (!iohdl)
        return -EBADF;

     rc = xc_hvm_register_pcidev (xch, iohdl->domid, iohdl->serverid,
                                  pci->info.domain, pci->info.bus,
                                  pci->info.device, pci->info.function);

    if (rc)
        return rc;

    pci->iohdl = iohdl;

    pthread_mutex_lock (&iohdl->pci_lock);

    LIST_INSERT_HEAD (&iohdl->pci_list, pci, link);

    pthread_mutex_unlock (&iohdl->pci_lock);

    return 0;
}
