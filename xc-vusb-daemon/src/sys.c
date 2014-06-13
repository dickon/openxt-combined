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

/* USB policy daemon
 * Extract information about devices from /sys and /proc
 *
 * We identify USB devices by their system names, which is the last part of the path of the node in
 * /sys which represents the device. For example, for a device with node:
 *   /sys/bus/usb/devices/2-1.1
 * the device system name is
 *   2-1.1
 *
 * For USB interfaces we use the same mechanism, but these names will include a colon, for example:
 *   2-1.1:1.0
 *
 * In both cases this name is the string that must be written to /sys control files to bind / unbind
 * devices to / from drivers.
 */

#include "project.h"


static struct udev *udev;              /* Udev device used to create monitor */
static struct udev_monitor *udev_mon;  /* Udev monitor */


/* Special filesystem paths */
#define USB_SYS_ROOT_DIR     "/sys/bus/usb/devices/"
#define USB_SYS_UNBIND_POST  "/driver/unbind"
#define USB_SYS_USBHID_BIND_FILE "/sys/bus/usb/drivers/usbhid/bind"
#define USB_SYS_PM_POST      "/power/level"
#define USB_SYS_CTRL_FILE    "/sys/bus/usb/drivers_autoprobe"
#define USB_SYS_PROBE_FILE   "/sys/bus/usb/drivers_probe"
#define SCSI_HOSTS_FILE      "/proc/scsi/scsi"
#define USB_HID_ROOT_DIR     "/dev/"


/* File names within a device's /sys directory */
#define SYS_FILE_READY        "/uevent"
#define SYS_FILE_DESCRIPTORS  "/descriptors"
#define SYS_FILE_VENDOR       "/manufacturer"
#define SYS_FILE_PRODUCT      "/product"
#define SYS_FILE_SERIAL       "/serial"
#define SYS_FILE_BUS          "/busnum"
#define SYS_FILE_DEV          "/devnum"
#define SYS_FILE_VID          "/idVendor"
#define SYS_FILE_PID          "/idProduct"
#define SYS_FILE_BINDING      "/driver"
#define SYS_FILE_ICOUNT       "/bNumInterfaces"
#define SYS_FILE_ICLASS       "/bInterfaceClass"


/* File names within an interface's /sys directory */
#define SYS_FILE_SCSI         "host"


/* System file magic formats */
#define SCSI_HOSTS_HOST       "Host: scsi%d "
#define SCSI_HOSTS_TYPE       "  Type: CD-ROM%c"



/**** File handling utilities ****/

/* Read an integer from the specified file in the given directory
 * param    dir                  full path of directory containing file to read from
 * param    file                 file to read integer from
 * param    value                location to store read integer
 * param    base                 base to read value in (10 or 16)
 * return                        0 on success, -EPERM on failure
 */
static int readFileInt(const char *dir, const char *file, int *value, int base)
{
    static Buffer pathBuffer = { NULL, 0 };
    FILE *fp;

    assert(dir != NULL);
    assert(file != NULL);
    assert(value != NULL);
    assert(base == 10 || base == 16);

    setBuffer(&pathBuffer, dir);
    catToBuffer(&pathBuffer, file);

    fp = fopen(pathBuffer.data, "r");
    if(fp == NULL)
        return -EPERM;

    if(fscanf(fp, (base == 10) ? "%d" : "%x", value) <= 0)
    {
        LogError("Could not read %s", pathBuffer.data);
        fclose(fp);
        return -EPERM;
    }

    fclose(fp);
    return 0;
}


/* Cat a string from the specified file in the given directory to the specified buffer
 * param    dir                  full path of directory containing file to read from
 * param    file                 file to read string from
 * param    buffer               buffer to cat to
 * return                        0 on success, -EPERM on failure
 */
static int catFileString(const char *dir, const char *file, Buffer *buffer)
{
    static Buffer pathBuffer = { NULL, 0 };
    FILE *fp;
    int c;

    assert(dir != NULL);
    assert(file != NULL);
    assert(buffer != NULL);

    setBuffer(&pathBuffer, dir);
    catToBuffer(&pathBuffer, file);

    fp = fopen(pathBuffer.data, "r");
    if(fp == NULL)
    {
        //        LogError("Could not open %s", pathBuffer.data);
        return -EPERM;
    }

    while((c = getc(fp)) != EOF)
    {
        char t[2] = {c, '\0'};
        catToBuffer(buffer, t);
    }

    fclose(fp);
    return 0;
}



/**** Functions to handle dom0 driver binding ****/

/* Bind all the single given device or interface to dom0 drivers
 * param    sysName              device or interface's system name
 */
void bindToDom0(const char *sysName)
{
    FILE *file;

    assert(sysName != NULL);

    file = fopen(USB_SYS_PROBE_FILE, "w");

    if(file == NULL)
    {
        LogError("Could not open %s (%d)", USB_SYS_PROBE_FILE, errno);
        return;
    }

    LogInfo("Binding %s to dom0", sysName);
    fprintf(file, "%s", sysName);
    fclose(file);
}

/* Attempt to bind the single given device or interface to usbhid drivers
 * param    sysName              device or interface's system name
 */
int bindToUsbhid(const char *bus_id)
{
    int fd;
    int l;
    LogInfo("Attempting to bind to hidraw %s", bus_id);
    assert(bus_id != NULL);

    l=strlen(bus_id);

    if(!l)
        return -1;

    fd = open(USB_SYS_USBHID_BIND_FILE, O_WRONLY);

    if(fd<0)
        return -1;

    if(write(fd,bus_id,l)!=l)
    {
        close(fd);
        return -1;
    }

    LogInfo("Bind to hidraw worked");

    close(fd);
    return 0;
}

