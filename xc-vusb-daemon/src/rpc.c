/*
 * Copyright (c) 2014 Citrix Systems, Inc.
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

#include "project.h"
#include "rpcgen/xenmgr_vm_client.h"
#include "rpcgen/xenmgr_client.h"
#include "owls.h"

#define SERVICE "com.citrix.xenclient.usbdaemon"
#define SERVICE_OBJ_PATH "/"

static xcdbus_conn_t   *g_xcbus = NULL;
static DBusConnection  *g_dbus_conn = NULL;
static DBusGConnection *g_glib_dbus_conn = NULL;

/*********************************************/
/** CTXUSB_DAEMON dbus object implementation */
/******************vvvvvvvvv******************/
#include "rpcgen/ctxusb_daemon_server_obj.h"

gboolean is_usb_enabled(int domid)
{
    char *obj_path = NULL;
    gboolean v;
    if (!com_citrix_xenclient_xenmgr_find_vm_by_domid_ ( g_xcbus, "com.citrix.xenclient.xenmgr", "/", domid, &obj_path )) {
        return 0;
    }
    if (!property_get_com_citrix_xenclient_xenmgr_vm_usb_enabled_ ( g_xcbus, "com.citrix.xenclient.xenmgr", obj_path, &v )) {
        return 0;
    }
    return v;
}

gboolean has_device_grab(int domid)
{
    char *obj_path = NULL;
    gboolean v;
    if (!com_citrix_xenclient_xenmgr_find_vm_by_domid_ ( g_xcbus, "com.citrix.xenclient.xenmgr", "/", domid, &obj_path )) {
        return 0;
    }
    if (!property_get_com_citrix_xenclient_xenmgr_vm_usb_grab_devices_ ( g_xcbus, "com.citrix.xenclient.xenmgr", obj_path, &v )) {
        return 0;
    }
    return v;
}

