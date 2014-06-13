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
#ifndef LIBPCIEMU_INT_H_
# define LIBPCIEMU_INT_H_

# include "libpciemu.h"
# include "libpciemu_regs.h"
# include "list.h"

#define PCI_NUM_BAR 6
#define PCI_CONFIG_HEADER_SIZE 0x40
#define PCI_CONFIG_SIZE 0x100
#define PCI_BDF_FMT "%x:%x.%d"
#define PCI_BDF_ARGS(pci)                   \
    (pci)->info.bus, (pci)->info.device,    \
    (pci)->info.function

/* Describe a pci BAR */
typedef struct
{
   /* Function when we access to the BAR */
    libpciemu_io_ops_t ops;
    /* Is it a MMIO BAR (1) or an IO BAR (0) ? */
    int is_mmio;
    /* Indicate if we use or not the BAR */
    int enable;
    /* Address of the BAR in memory */
    uint64_t addr;
    /* Indicates is the BAR is mapped in memory */
    int mapped;
    /* Size of the BAR */
    uint64_t size;
    /* Type */
    uint8_t type;
} libpciemu_pci_bar_t;

/* Describe the pci ROM */
typedef struct
{
    /* Rom file */
    const char *romfile;
    /* Address we the rom is mapped in memory */
    uint8_t *addr;
    /* Size of the rom */
    uint32_t size;
    /* Indicate if we use or not the ROM */
    int enable;
} libpciemu_pci_rom_t;

/* IRQ level */
enum e_irqstate
{
    IRQ_LOW,
    IRQ_HIGH,
};

/* Describe a pci */
struct s_pci
{
    /* PCI info */
    libpciemu_pci_info_t info;
    /* BDF */
    uint16_t bdf;
    /* Config space */
    uint8_t config[PCI_CONFIG_SIZE];
    /* Write config space: indicate which bit you are allowed to modify */
    uint8_t cwrite[PCI_CONFIG_SIZE];
    /* FIXME: missing write 1 to clear implementation */
    /* BARs and ROM */
    libpciemu_pci_bar_t bars[PCI_NUM_BAR];
    libpciemu_pci_rom_t rom;
    /* IO handle : information about the device model */
    libpciemu_handle_t iohdl;
    /* Private data */
    void *priv;
    /* Link to the next pci */
    LIST_ENTRY(struct s_pci) link;
    /* Interrupt information */
    uint32_t irq_pin;
    enum e_irqstate irq_state;
};

# endif /* !LIBPCIEMU_INT_H_ */