void unbindAllFromDom0ExceptRootDevice(const char *sysName)
{
    static Buffer unbindPath = { NULL, 0 };  /* Path to unbind control file */
    FILE *unbindFile;      /* Handle to unbind control file */
    DIR *dir;              /* Handle to devices directory */
    struct dirent *iface;  /* Handle to each interface */
    int nameLen;           /* Length of given sysName */

    assert(sysName != NULL);

    /* If we just unbind the device without doing the interfaces first external drives take
     * around a minute to unbind, hence we unbind the interfaces first
     */
    
    /* Open devices directory */
    dir = opendir(USB_SYS_ROOT_DIR);

    if(dir == NULL)
    {
        LogError("Could not open " USB_SYS_ROOT_DIR " (%d)", errno);
    }
    else
    {
        nameLen = strlen(sysName);
        /* Look at each listed USB interface, eg /sys/bus/usb/devices/3-2:1.0/ */
        while((iface = readdir(dir)) != NULL)
        {
            if(strncmp(iface->d_name, sysName, nameLen) == 0 && iface->d_name[nameLen] == ':')
            {
                /* This is an interface of the given device, unbind it */
                setBuffer(&unbindPath, USB_SYS_ROOT_DIR);
                catToBuffer(&unbindPath, iface->d_name);
                catToBuffer(&unbindPath, USB_SYS_UNBIND_POST);

                unbindFile = fopen(unbindPath.data, "w");

                if(unbindFile == NULL)
                {
                    LogError("Could not open %s (%d)", unbindPath.data, errno);
                }
                else
                {
                    LogInfo("Unbinding %s from dom0", iface->d_name);
                    fprintf(unbindFile, "%s", iface->d_name);
                    fclose(unbindFile);
                }
            }
        }
    }

    closedir(dir);
}


/* Unbind all interfaces of the given device from dom0
 * param    sysName              device's system name
 */
void unbindAllFromDom0(const char *sysName)
{
    static Buffer unbindPath = { NULL, 0 };  /* Path to unbind control file */
    FILE *unbindFile;      /* Handle to unbind control file */

    assert(sysName != NULL);

    unbindAllFromDom0ExceptRootDevice(sysName);

    /* Now unbind the device */
    setBuffer(&unbindPath, USB_SYS_ROOT_DIR);
    catToBuffer(&unbindPath, sysName);
    catToBuffer(&unbindPath, USB_SYS_UNBIND_POST);

    unbindFile = fopen(unbindPath.data, "w");

    if(unbindFile == NULL)
    {
        LogError("Could not open %s (%d)", unbindPath.data, errno);
        return;
    }

    LogInfo("Unbinding %s from dom0", sysName);
    fprintf(unbindFile, "%s", sysName);
    fclose(unbindFile);
}



/* Unbind all interfaces of the given device from dom0
 * param    sysName              device's system name
 */
void bindAllToDom0(const char *sysName)
{
    static Buffer unbindPath = { NULL, 0 };  /* Path to unbind control file */
    FILE *unbindFile;      /* Handle to unbind control file */
    DIR *dir;              /* Handle to devices directory */
    struct dirent *iface;  /* Handle to each interface */
    int nameLen;           /* Length of given sysName */

    assert(sysName != NULL);

    bindToDom0(sysName);
    
    /* Open devices directory */
    dir = opendir(USB_SYS_ROOT_DIR);

    if(dir == NULL)
    {
        LogError("Could not open " USB_SYS_ROOT_DIR " (%d)", errno);
    }
    else
    {
        nameLen = strlen(sysName);
        /* Look at each listed USB interface, eg /sys/bus/usb/devices/3-2:1.0/ */
        while((iface = readdir(dir)) != NULL)
        {
            if(strncmp(iface->d_name, sysName, nameLen) == 0 && iface->d_name[nameLen] == ':')
            {
    		bindToDom0(iface->d_name);
            }
        }
    }

    closedir(dir);

}



/**** Functions to handle optical determination ****/

/* Determine whether the specified SCSI host number is an optical drive
 * param    host                 SCSI host number
 * param    sysDev               containing device's system name (for logging)
 * return                        0 if host is an optical drive, non-0 otherwise
 */
int isHostOptical(int host, const char *sysDev)
{
    FILE *file;   /* SCSI hosts file */
    int host_id;  /* id of each host found in file */
    int r;        /* scanf return value */
    char c;       /* trailing character */

    assert(sysDev != NULL);

    /* Open devices directory */
    file = fopen(SCSI_HOSTS_FILE, "r");

    if(file == NULL)
    {
        LogError("Could not open " SCSI_HOSTS_FILE " (%d)", errno);
        return 1;
    }

    while(1)
    {
        /* Look for the relevant Host line */
        r = fscanf(file, SCSI_HOSTS_HOST, &host_id);

        if(r == EOF)
        {
            /* End of file reached, host not found */
            LogInfo("Couldn't find SCSI host %d", host);
            fclose(file);
            return 1;
        }

        if(r == 1 && host_id == host)  /* Host found */
            break;

        dumpLine(file);
    }

    /* Dump the rest of the host line and the next line */
    dumpLine(file);
    dumpLine(file);

    r = fscanf(file, SCSI_HOSTS_TYPE, &c);
    fclose(file);

    if(r == 1 && c == ' ')
    {
        /* Host is an optical drive */
        LogInfo("SCSI host %d (device %s) is an optical drive", host, sysDev);
        return 0;
    }

    /* Host isn't an optical drive */
    LogInfo("SCSI host %d (device %s) is not an optical drive", host, sysDev);
    return 1;
}


