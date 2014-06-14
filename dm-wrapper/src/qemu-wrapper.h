/*
 * Copyright (c) 2012 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef QEMU_WRAPPER_H_
# define QEMU_WRAPPER_H_

# include <unistd.h>
# include <libdmbus.h>

typedef enum
{
    SPAWN_QEMU_OLD,
    SPAWN_QEMU,
    SPAWN_DMBUS,
    SPAWN_INVALID,
} e_spawn_type;

typedef struct spawn_dm s_spawn_dm;
typedef struct service *dmbus_service_t;

typedef char * const *(*build_args_func)(const s_spawn_dm *dm,
                                         int argc, char * const *argv);
typedef int (*is_needed_func)(const s_spawn_dm *dm,
                              int argc, char * const *argv);

typedef enum dmbus_state
{
    DMBUS_CONNECT,
    DMBUS_RECONNECT,
    DMBUS_DISCONNECT,
} e_dmbus_state;

typedef void (*dmbus_state_func) (const s_spawn_dm *dm, e_dmbus_state state);

void dm_ready_dmbus (s_spawn_dm *dm, int reconnect);

typedef uint32_t cap_t;

struct spawn_dm
{
    const char* name;
    e_spawn_type type;
    is_needed_func is_needed;
    build_args_func build_args;
    dmbus_state_func dmbus_state;
    pid_t pid;
    int running;
    unsigned int dmid;
    int service;
    DeviceType devtype;
    dmbus_service_t serv;
    cap_t cap; /* Device model capabilities */
};

#define DM_CAP_NONE (0)
/* Emulate wired network */
#define DM_CAP_WIRED (1 << 0)
/* Emulate wifi network */
#define DM_CAP_WIFI (1 << 1)
/**
 * Be careful, we can't emulate only one network card outside QEMU old
 */
#define DM_CAP_NETWORK (DM_CAP_WIRED | DM_CAP_WIFI)

#define SPAWN_END_OF_LIST()                                 \
{ NULL, SPAWN_INVALID, NULL, NULL, NULL, -1, 0, 0, 0, 0, NULL, 0 }

#define SPAWN_QEMU_OLD(name, is_needed, build_args)                     \
{ name, SPAWN_QEMU_OLD, is_needed, build_args, NULL, -1, 0, 0, 0, 0, NULL, 0 }

#define SPAWN_QEMU(name, is_needed, build_args, cap)                        \
{ name, SPAWN_QEMU, is_needed, build_args, NULL, -1, 0, 0, 0, 0, NULL, cap }

#define SPAWN_DMBUS(name, is_needed, dmbus_state, service, devtype)         \
{ name, SPAWN_DMBUS, is_needed, NULL, dmbus_state, -1, 0, 0, service,       \
    devtype, NULL, 0 }

# define FOREACH_DMS(it)                                            \
    for ((it) = dms; (it) && (it)->type != SPAWN_INVALID; (it)++)


dmbus_service_t dmbus_service_connect (int service, DeviceType devtype,
                                      domid_t domid, s_spawn_dm *dm);
void dmbus_service_disconnect (dmbus_service_t service);
int dmbus_sync_recv (dmbus_service_t service, uint32_t type, void *data,
                     size_t size);
int dmbus_send (dmbus_service_t service, uint32_t msgtype, void *data,
                size_t len);

#endif /* !QEMU_WRAPPER_H_ */
