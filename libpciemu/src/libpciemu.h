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
#ifndef LIBPCIEMU_H_
# define LIBPCIEMU_H_

# include <stdint.h>
# include <libpciemu_regs.h>

/**
 *  The library is unable to handle 64 bits address for the moment
 */

/* Log level */
typedef enum
{
    LIBPCIEMU_MESSAGE_FATAL = 0,
    LIBPCIEMU_MESSAGE_ERROR = 3,
    LIBPCIEMU_MESSAGE_WARNING = 4,
    LIBPCIEMU_MESSAGE_INFO = 6,
    LIBPCIEMU_MESSAGE_DEBUG = 7,
} libpciemu_loglvl;

/* Logging callback */
typedef void (*libpciemu_logging_t) (libpciemu_loglvl lvl,
                                     const char *fmt, ...)
                                     __attribute__ ((format (printf, 2, 3)));

/**
 * Initialize the library when the application start
 * logging: callback to retrieve libpciemu message
 * return 0 if success and non-zero value if failed.
 */
int libpciemu_init (libpciemu_logging_t logging);

/* Cleanup libpciemu. Must be called at the end of the application */
void libpciemu_cleanup (void);

/* I/O domain handle */
typedef struct iohandle *libpciemu_handle_t;

/**
 * Read/Write I/O region callback
 * addr: absolute except for BAR callback the address is relative to the start
 * size: size of the access
 */
typedef uint64_t (*libpciemu_read_t) (uint64_t addr, uint32_t size,
                                      void *priv);
typedef void (*libpciemu_write_t) (uint64_t addr, uint64_t data,
                                   uint32_t size, void *priv);

typedef struct
{
    libpciemu_read_t read;
    libpciemu_write_t write;
} libpciemu_io_ops_t;

/**
 * For each new domain, an io handle need to be create. It will be used
 * to handle I/O and dispatch to the right pci device.
 */
libpciemu_handle_t libpciemu_handle_create (int domid);

/* Cleanup the iohandle when the domain died */
void libpciemu_handle_destroy (libpciemu_handle_t iohdl);

/**
 * Retrieve the file descriptor. The application needs to wait the I/O on
 * this fd.
 */
int libpciemu_handle_get_fd (libpciemu_handle_t iohdl);

/**
 * When the file descriptor become readable, the application needs to
 * call this function to dispatch the I/O
 */
int libpciemu_handle (libpciemu_handle_t iohdl);

/**
 * TODO :
 *  - capabilities
 *  - rom
 *  - status
 */

/* Bar update callback */
typedef void (*libpciemu_bar_update_t) (uint8_t id, uint64_t bar, void *priv);

/* This structure contains main information to create a device pci */
typedef struct
{
    /* BDF : bus (8 bits) device (5 bits) function (3 bits) */
    uint16_t domain;
    uint8_t bus;
    uint8_t device : 5;
    uint8_t function : 3;
    /* Vendor and device ID */
    uint16_t vendor_id;
    uint16_t device_id;
    /* SubVendor and subDevice ID */
    uint16_t subvendor_id;
    uint16_t subdevice_id;
    /* Revision ID */
    uint8_t revision;
    /* Class Code divide in 2 field */
    uint16_t class;
    uint8_t prog_if; /* Program interface */
    /* Header */
    uint8_t header;
    /* Command */
    uint32_t command;
    /**
     * Config space handler
     * if the config_read or config_write is NULL then we use default
     * config space callback. If you implement your own config space
     * callback, you need to call the default callbacks.
     */
    libpciemu_io_ops_t config;
    /* Bar update callback */
    libpciemu_bar_update_t bar_update;
} libpciemu_pci_info_t;

/* Opaque type for pci device */
typedef struct s_pci *libpciemu_pci_t;

/* Initialize a pci device */
libpciemu_pci_t libpciemu_pci_device_init (const libpciemu_pci_info_t *info,
                                           void *priv);

/**
 * Release a pci device
 * Use this function to release memory if libpciemu_pci_device_register failed.
 * Otherwise, the pci device will be release when iohandle will be
 * destroy.
 */
void libpciemu_pci_device_release (libpciemu_pci_t pci);

/* Register inside Xen the pci device. It returns 0 on success */
int libpciemu_pci_device_register (libpciemu_handle_t iohdl,
                                   libpciemu_pci_t pci);

/**
 * Register a BAR
 * MMIO BAR size must be align on 16-byte
 * IO BAR size must be align on 4-byte
 * /!\ callbacks use a relative address from the start of the BAR
 * it's to avoid to keep BAR address
 */

#define PCI_BAR_TYPE_PREFETCH PCI_BASE_ADDRESS_MEM_PREFETCH
#define PCI_BAR_TYPE_1M PCI_BASE_ADDRESS_MEM_TYPE_1M
#define PCI_BAR_UNMAPPED (~(uint32_t)0)

int libpciemu_pci_register_bar (libpciemu_pci_t pci, uint32_t id, int is_mmio,
                                uint8_t type, uint64_t size,
                                const libpciemu_io_ops_t *ops);

/* Register a ROM */
int libpciemu_pci_register_rom (libpciemu_pci_t pci, const char *romfile);


/* Default config read/write function */
uint64_t libpciemu_pci_default_config_read (uint64_t addr, uint32_t size,
                                            libpciemu_pci_t pci);
void libpciemu_pci_default_config_write (uint64_t addr, uint64_t data,
                                         uint32_t size, libpciemu_pci_t pci);

/* Set an interrupt */
int libpciemu_pci_interrupt_set (libpciemu_pci_t pci, uint8_t level);

/**
 *  Bunch of functions to easily use the config space
 */

/**
 * Set/Get data to the config space
 * /!\ don't use this function for BAR and ROM, instead
 * use libpciemu_pci_register_rom and libpciemu_pci_register_bar
 */
void libpciemu_pci_config_set_byte (libpciemu_pci_t pci, uint32_t offset,
                                    uint8_t byte);
void libpciemu_pci_config_set_word (libpciemu_pci_t pci, uint32_t offset,
                                    uint16_t word);
void libpciemu_pci_config_set_dword (libpciemu_pci_t pci, uint32_t offset,
                                     uint32_t dword);

/* Read data from config space */
uint8_t libpciemu_pci_config_get_byte (libpciemu_pci_t pci, uint32_t offset);
uint16_t libpciemu_pci_config_get_word (libpciemu_pci_t pci, uint32_t offset);
uint32_t libpciemu_pci_config_get_dword (libpciemu_pci_t pci, uint32_t offset);

/* Set/Get status */
void libpciemu_pci_set_status (libpciemu_pci_t pci, uint16_t status);
uint16_t libpciemu_pci_get_status (libpciemu_pci_t pci);

/* Set/Get command */
void libpciemu_pci_set_command (libpciemu_pci_t pci, uint16_t command);
uint16_t libpciemu_pci_get_command (libpciemu_pci_t pci);

/* Set interrupt pin */
int libpciemu_pci_set_interrupt_pin (libpciemu_pci_t pci, uint8_t pin);

/* Check if interruption is disabled */
int libpciemu_pci_interrupt_disabled (libpciemu_pci_t pci);

/* Add/remove I/O range to trap for a specific domain */
void libpciemu_handle_remove_iorange (libpciemu_handle_t iohdl, uint64_t addr,
                                      int mmio);
void libpciemu_handle_add_iorange (libpciemu_handle_t iohdl, uint64_t addr,
                                   uint64_t size, int mmio,
                                   const libpciemu_io_ops_t *ops,
                                   void *priv);

/* FIXME: handle capabilities */

# endif /* !PCI_H_ */