/* Determine whether the dom0 bound device interface is an optical drive
 * param    sysPath              interface's full system path
 * param    sysDev               containing device's system name (for logging)
 * return                        0 if interface is an optical drive, non-0 otherwise
 */
static int isIfaceOptical(const char *sysPath, const char *sysDev)
{
    DIR *dir;             /* Handle to interface's directory */
    struct dirent *file;  /* Handle to each file */

    assert(sysPath != NULL);
    assert(sysDev != NULL);
    
    /* Open devices directory */
    dir = opendir(sysPath);

    if(dir == NULL)
    {
        LogError("Could not open %s (%d)", sysPath, errno);
        return 1;
    }
    
    /* Look at each listed file in the interface directory */
    while((file = readdir(dir)) != NULL)
    {
        if(strncmp(file->d_name, SYS_FILE_SCSI, strlen(SYS_FILE_SCSI)) == 0)
        {
            /* This is a SCSI host link */
            if(isHostOptical(atoi(file->d_name + strlen(SYS_FILE_SCSI)), sysDev) == 0)
            {
                /* We're optical */
                closedir(dir);
                return 0;
            }

            /* Host is not optical */
            break;
        }
    }

    closedir(dir);

    /* No interfaces optical */
    return 1;
}


/* Determine whether the dom0 bound device with the given sysName is an optical drive
 * param    sysName              device's system name
 * return                        0 if device is an optical drive, non-0 otherwise
 */
int isDeviceOptical(const char *sysName)
{
    static Buffer ifacePath = { NULL, 0 };  /* Path of each interface */
    DIR *dir;              /* Handle to device's directory */
    struct dirent *iface;  /* Handle to each interface */
    int nameLen;           /* Length of given sysName */

    assert(sysName != NULL);
    
    /* Open devices directory */
    dir = opendir(USB_SYS_ROOT_DIR);

    if(dir == NULL)
    {
        LogError("Could not open " USB_SYS_ROOT_DIR " (%d)", errno);
        return 1;
    }
    
    nameLen = strlen(sysName);
    /* Look at each listed USB interface, eg /sys/bus/usb/devices/3-2:1.0/ */
    while((iface = readdir(dir)) != NULL)
    {
        if(strncmp(iface->d_name, sysName, nameLen) == 0 && iface->d_name[nameLen] == ':')
        {
            /* This is an interface of the given device, check for scsi host */
            setBuffer(&ifacePath, USB_SYS_ROOT_DIR);
            catToBuffer(&ifacePath, iface->d_name);

            if(isIfaceOptical(ifacePath.data, sysName) == 0)
            {
                /* We're optical */
                closedir(dir);
                return 0;
            }

            /* Interface is not optical */
        }
    }

    closedir(dir);

    /* No interfaces optical */
    return 1;
}



/**** Functions to process HiD reports ****/

/* HiD report item prefix bit specifiers */
#define HID_TYPE_MASK          0x0C  /* 0000nn00 Type bits */
#define HID_SIZE_MASK          0x03  /* 000000nn Size bits */
#define HID_TAG_TYPE_MASK      0xFC  /* nnnnnn00 Both the tag and type bits*/

#define HID_TAG_LONG           0xFE  /* 11111110 Value specifying long format item */
#define HID_TAG_USAGE_PAGE     0x04  /* 000001nn Tag & type value specifying usage page */
#define HID_TAG_USAGE          0x08  /* 000010nn Tag & type value specifying usage */
#define HID_TYPE_MAIN          0x00  /* nnnn00nn Type tag for a "main" item */

#define HID_USAGE_KEYBOARD     0x07  /* Usage page indicating keyboard */
#define HID_USAGE_MIN          0x18  /* 000110nn Tag value specifying the min usage */
#define HID_USAGE_MAX          0x28  /* 001010nn Tag value specifying the max usage */
#define KB_LEFT_CTRL           0xE0  /* Tag value specifying left CTRL usage ID */
#define KB_RIGHT_CTRL          0xE4  /* Tag value specifying right CTRL usage ID */

static int HiD_data_size[4] = {0, 1, 2, 4};  /* Length value to bytes conversion table */


/* Determine whether the data in the given HiD report contains a keyboard
 * param    report               HiD report to check
 * param    name                 report name, for error reporting
 * return                        0 if report contains a keyboard, 1 for no keyboard, <0 on error
 */
