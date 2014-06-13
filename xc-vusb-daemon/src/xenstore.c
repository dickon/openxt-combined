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
 * Xenstore based utilities
 */

#include "project.h"

/*VM states */
enum {
    DS_new = 0,   /* VM is new and requires USB devices to be assigned to it */
    DS_running,   /* VM is running */
    DS_stopping,  /* VM is stopping and should not have any devices assigned to it */
    DS_max        /* Unassignable value for range checking */
};

static const char *state_names[DS_max] = {  /* Human readable state names for debug output */
    "new", "running", "stopping"
};

typedef struct {  /* Information about running domains */
    int id;       /* Domain id */
    int state;    /* Domain state (a DS_* value) */
    char *uuid;   /* Domain UUID */
} VMinfo;

static VMinfo *runningVMs = NULL;    /* Info about running VMs */
static int runningVMcount = 0;       /* Number of VMs we know about */
static int UIVM_id = -1;             /* domid of UIVM */

/* Look up the UUID of the domain with the specified id, putting it into the uuid buffer
 * param    domid                id of domain to find UUID for
 * param    uuid                 buffer to put uuid in
 * return                        0 on success, non-0 on failure
 */
static int id2uuid(int32_t domid, Buffer *uuid)
{
    int r;
    char *uuidkey;

    assert(uuid != NULL);

    uuidkey = xenstore_dom_read(domid,"vm");

    if(uuidkey == NULL || strlen(uuidkey) <= 4)
    {
        LogError("Cannot get UUID of domain %d", domid);
        setBuffer(uuid, "");
        r = -ENOENT;
    }
    else
    {
        setBuffer(uuid, uuidkey + 4);  /* uuidkey contains /vm/<uuid> */
        r = 0;
    }
    
    if(uuidkey != NULL)
        free(uuidkey);

    return r;
}



/* Functions to handle list of running domains */

/* Update our list of running domains
 * param    new_id               id of new VM, <0 for none
 */
void updateVMs(int new_id)
{
    static IBuffer idBuf = { NULL, 0, 0 };  /* Buffer for VM ids */
    static Buffer uuid = { NULL, 0 };       /* buffer for each VM's UUID */
    int count;           /* Number of VMs running */
    int *ids;            /* IDs of running VMs */
    VMinfo *newInfo;     /* Updated info for running VMs */
    int i, j;            /* VM counters */
    char *uivm_str;      /* UIVM domid */
    
    /* Determine domain id of UIVM */
    uivm_str = xenstore_read("/xenmgr/vms/" UIVM_UUID "/domid");

    if(uivm_str == NULL || sscanf(uivm_str, "%d", &UIVM_id) < 1)
    {
        LogError("Could not determine domain id of UIVM");
        UIVM_id = -1;
    }

    if(uivm_str != NULL)
        free(uivm_str);


    /* Get list of running VMs */
    IBufferSize(&idBuf, 4);

    while(1)
    {
        ids = idBuf.data;

        /* Find the currently running domains */
        if(xcdbus_xenmgr_list_domids(rpc_xcbus(), ids, idBuf.allocated * sizeof(int), &count) == 0)
        {
            LogError("Could not query running domains");
            count = 0;
            break;
        }

        if(count < idBuf.allocated)
            break;

        /* Buffer may not be big enough, enlarge it and try again */
        IBufferSize(&idBuf, idBuf.allocated * 2);
    }

    /* Setup new VM info list */
    assert(count >= 0);
    newInfo = malloc((count + 1) * sizeof(VMinfo));  /* +1 for dom0 */
    if(newInfo == NULL)
    {
        LogError("Could not allocate VM info (%d bytes)", (count + 2) * sizeof(VMinfo));
        exit(2);
    }

    for(i = 0 ; i < count; ++i)
    {
        newInfo[i].id = ids[i];
        newInfo[i].state = DS_running;

        if(id2uuid(ids[i], &uuid) != 0)
            setBuffer(&uuid, "ERROR");

        newInfo[i].uuid = strcopy(uuid.data);

        /* Check old list for VM states */
        for(j = 0; j < runningVMcount; ++j)
        {
            if(runningVMs[j].id == newInfo[i].id)
            {
                newInfo[i].state = runningVMs[j].state;
                runningVMs[j].state = DS_stopping;  /* Don't purge devices from old version */
                break;
            }
        }

        if(newInfo[i].id == new_id)  /* This is a new VM */
            newInfo[i].state = DS_new;
    }

    /* The list of ids we get back from XenStore doesn't include UIVM or dom0, so add them separately */
    newInfo[count].id = 0;
    newInfo[count].state = DS_running;
    newInfo[count].uuid = strcopy(DOM0_UUID);
    ++count;
    
    /* Find any VMs that have disappeared */
    for(i = 0; i < runningVMcount; ++i)
    {
        if(runningVMs[i].id != 0 &&
           runningVMs[i].state != DS_stopping)  /* Old VM at index i has disappeared */
            purgeDeviceAssignments(runningVMs[i].id);
    }

    /* Replace old list with new one */
    for(i = 0; i < runningVMcount; ++i)
    {
        assert(runningVMs[i].uuid != NULL);
        free(runningVMs[i].uuid);
    }

    if(runningVMs != NULL)
        free(runningVMs);

    runningVMs = newInfo;
    runningVMcount = count;
}