NOPROTO gboolean ctxusb_daemon_set_policy_domuuid(
    CtxusbDaemonObject *this,
    const char *uuid,
    const char *policy, GError **error)
{
    if (storePolicy(uuid, policy) != 0) {
        /* Couldn't store policy */
        LogInfo("Could not store USB policy for domain %s", uuid);
    }
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_get_policy_domuuid(
    CtxusbDaemonObject *this,
    const char *uuid,
    char **value, GError **error)
{
    const char *policy = fetchPolicy(uuid);
    if (policy == NULL)  /* No policy found */
        policy = "";
    *value = g_strdup(policy);
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_new_vm(CtxusbDaemonObject *this,
                              gint IN_dom_id, GError **error)
{
    updateVMs(IN_dom_id);
    processNewVMs();
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_vm_stopped(CtxusbDaemonObject *this,
                                  gint IN_dom_id, GError **error)
{
    VMstopping(IN_dom_id);
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_list_devices(CtxusbDaemonObject *this,
                                    GArray* *OUT_devices, GError **error)
{
    static IBuffer devs = { NULL, 0, 0 };
    GArray *devArray;
    int i;

    assert(OUT_devices != NULL);

    getDeviceList(&devs);
    devArray = g_array_new(FALSE, FALSE, sizeof(gint));
    
    LogInfo("List devices:");
    for(i = 0; i < devs.len; ++i)
    {
        LogInfo("    %d", devs.data[i]);
        g_array_append_val(devArray, devs.data[i]);
    }

    *OUT_devices = devArray;
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_get_device_info(CtxusbDaemonObject *this,
                                       gint IN_dev_id, const char* IN_vm_uuid,
                                       char* *OUT_name, gint *OUT_state, char* *OUT_vm_assigned, char* *OUT_detail, GError **error)
{
    static Buffer nameBuf = { NULL, 0 };
    static Buffer fullNameBuf = { NULL, 0 };
    static Buffer assignedBuf = { NULL, 0 };
    int state;

    clearBuffer(&nameBuf);
    clearBuffer(&assignedBuf);

    state = getDeviceInfo(IN_dev_id, IN_vm_uuid, &nameBuf, &fullNameBuf, &assignedBuf);

    LogInfo("Device %d info: name=\"%s\" state=%d info=\"%s\" assoc=\"%s\" PoV=\"%s\"",
            IN_dev_id, nameBuf.data, state, fullNameBuf.data, assignedBuf.data, IN_vm_uuid);

    *OUT_state = state;
    *OUT_name = g_strdup(nameBuf.data);
    *OUT_vm_assigned = g_strdup(assignedBuf.data);
    *OUT_detail = g_strdup(fullNameBuf.data);

    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_assign_device(CtxusbDaemonObject *this,
                                     gint IN_dev_id, const char* IN_vm_uuid, GError **error)
{
    assignToExistingVM(IN_dev_id, IN_vm_uuid);
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_unassign_device(CtxusbDaemonObject *this,
                                       gint IN_dev_id, GError **error)
{
    unassignFromVM(IN_dev_id);
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_set_sticky(CtxusbDaemonObject *this,
                                  gint IN_dev_id, gint IN_sticky, GError **error)
{
    setSticky(IN_dev_id, IN_sticky);
    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_state(CtxusbDaemonObject *this,
                             char **OUT_state, GError **error)
{
    static Buffer stateDump = { NULL, 0 };

    setBuffer(&stateDump, "ctxusb-daemon state:\n");
    dumpVMs(&stateDump);
    dumpDevice(&stateDump);
    alwaysDump(&stateDump);
    dumpPolicy(&stateDump);
    dumpFakeNames(&stateDump);

    *OUT_state = g_strdup(stateDump.data);

    return TRUE;
}

NOPROTO gboolean ctxusb_daemon_name_device(CtxusbDaemonObject *this,
                                   gint IN_dev_id, const char* IN_name, GError **error)
{
    setDeviceName(IN_dev_id, IN_name);
    return TRUE;
}


/***************^^^^^^^^^^^^^^^********************/
/** CTXUSB_DAEMON dbus object implementation ENDS */
/**************************************************/

xcdbus_conn_t *rpc_xcbus()
{
    return g_xcbus;
}

/** does RPC call to ctxusb domain component */
int remote_plug_device(int domid, int bus_num, int dev_num)
{
    if (owls_domain_attach(bus_num, dev_num, domid)) {
        return -1;
    }
    if (owls_vusb_assign(bus_num, dev_num)) {
        owls_domain_detach(bus_num, dev_num, domid);
        return -1;
    }

    return 0;
}

/** does RPC call to ctxusb domain component */
int remote_unplug_device(int domid, int bus_num, int dev_num)
{
    int rv = 0;
    rv |= owls_vusb_unassign(bus_num, dev_num);
    rv |= owls_domain_detach(bus_num, dev_num, domid);
    return rv ? -1:0;
}

/** does RPC call to ctxusb domain component */
int remote_query_devices(int domid, IBuffer *device_ids)
{
    int devices[256];
    int numdevs;
    int i;
    int dev;

    assert(device_ids != NULL);

    numdevs = owls_list_domain_devices(domid, devices, sizeof(devices)/sizeof(int));
    IBufferSize(device_ids, numdevs);
    
    for(i = 0; i < numdevs; ++i)
    {
        dev = devices[i];
        if(dev >= 0)
            appendIBuffer(device_ids, dev);
    }

    return TRUE;
}

void remote_report_rejected(const char *dev_name, const char *cause)
{
    assert(g_glib_dbus_conn != NULL);
    notify_com_citrix_xenclient_usbdaemon_device_rejected(g_xcbus,
                                                          "com.citrix.xenclient.usbdaemon", 
                                                          "/", (char *)dev_name, (char *)cause);
}

void remote_report_all_devs_change(void)
{
    assert(g_glib_dbus_conn != NULL);
    notify_com_citrix_xenclient_usbdaemon_devices_changed(g_xcbus,
                                                              "com.citrix.xenclient.usbdaemon", 
                                                              "/");
}

void remote_report_dev_change(int dev_id)
{
    assert(g_glib_dbus_conn != NULL);
    notify_com_citrix_xenclient_usbdaemon_device_info_changed(g_xcbus,
                                                              "com.citrix.xenclient.usbdaemon", 
                                                              "/", dev_id);
}


void rpc_init()
{
    CtxusbDaemonObject *server_obj = NULL;
    /* have to initialise glib type system */
    g_type_init();

    g_glib_dbus_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
    if (!g_glib_dbus_conn) {
        LogError("no bus");
        exit(1);
    }
    g_dbus_conn = dbus_g_connection_get_connection(g_glib_dbus_conn);
    g_xcbus = xcdbus_init_event(SERVICE, g_glib_dbus_conn);
    if (!g_xcbus) {
        LogError("failed to init dbus connection / grab service name");
        exit(1);
    }
    /* export server object */
    server_obj = ctxusb_daemon_export_dbus(g_glib_dbus_conn, SERVICE_OBJ_PATH);
    if (!server_obj) {
        LogError("failed to export server object");
        exit(1);
    }
}