static int checkReportKeyboard(struct hidraw_report_descriptor *report, const char *name)
{
    /* Useful reading: Device Class Definition for HID v1.11 */
    int i, j;    /* Byte counters */
    int prefix;  /* Prefix byte for each item */
    int size;    /* Item data size (in bytes) */

    bool kb_usage_page = false;       /* If a keyboard usage page has been found (global) */
    bool kb_usage = false;            /* If a keyboard usage has been found (local) */
    int hid_usage_min = 0xFF;         /* Usage minimum (local) */
    int hid_usage_max = 0x00;         /* Usage maximum (local) */

    assert(report != NULL);
    assert(name != NULL);

    /* Run through report checking item tags */
    for(i = 0; i < report->size; ++i)
    {
        prefix = report->value[i];
        size = HiD_data_size[prefix & HID_SIZE_MASK];

        if((i + size) > report->size)
        {
            LogInfo("Error in HiD report %s at index %d, insufficient data for item size %d", name, i, size);
            return -EINVAL;
        }

        if(prefix == HID_TAG_LONG)
        {
            /* Long format item */
            if(size != 2)
            {
                LogInfo("Error in HiD report %s at index %d, invalid long item length", name, i);
                return -EINVAL;
            }

            i += report->value[i + 1];
        }
        else
        {
            /* Short format item */
            if((prefix & HID_TYPE_MASK) == HID_TYPE_MAIN)
            {
                if(kb_usage || kb_usage_page)
                    if(((hid_usage_min <= KB_LEFT_CTRL)  && (KB_LEFT_CTRL <= hid_usage_max)) || \
                       ((hid_usage_min <= KB_RIGHT_CTRL) && (KB_RIGHT_CTRL <= hid_usage_max)))
                    {
                        LogInfo("Secure keyboard found");
                        return 0;
                    }
                /* Reset all local items as now out of scope */
                kb_usage = false;
                hid_usage_min = 0xFF;
                hid_usage_max = 0x00;
            }
            else
            {
               int data = 0;  /* Data from the usage page */

               for(j = size; j > 0; --j)
                   data = (data << 8) | report->value[i + j];

               if((prefix & HID_TAG_TYPE_MASK) == HID_TAG_USAGE_PAGE)
                   kb_usage_page = (data == HID_USAGE_KEYBOARD);

               if((prefix & HID_TAG_TYPE_MASK) == HID_TAG_USAGE)
                   kb_usage = (data == HID_USAGE_KEYBOARD);

               if((prefix & HID_TAG_TYPE_MASK) == HID_USAGE_MIN)
                   hid_usage_min = data;

               if((prefix & HID_TAG_TYPE_MASK) == HID_USAGE_MAX)
                   hid_usage_max = data;
            }
        }
        i += size;
    }

    if(i != report->size)
    {
        LogInfo("Error in HiD report %s, data length mismatch (%d != %d)", name, i, report->size);
        return -EINVAL;
    }

    /* No keyboard found */
    return 1;
}


/* Determine whether the specified hidraw device represents a keyboard
 * param    hidName              name of hidraw dev node to check
 * param    name                 name for error reporting
 * return                        0 if device contains a keyboard, 1 for no keyboard, <0 on error
 */
int checkHidrawKeyboard(const char *hidName, const char *name)
{
    static Buffer hidPath = { NULL, 0 };     /* Path of HiD node to check */
    struct hidraw_report_descriptor report;  /* HiD report */
    int r;     /* Return values */
    int fd;    /* File descriptor for ioctls */
    int size;  /* Size of report descriptor */

    assert(hidName != NULL);
    assert(name != NULL);

    setBuffer(&hidPath, USB_HID_ROOT_DIR);
    catToBuffer(&hidPath, hidName);

    fd = open(hidPath.data, O_RDWR);
    if(fd < 0)
    {
        LogInfo("Could not open %s (%d)", hidPath.data, errno);
        return -EINVAL;
    }

    /* Get report descriptor size */
	r = ioctl(fd, HIDIOCGRDESCSIZE, &size);
    if(r < 0)
    {
        LogInfo("Could not get report descriptor size for %s (%d)", hidPath.data, r);
        close(fd);
        return -EINVAL;
    }
    
    /* Get report descriptor */
    report.size = size;
    r = ioctl(fd, HIDIOCGRDESC, &report);
    if(r < 0)
    {
        LogInfo("Could not get report descriptor for %s (%d)", hidPath.data, r);
        close(fd);
        return -EINVAL;
    }

    r = checkReportKeyboard(&report, name);
    close(fd);
    return r;
}


/* Determine whether the specified HiD device is a keyboard
 * parm    sysName               sysName of device to check
 * return                        0 if device contains a keyboard, 1 for no keyboard, <0 on error
 */