/* Mark the specified VM as stopping
 * param    dom_id               id of domain that's stopping
 */
void VMstopping(int dom_id)
{
    int i;

    for(i = 0; i < runningVMcount; ++i)
    {
        if(runningVMs[i].id == dom_id)
        {
            /* VM found */
            runningVMs[i].state = DS_stopping;
            purgeDeviceAssignments(dom_id);
            return;
        }
    }

    LogInfo("Could not stop VM %d, not found", dom_id);
}


/* Process our new VMs, if any
 */
void processNewVMs(void)
{
    int i;

    for(i = 0; i < runningVMcount; ++i)
    {
        if(runningVMs[i].state == DS_new)
        {
            /* new VM found */
            assignToNewVM(runningVMs[i].id);
            runningVMs[i].state = DS_running;
        }
    }
}


/* Report our list of running VM ids
 * param    buffer               integer buffer to put ids in
 */
void getVMs(IBuffer *buffer)
{
    int i;

    assert(buffer != NULL);

    IBufferSize(buffer, runningVMcount);
    clearIBuffer(buffer);
    
    for(i = 0; i < runningVMcount; ++i)
        if(runningVMs[i].state != DS_stopping)
            appendIBuffer(buffer, runningVMs[i].id);
}


/* Report the domain id for the UIVM
 * return                        id of UIVM, <0 for error
 */
int getUIVMid(void)
{
    return UIVM_id;
}


/* Dump out our list of running VMS, for debugging
 * param    buffer               buffer to write state into
 */
void dumpVMs(Buffer *buffer)
{
    int i;

    assert(buffer != NULL);

    catToBuffer(buffer, "Running VMs (%d):\n", runningVMcount);

    for(i = 0; i < runningVMcount; ++i)
        catToBuffer(buffer, "  %d [%s] - %s\n", runningVMs[i].id,
                    state_names[runningVMs[i].state], runningVMs[i].uuid);
}


/* Find the UUID for the specified VM
 * param    domid                id of VM to find UUID for
 * return                        UUID of VM, NULL if not found
 */
const char *VMid2uuid(int domid)
{
    int i;

    assert(domid >= 0);

    for(i = 0; i < runningVMcount; ++i)
    {
        if(runningVMs[i].id == domid)
        {
            if(runningVMs[i].state == DS_stopping)
                return NULL;

            return runningVMs[i].uuid;
        }
    }

    /* Not found */
    return NULL;
}


/* Find the id for the specified VM
 * param    uuid                 UUID of VM to find id for
 * return                        id of VM, <0 if not found
 */
int VMuuid2id(const char *uuid)
{
    int i;

    assert(uuid != NULL);

    for(i = 0; i < runningVMcount; ++i)
    {
        if(strcmp(runningVMs[i].uuid, uuid) == 0)
        {
            if(runningVMs[i].state == DS_stopping)
                return -1;

            return runningVMs[i].id;
        }
    }

    /* Not found */
    return -1;
}
