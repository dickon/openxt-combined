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

/*
Fundamentally we are responsible for spotting new USB devices being plugged in to the system,
consulting the various policies and handing those devices to the appropriate VMs. We also maintain
the list of "always" assignments the user has specified and provide all the information required by
the USB part of the UIVM GUI.

To do this we maintain information regarding:
* what devices are currently plugged in to the system
* what VMs are currently running
* what USB buses are on the system
* what "always" assignemnts have been specified

To prevent the system from being hosed if this daemon crashes or is killed, this information is all
stored elsewhere and merely cached here.

When started we:
1. Query /dev to find what USB buses and devices are on the system.
2. Query XenStore to find all running VMs.
3. Query each of those VMs to determine which devices they are using.

When a new VM starts it informs us and we then go through our list of unassigned devices and assign
any appropriate to the new VM.

When a VM ejects a device it informs us and we add it to our unused list.

When a new device is connected we assign it to the VM on screen, if that is appropriate. Otherwise
we either give the device to dom0 or leave it unassigned, depending on the type of device.

Whenever we spot a new VM or device we update our list of running VMs and purge devices from any
that disappeared.

*/

#include "project.h"

static struct event usb_event; 
                
static void  wrapper_udevMonHandler(int fd , short event , void *opaque)
{
    udevMonHandler();
}    
                
/* Setup and perform directory & dbus watching
 */
static int watch(void)
{
    int r;
    int udev_fd;
    fd_set read_set;
    int nfds = 0;
    struct timeval timeout;

    udev_fd = initSys();
    if(udev_fd < 0)
        return udev_fd;

    updateVMs(-1);
    queryExistingDevices();
    checkAllOptical();
    checkAllKeyboard();
#if 0
    purgeAccidentalBinds();
#endif
    remote_report_all_devs_change();

    event_set(&usb_event,udev_fd,EV_READ | EV_PERSIST, wrapper_udevMonHandler, (void*)1);
    event_add(&usb_event,NULL);

    LogInfo("Dispatching events (Event library v%s .  Method %s)",event_get_version() , event_get_method() );

    event_dispatch();
  
    /* won't get here */
    return 2;
}                                          


int main(int argc, char **argv)
{
    if(argc != 1)
    {
        LogError("Usage: %s", argv[0]);
        return 1;
    }

    /* Initialise stuff */
    event_init();
    xenstore_init();
    rpc_init();
    alwaysInit();
    initFakeNames();

    return watch();
}                                           