int checkHidKeyboard(const char *sysName)
{
    static Buffer path = { NULL, 0 };  /* Path of each subdirectory opened */
    DIR *devDir;             /* Handle to devices directory */
    struct dirent *devFile;  /* Handle to each interface found in devices directory */
    int nameLen;             /* Length of given sysName */
    int ifaceCount;          /* Number of matching interfaces found */

    assert(sysName != NULL);

    /* We need to find all directories with paths of the form:
     * /sys/bus/usb/devices/<sysName>:<interface>/<hidbus>:<vid>:<pid>.????/hidraw/hidraw<number>
     * eg:
     * /sys/bus/usb/devices/2-1.1.1:1.0/0003:413C:3200.0003/hidraw/hidraw0
     * It is the trailing integer we actually need.
     */

    /* Open devices directory */
    devDir = opendir(USB_SYS_ROOT_DIR);

    if(devDir == NULL)
    {
        LogError("Could not open " USB_SYS_ROOT_DIR " (%d)", errno);
        return -EINVAL;
    }
    
    nameLen = strlen(sysName);
    ifaceCount = 0;

    /* Look at each listed USB interface, eg /sys/bus/usb/devices/3-2:1.0/ */
    while((devFile = readdir(devDir)) != NULL)
    {
        if(strncmp(devFile->d_name, sysName, nameLen) == 0 && devFile->d_name[nameLen] == ':')
        {
            DIR *ifaceDir;             /* Handle to interface's directory */
            struct dirent *ifaceFile;  /* Handle to each file in interface's directory */

            /* This is an interface of the given device, check for hidraw link */
            ++ifaceCount;
            setBuffer(&path, USB_SYS_ROOT_DIR);
            catToBuffer(&path, devFile->d_name);

            /* Open interface's directory */
            ifaceDir = opendir(path.data);

            if(ifaceDir == NULL)
            {
                LogError("Could not open %s (%d)", path.data, errno);
                continue;
            }
    
            /* Look at each file in interface's directory */
            while((ifaceFile = readdir(ifaceDir)) != NULL)
            {
                if(strlen(ifaceFile->d_name) == 19 && ifaceFile->d_name[4] == ':')
                {
                    DIR *hidDir;             /* Handle to hidraw directory */
                    struct dirent *hidFile;  /* Handle to each file in hidraw directory */

                    /* We've got to, eg:
                     * /sys/bus/usb/devices/2-1.1.1:1.0/0003:413C:3200.0003
                     */
                    setBuffer(&path, USB_SYS_ROOT_DIR);
                    catToBuffer(&path, devFile->d_name);
                    catToBuffer(&path, "/");
                    catToBuffer(&path, ifaceFile->d_name);
                    catToBuffer(&path, "/hidraw/");

                    /* Open hidraw directory */
                    hidDir = opendir(path.data);

                    if(hidDir == NULL)
                    {
                        LogError("Could not open %s (%d)", path.data, errno);
                        continue;
                    }

                    /* Look for hidraw<number> directories */
                    while((hidFile = readdir(hidDir)) != NULL)
                    {
                        if(strncmp(hidFile->d_name, "hidraw", strlen("hidraw")) == 0)
                        {
                            int r;  /* Report checking return value */

                            /* We have a match, check the report */
                            r = checkHidrawKeyboard(hidFile->d_name, devFile->d_name);

                            if(r <= 0)
                            {
                                /* Error, or we've found a keyboard */
                                closedir(hidDir);
                                closedir(ifaceDir);
                                closedir(devDir);
                                return r;
                            }
                        }
                    }

                    closedir(hidDir);
                }
            }

            closedir(ifaceDir);
        }
    }

    closedir(devDir);

    if(ifaceCount == 0)
    {
        LogInfo("Device %s doesn't yet have any interfaces", sysName);
        return -ENOENT;
    }

    /* No keyboards found */
    return 1;
}



/**** Functions using libusb to control devices ****/

/* Find the libusb node representing the device with the given bus and device numbers
 * param    busNum               number of bus target device is on
 * param    devNum               number of targe device within bus
 * return                        libusb node representing specified device, NULL for none
 */
static struct usb_device *findLibusbDev(int busNum, int devNum)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    usb_find_busses();
    usb_find_devices();

    for(bus = usb_get_busses(); bus != NULL; bus = bus->next)
        for(dev = bus->devices; dev != NULL; dev = dev->next)
            if(atoi(bus->dirname) == busNum && atoi(dev->filename) == devNum)  /* Match */
                return dev;

    /* Device not found */
    return NULL;
}


/* Reset the specified device
 * param    sysName              sys name of device to reset
 * param    busNum               number of bus target device is on
 * param    devNum               number of targe device within bus
 */
void resetDevice(const char * sysName, int busNum, int devNum)
{
    struct usb_device *dev;  /* libusb device */
    usb_dev_handle *handle;  /* handle to libusb device */

    LogInfo("entering in resetDevice for %s", sysName);
    assert(sysName != NULL);

    unbindAllFromDom0(sysName);

    dev = findLibusbDev(busNum, devNum);

    if(dev != NULL)
    {
        /* Device found */
        LogInfo("Resetting %s", sysName);
        handle = usb_open(dev);
    	if (handle != NULL)
    	{
    	  usb_reset(handle);
    	  usb_close(handle);
    	} else {
    	  LogInfo("usb_open failed for %s", sysName);
    	}
    }

    bindToDom0(sysName);
}

/* Resume the specified device
 * param    sysName              sys name of device to resume
 */
void resumeDevice(const char * sysName)
{
    static Buffer pmPath = { NULL, 0 };  /* Path to pm control file */
    FILE *pmFile;

    /* automatic suspend/resume of USB devices from ctxusb_daemon is
       creating some problems mostly because is difficult to test
       properly all kind of classes we want to suspend
       automatically. For now disable this feature completely */
    return;

    assert(sysName != NULL);

    setBuffer(&pmPath, USB_SYS_ROOT_DIR);
    catToBuffer(&pmPath, sysName);
    catToBuffer(&pmPath, USB_SYS_PM_POST);

    pmFile = fopen(pmPath.data, "w");
    
    if(pmFile == NULL)
      {
	LogError("Could not open %s (%d)", pmPath.data, errno);
      }
    else
      {
	LogInfo("Resuming %s ", sysName);
	fprintf(pmFile, "on");
	fclose(pmFile);
      }
}

/* Suspend the specified device
 * param    sysName              sys name of device to suspend
 */
