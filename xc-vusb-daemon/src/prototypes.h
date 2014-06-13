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

/* rpc.c */
extern gboolean is_usb_enabled(int domid);
extern gboolean has_device_grab(int domid);
extern xcdbus_conn_t *rpc_xcbus(void);
extern int remote_plug_device(int domid, int bus_num, int dev_num);
extern int remote_unplug_device(int domid, int bus_num, int device_id);
extern int remote_query_devices(int domid, IBuffer *device_ids);
extern void remote_report_rejected(const char *dev_name, const char *cause);
extern void remote_report_all_devs_change(void);
extern void remote_report_dev_change(int dev_id);
extern void rpc_init(void);
/* main.c */
extern int main(int argc, char **argv);
/* policy.c */
extern const char *fetchPolicy(const char *domuuid);
extern int storePolicy(const char *domuuid, const char *policy);
extern void deletePolicies(void);
extern void dumpPolicy(Buffer *buffer);
extern int consultPolicy(const char *uuid, int Vid, int Pid, int class_count, int *classes);
/* buffer.c */
extern void bufferSize(Buffer *buffer, int size);
extern void growBuffer(Buffer *buffer, int size);
extern void clearBuffer(Buffer *buffer);
extern void setBuffer(Buffer *buffer, const char *string);
extern void catToBuffer(Buffer *buffer, const char *format, ...);
extern int trimEndBuffer(Buffer *buffer, char terminator);
extern char *copyBuffer(Buffer *buffer);
extern int isBufferWhite(Buffer *buffer);
extern void IBufferSize(IBuffer *buffer, int size);
extern void clearIBuffer(IBuffer *buffer);
extern void appendIBuffer(IBuffer *buffer, int value);
extern char *strcopy(const char *string);
/* devstore.c */
extern int makeDeviceId(int bus_num, int dev_num);
extern void makeBusDevPair(int devid, int *bus_num, int *dev_num);
extern void clearStickyIfPolicyBlocked(int devid, const char *dom_uuid);
extern void purgeDeviceAssignments(int dom_id);
extern int addDevice(int Vid, int Pid, const char *serial, int bus_num, int dev_num, const char *name, const char *fullName, const char *sysName, IBuffer *classBuf);
extern void removeDevice(const char *sysName);
extern void dumpDevice(Buffer *buffer);
extern void claimDevice(int dev_id, int dom_id);
extern void checkAllOptical(void);
extern void checkAllKeyboard(void);
extern void checkDeviceOptical(const char *sysName, int host);
extern void processNewHidraw(const char *hidName, const char *devName);
extern void processNewInterface(const char *sysDev, const char *sysIface);
extern void processRemovedInterface(const char *sysDev);
extern void getDeviceList(IBuffer *buffer);
extern int getDeviceInfo(int dev_id, const char *vm_uuid, Buffer *name, Buffer *fullName, Buffer *vm_assigned);
extern void queryExistingDevices(void);
extern void assignNewDevice(int dev_id);
extern void assignToNewVM(int dom_id);
extern void assignToExistingVM(int dev_id, const char *uuid);
extern void unassignFromVM(int dev_id);
extern void setSticky(int dev_id, int sticky);
extern void setDeviceName(int dev_id, const char *name);
/* always.c */
extern int dumpLine(FILE *file);
extern int getNextChar(FILE *file);
extern int loadHex(FILE *file, int digits, const char *filename);
extern int loadChar(FILE *file, char character, const char *filename);
extern int loadString(FILE *file, Buffer *buffer, const char *filename);
extern void storeString(FILE *file, const char *string);
extern void alwaysInit(void);
extern char *alwaysGet(int Vid, int Pid, const char *serial);
extern int alwaysCompare(int Vid, int Pid, const char *serial, const char *uuid);
extern void alwaysSet(int Vid, int Pid, const char *serial, const char *VM);
extern void alwaysClear(int Vid, int Pid, const char *serial);
extern void alwaysClearOther(int Vid, int Pid, const char *serial, const char *uuid);
extern void alwaysDump(Buffer *buffer);
/* xenstore.c */
extern void updateVMs(int new_id);
extern void VMstopping(int dom_id);
extern void processNewVMs(void);
extern void getVMs(IBuffer *buffer);
extern int getUIVMid(void);
extern void dumpVMs(Buffer *buffer);
extern const char *VMid2uuid(int domid);
extern int VMuuid2id(const char *uuid);
/* sys.c */
extern void bindToDom0(const char *sysName);
extern int bindToUsbhid(const char *bus_id);
extern void unbindAllFromDom0(const char *sysName);
extern void unbindAllFromDom0ExceptRootDevice(const char *sysName);
extern void bindAllToDom0(const char *sysName);
extern int isHostOptical(int host, const char *sysDev);
extern int isDeviceOptical(const char *sysName);
extern int checkHidrawKeyboard(const char *hidName, const char *name);
extern int checkHidKeyboard(const char *sysName);
extern void resetDevice(const char *sysName, int busNum, int devNum);
extern void resumeDevice(const char *sysName);
extern void suspendDevice(const char *sysName);
extern int isInterfaceHid(const char *sysName);
extern int getDeviceInterfaceCount(const char *sysName);
extern void udevMonHandler(void);
extern int initSys(void);
/* fakenames.c */
extern void initFakeNames(void);
extern void getFakeName(int Vid, int Pid, const char *serial, Buffer *name);
extern void setFakeName(int Vid, int Pid, const char *serial, const char *name);
extern void dumpFakeNames(Buffer *buffer);
extern void sanitiseName(Buffer *buffer);
/* tablenames.c */
extern void findTableName(int Vid, int Pid, Buffer *vendor, Buffer *product);