void suspendDevice(const char * sysName)
{
    static Buffer pmPath = { NULL, 0 };  /* Path to pm control file */
    FILE *pmFile;
    struct stat statBuf;

    /* automatic suspend/resume of USB devices from ctxusb_daemon is
       creating some problems mostly because is difficult to test
       properly all kind of classes we want to suspend
       automatically. For now disable this feature completely */
    return;

    assert(sysName != NULL);

    if (!stat("/config/dont-use-usb-suspend",&statBuf)) {
      LogInfo("/config/dont-use-usb-suspend file is preventing suspend\n");
      return;
    }
    setBuffer(&pmPath, USB_SYS_ROOT_DIR);
    catToBuffer(&pmPath, sysName);
    catToBuffer(&pmPath, USB_SYS_PM_POST);

    pmFile = fopen(pmPath.data, "w");
    
    if(pmFile == NULL)
      {
	LogError("Could not open %s (%d)", pmPath.data, errno);
      }
    else
      {
	LogInfo("Suspending %s ", sysName);
	fprintf(pmFile, "suspend");
	fclose(pmFile);
      }
}



/**** Functions to extract information from USB devices ****/

/* Determine whether the specified interface is HiD
 * param    sysName              sys name of interface to test
 * return                        0 if interface is HiD, >0 if not, <0 on error
 */
int isInterfaceHid(const char *sysName)
{
    static Buffer sysPath = { NULL, 0 };  /* Interface sys path */
    int class;

    assert(sysName != NULL);

    setBuffer(&sysPath, USB_SYS_ROOT_DIR);
    catToBuffer(&sysPath, sysName);

    if(readFileInt(sysPath.data, SYS_FILE_ICLASS, &class, 16) < 0)
    {
        LogError("Could not read information from %s", sysPath.data);
        return -EPERM;
    }

    return (class == USB_CLASS_HID) ? 0 : 1;
}


/* Determine the number of interfaces the specified device has
 * param    sysName              sys name of device to check
 * return                        number of interfaces in device, <0 on error
 */
int getDeviceInterfaceCount(const char *sysName)
{
    static Buffer sysPath = { NULL, 0 };  /* Devicce sys path */
    int count;

    assert(sysName != NULL);

    setBuffer(&sysPath, USB_SYS_ROOT_DIR);
    catToBuffer(&sysPath, sysName);

    if(readFileInt(sysPath.data, SYS_FILE_ICOUNT, &count, 10) < 0)
    {
        LogError("Could not read interface count from %s", sysPath.data);
        return -1;
    }

    return count;
}


/* Read a USB descriptor into the given buffer
 * param    fd                   file descriptor to read USB info from
 * param    path                 full path of device (for error reporting)
 * param    buffer               buffer to read descriptor into
 * return                        type of descriptor read, <0 for no more descriptors
 */
static int readUSBdescriptor(int fd, const char *path, Buffer *buffer)
{
    int r;   /* read return values */
    int l;   /* length of descriptor */
    int dt;  /* descriptor type */

    assert(path != NULL);
    assert(buffer != NULL);

    bufferSize(buffer, 2);
    r = read(fd, buffer->data, 2);

    if(r == 0)  /* no more descriptors */
        return -1;

    if(r < 2)
    {
        LogError("Error reading %s (r=%d errno %d)", path, r, errno);
        return -2;
    }

    /* Get the rest of this descriptor */
    l = buffer->data[0];
    dt = buffer->data[1];
    bufferSize(buffer, l);
    buffer->data[0] = l;
    buffer->data[1] = dt;
    r = read(fd, buffer->data + 2, l - 2);

    if(r < (l - 2))
    {
        LogError("Error reading %s (r=%d errno %d)", path, r, errno);
        return -2;
    }

    return dt;
}


/* Read class information for the given device
 * param    sysName              system name of device to process
 * param    classes              buffer to store list of classes in
 * return                        0 on success,
 *                               -EPERM on failure
 */
static int getClasses(const char *sysName, IBuffer *classes)
{
    static Buffer infoPath = { NULL, 0 };    /* path of file to get device information from */
    static Buffer descBuffer = { NULL, 0 };  /* each descriptor */
    int fd;  /* file descriptor to get data from */
    int dt;  /* descriptor type */
    int j;   /* class counters */
    int class;  /* each class found, <0 for none */

    assert(sysName != NULL);
    assert(classes != NULL);

    setBuffer(&infoPath, USB_SYS_ROOT_DIR);
    catToBuffer(&infoPath, sysName);
    catToBuffer(&infoPath, SYS_FILE_DESCRIPTORS);

    fd = open(infoPath.data, O_RDONLY);
    if(fd < 0)
        return -EPERM;

    clearIBuffer(classes);

    /* Find descriptors */
    while((dt = readUSBdescriptor(fd, infoPath.data, &descBuffer)) >= 0)
    {
        if(dt == 1)
        {
            /* We've found a device descriptor */
            struct usb_device_descriptor *d = (struct usb_device_descriptor *)descBuffer.data;
            class = ((d->bDeviceClass & 0xFF) << 8) | (d->bDeviceSubClass & 0xFF);
            
            if(class == 0)  /* Ignore device class of 0, just use interface class */
                class = -1;
        }
        else if(dt == 4)
        {
            /* We've found an interface descriptor */
            struct usb_interface_descriptor *i = (struct usb_interface_descriptor *)descBuffer.data;
            class = ((i->bInterfaceClass & 0xFF) << 8) | (i->bInterfaceSubClass & 0xFF);
        }
        else
            continue;

        /* Check for duplicates */
        for(j = 0; j < classes->len; ++j)
        {
            if(classes->data[j] == class)
            {
                /* Duplicate, eliminate */
                class = -1;
                break;
            }
        }

        if(class >= 0)
            appendIBuffer(classes, class);
    }

    close(fd);
    return 0;
}


/* Read a device definition and add it to our list
 * param    sysName              system name of device to process
 * return                        device id on success, -EPERM for ignore
 */
static int readDevice(const char *sysName)
{
    static IBuffer classes = { NULL, 0, 0 };  /* Classes defined by device */
    static Buffer serial = { NULL, 0 };       /* Serial number */
    static Buffer product = { NULL, 0 };      /* Product string */
    static Buffer vendor = { NULL, 0 };       /* Vendor string */
    static Buffer tableProduct = { NULL, 0 }; /* Product string from system definition file */
    static Buffer tableVendor = { NULL, 0 };  /* Vendor string from system definition file */
    static Buffer name = { NULL, 0 };         /* Friendly name */
    static Buffer fullName = { NULL, 0 };     /* Full name */
    static Buffer sysPath = { NULL, 0 };      /* Path of device's /sys directory */
    FILE *fp;       /* Descriptor file */
    int Vid;        /* Vendor id */
    int Pid;        /* Product id */
    int busNum;     /* Number of bus device is on */
    int devNum;     /* Number of device within bus */
    int r;          /* Return value from getClass */

    assert(sysName != NULL);

    /* Get advertised classes */
    r = getClasses(sysName, &classes);

    if(r == -EPERM)  /* Failed to read classes, ignore device */
        return -EPERM;

    setBuffer(&sysPath, USB_SYS_ROOT_DIR);
    catToBuffer(&sysPath, sysName);

    if(readFileInt(sysPath.data, SYS_FILE_VID, &Vid, 16) < 0 ||
       readFileInt(sysPath.data, SYS_FILE_PID, &Pid, 16) < 0 ||
       readFileInt(sysPath.data, SYS_FILE_BUS, &busNum, 10) < 0 ||
       readFileInt(sysPath.data, SYS_FILE_DEV, &devNum, 10) < 0)
    {
        LogError("Could not read information from %s", sysPath.data);
        return -EPERM;
    }

    /* Read identifying strings */
    setBuffer(&vendor, "");
    catFileString(sysPath.data, SYS_FILE_VENDOR, &vendor);
    sanitiseName(&vendor);

    setBuffer(&product, "");
    catFileString(sysPath.data, SYS_FILE_PRODUCT, &product);
    sanitiseName(&product);

    setBuffer(&serial, "");
    catFileString(sysPath.data, SYS_FILE_SERIAL, &serial);
    /* Do not sanitise serial number, we do matching on it */

    /* Get names from system definition file */
    findTableName(Vid, Pid, &tableVendor, &tableProduct);

    /* Construct full name */
    setBuffer(&fullName, vendor.data);

    if(product.data[0] != '\0')
    {
        if(fullName.data[0] != '\0')
            catToBuffer(&fullName, " ");

        catToBuffer(&fullName, "%s", product.data);
    }

    if(tableVendor.data[0] != '\0')
    {
        /* Note you can't have a table product name without a table vendor name */
        if(fullName.data[0] != '\0')
            catToBuffer(&fullName, " / ");

        catToBuffer(&fullName, "%s", tableVendor.data);

        if(tableProduct.data[0] != '\0')
            catToBuffer(&fullName, " %s", tableProduct.data);
    }

    /* Construct friendly name */
    if(product.data[0] != '\0')
    {
        /* Product name provided */
        setBuffer(&name, product.data);
    }
    else if(tableProduct.data[0] != '\0')
    {
        /* Product name given in system definition file, use that */
        setBuffer(&name, tableProduct.data);
    }
    else if(vendor.data[0] != '\0')
    {
        /* Product name not provided, but vendor name is, use that */
        setBuffer(&name, vendor.data);
        catToBuffer(&name, " device");
    }
    else if(tableVendor.data[0] != '\0')
    {
        /* Product name not provided, but vendor name is given in system definition file, use that */
        setBuffer(&name, tableVendor.data);
        catToBuffer(&name, " device");
    }
    else
    {
        /* No product or vendor name provided anywhere */
        setBuffer(&name, "Unknown device");
        setBuffer(&fullName, name.data);
    }

    catToBuffer(&fullName, " [%04X:%04X]", Vid, Pid);

    getFakeName(Vid, Pid, serial.data, &name);

    /* Exclude Intel(R) Centrino(R) Advanced-N + WiMAX 6250 [XC-6623] */
    if (Vid == 0x8086 && Pid == 0x0188) {
      LogInfo("Excluding WiMAX 6250\n");
      return;
    }

    return addDevice(Vid, Pid, serial.data, busNum, devNum, name.data, fullName.data, sysName, &classes);
}



/**** Functions to handle watching for new devices ****/

/* Handle a udev event */
void udevMonHandler(void)
{
    static Buffer device = { NULL, 0 };  /* Containing device */
    struct udev_device *dev;
    const char *action;
    const char *type;
    const char *path;
    const char *subsystem;
    const char *endPath;  /* Part of path after last / */
    const char *p;        /* Misc path part pointer */

    dev = udev_monitor_receive_device(udev_mon);
    assert(dev != NULL);

    action = udev_device_get_action(dev);
    type = udev_device_get_devtype(dev);
    path = udev_device_get_devpath(dev);
    subsystem = udev_device_get_subsystem(dev);

    for(endPath = path + strlen(path); *endPath != '/' && endPath > path; --endPath);  /* Find last / */
    ++endPath;  /* Points to string after last / */

    //    LogInfo("udevmon action=%s type=%s subsystem=%s path=%s", action, type, subsystem, path);

    if(action != NULL && strcmp(action, "add") == 0)
    {
        if(type != NULL && strcmp(type, "usb_device") == 0)
        {
            /* New device spotted 
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3
             *
             * We need the device sys name (2-1.1.3)
             */
            int id;  /* id of new device */

            LogInfo("Spotted new device %s", endPath);
            id = readDevice(endPath);

            if(id >= 0)
                assignNewDevice(id);
        }
        else if(type != NULL && strcmp(type, "usb_interface") == 0)
        {
            /* New interface spotted
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3/2-1.1.3:1.0
             *
             * We need the device sys name (2-1.1.3) and interface sys name (2-1.1.3:1.0)
             */
            LogInfo("Spotted new interface %s", endPath);
            setBuffer(&device, endPath);
            trimEndBuffer(&device, ':');
            processNewInterface(device.data, endPath);
        }
        else if(type != NULL && strcmp(type, "scsi_target") == 0)
        {   /* New SCSI target spotted
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3/2-1.1.3:1.0/host43/target43:0:0
             *
             * We need the device sys name (2-1.1.3) and the host number (43)
             */
            int host;

            LogInfo("Spotted new scsi target %s", endPath);
            setBuffer(&device, endPath);
            trimEndBuffer(&device, ':');
            host = atoi(device.data + strlen("target"));

            /* endPath points to character after last slash.
             * Back up 2 slashes before that to find start of interface (2-1.1.3:1.0).
             */
            p = endPath - 1;           /* p points to last slash */
            for(--p; *p != '/'; --p);  /* p points to slash before "host" */
            for(--p; *p != '/'; --p);  /* p points to slash before interface sys name */
            ++p;                       /* p points to interface sys name */
            
            setBuffer(&device, p);
            trimEndBuffer(&device, ':');
            checkDeviceOptical(device.data, host);
        }
        else if(type == NULL && subsystem != NULL && strcmp(subsystem, "hidraw") == 0)
        {
            /* New hidraw node spotted
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3/2-1.1.3:1.0/0003:413C:2003.0013/hidraw/hidraw1
             *
             * We need the device sys name (2-1.1.3) and hidraw dev name (hidraw1)
             */

            /* endPath points to character after last slash.
             * Back up 2 slashes before that to find start of interface (2-1.1.3:1.0).
             */
            LogInfo("Spotted new hidraw %s", endPath);
            p = endPath - 1;           /* p points to last slash */
            for(--p; *p != '/'; --p);  /* p points to slash before first "hidraw" */
            for(--p; *p != '/'; --p);  /* p points to slash before "0003:413C:2003.0013" */
            for(--p; *p != '/'; --p);  /* p points to slash before interface sys name */
            ++p;                       /* p points to interface sys name */

            setBuffer(&device, p);
            trimEndBuffer(&device, ':');
            processNewHidraw(endPath, device.data);
        }
    }
    else if(action != NULL && strcmp(action, "remove") == 0)
    {
        if(type != NULL && strcmp(type, "usb_device") == 0)
        {
            /* Device removal spotted
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3
             *
             * We need the device sys name (2-1.1.3)
             */
            LogInfo("Spotted leaving device %s", endPath);
            removeDevice(endPath);
        }
        else if(type != NULL && strcmp(type, "usb_interface") == 0)
        {
            /* Interface removal spotted
             * Path is eg:
             * /devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.1/2-1.1.3/2-1.1.3:1.0
             *
             * We need the device sys name (2-1.1.3)
             */
            LogInfo("Spotted leaving interface %s", endPath);
            setBuffer(&device, endPath);
            trimEndBuffer(&device, ':');
            processRemovedInterface(device.data);
            /* Interface removal spotted, don't care */
        }
    }

    udev_device_unref(dev);
}


/* Initialise our system interactions, including detecting existing devices
 * return                        udev monitor file descriptor, <0 on error
 */
int initSys(void)
{
    FILE *probeFile;      /* Handle to control file to disable autobinding */
    int udev_fd;          /* Udev monitor file descriptor */
    DIR *dir;             /* Handle to existing device directory */
    struct dirent *file;  /* File handle for each device found */

    /* Disable autobinding */
    probeFile = fopen(USB_SYS_CTRL_FILE, "w");

    if(probeFile == NULL)
    {
        LogError("Cannot turn off autoprobe");
        return -EPERM;
    }

    LogInfo("Turning off autoprobe");
    fprintf(probeFile, "0");
    fclose(probeFile);

    /* Initialise udev monitor */
    udev = udev_new();
    if(udev == NULL)
    {
        LogInfo("Can't create udev monitor");
        return -EPERM;
    }

    udev_mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "usb", NULL);
    udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "scsi", NULL);
    udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "hidraw", NULL);
    udev_monitor_enable_receiving(udev_mon);
    udev_fd = udev_monitor_get_fd(udev_mon);

    /* Find existing devices */
    dir = opendir(USB_SYS_ROOT_DIR);

    if(dir == NULL)
    {
        LogError("Could not open directory %s (%d)", USB_SYS_ROOT_DIR, errno);
        return -EPERM;
    }
    
    while((file = readdir(dir)) != NULL)
    {
        if(file->d_type == DT_LNK && strchr(file->d_name, ':') == NULL)
        {
            int id;  /* id of device found */
            LogInfo("Adding existing device %s", file->d_name);
            id = readDevice(file->d_name);

            if(id >= 0) {
                claimDevice(id, 0);
		assignNewDevice(id);
	    }
        }
    }

    closedir(dir);

    usb_init();

    return udev_fd;
}
