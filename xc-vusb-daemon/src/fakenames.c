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
 * Module to handle fake names, ie those assigned by us for particular devices
 */

#include "project.h"


#define FAKE_FILE_PATH "/config/etc/USB_fakenames.conf"


/* We allow arbitrary strings to be provided for device names. There are 2 forms:
 *   specific device names which match Vid:Pid:serial, eg "My iPod"
 *   device variety names which match Vid:Pid, eg "iPod"
 *
 * Any given device may have a specific and a variety match, in which case the specific
 * takes priority.
 */

typedef struct _fakename {
    int Vid;
    int Pid;
    char *serial;  /* NULL for variety names */
    char *name;
    struct _fakename *next;
} FakeName;

static FakeName *varietyNames = NULL;
static FakeName *specificNames = NULL;


/* Find the fake name, if any, for the given Vid:Pid:serial triple
 * param    Vid                  Vid to find name for
 * param    Pid                  Pid to find name for
 * param    serial               serial number to find name for, NULL for none
 * return                        fake name record, on NULL if not found
 */
static FakeName *findFake(int Vid, int Pid, const char *serial)
{
    FakeName *p;

    if(serial != NULL)
    {
        /* First check specific */
        for(p = specificNames; p != NULL; p = p->next)
            if(p->Vid == Vid && p->Pid == Pid && strcmp(p->serial, serial) == 0)  /* Match */
                return p;
    }

    /* Now check variety names */
    for(p = varietyNames; p != NULL; p = p->next)
        if(p->Vid == Vid && p->Pid == Pid)  /* Match */
            return p;

    /* No match */
    return NULL;
}


/* Add the given fake name to our list
 * param    Vid                  Vid of device to replace name for
 * param    Pid                  Pid of device to replace name for
 * param    serial               serial number of device
 * param    name                 replacement name
 * return                        non-0 if we overwrote an existing name, 0 otherwise
 */
static int addName(int Vid, int Pid, const char *serial, const char *name)
{
    FakeName *p;

    assert(name != NULL);

    p = findFake(Vid, Pid, serial);

    if(p != NULL && ((serial == NULL && p->serial == NULL) || (serial != NULL && p->serial != NULL)))
    {
        /* We already have a fake name for this device, update it */
        assert(p->name != NULL);
        free(p->name);
        p->name = strcopy(name);
        return 1;
    }

    /* We don't yet have a fake name for this device, add one */
    p = malloc(sizeof(FakeName));
    if(p == NULL)
    {
        LogError("Could not allocate fake name record (%d bytes)", sizeof(FakeName));
        exit(2);
    }

    p->Vid = Vid;
    p->Pid = Pid;
    p->name = strcopy(name);

    if(serial == NULL)
    {
        /* Add a variety name */
        p->serial = NULL;
        p->next = varietyNames;
        varietyNames = p;
    }
    else
    {
        /* Add a specific name */
        p->serial = strcopy(serial);
        p->next = specificNames;
        specificNames = p;
    }
        
    return 0;
}


/* Initialise our name list by reading the provided file */
void initFakeNames(void)
{
    static Buffer name = { NULL, 0 };     /* Name for each device */
    static Buffer serial = { NULL, 0 };   /* Serial number for each device */
    FILE *file;    /* Definition file */
    int Vid;       /* Vid of each device */
    int Pid;       /* Pid of each device */
    FakeName *p;   /* Each fake */
    int c;         /* Next character */

    file = fopen(FAKE_FILE_PATH, "r");
    if(file == NULL)
    {
        LogInfo("Could not open fake names file " FAKE_FILE_PATH);
        return;
    }

    LogInfo("Reading fake names file " FAKE_FILE_PATH);

    while(1)
    {
        clearBuffer(&serial);
        Vid = loadHex(file, 4, FAKE_FILE_PATH);

        if(Vid < 0)  /* End of file reached */
            break;

        if(loadChar(file, ':', FAKE_FILE_PATH) < 0)  /* error */
            break;

        Pid = loadHex(file, 4, FAKE_FILE_PATH);

        if(Pid < 0)  /* error */
            break;

        c = getNextChar(file);

        if(c == ':')
        {
            /* Specific name, expect a serial number */
            if(loadString(file, &serial, FAKE_FILE_PATH) != 0)  /* error */
                break;

            if(loadChar(file, '=', FAKE_FILE_PATH) < 0)  /* error */
                break;

            if(loadString(file, &name, FAKE_FILE_PATH) != 0)  /* error */
                break;

            /* We have a specific fake name, add it to our list */
            if(addName(Vid, Pid, serial.data, name.data) != 0)
            {
                LogInfo("Duplicate fake names found for device %04X:%04X:\"%s\"", Vid, Pid, serial.data);
            }
        }
        else if(c == '=')
        {
            /* Variety name, do not expect a serial number */
            if(loadString(file, &name, FAKE_FILE_PATH) != 0)  /* error */
                break;

            /* We have a specific fake name, add it to our list */
            if(addName(Vid, Pid, NULL, name.data) != 0)
            {
                LogInfo("Duplicate fake names found for device %04X:%04X", Vid, Pid);
            }
        }
        else
        {
            if(c == EOF)
                LogError("Unexpected EOF in file " FAKE_FILE_PATH);
            else
                LogError("Unexpected character '%c' in file " FAKE_FILE_PATH, c);
            break;
        }
    }

    fclose(file);
}


/* Save our fake names list out to file
 */
static void saveNames(void)
{
    FILE *file;
    FakeName *p;

    file = fopen(FAKE_FILE_PATH, "w");
    if(file == NULL)
    {
        LogError("Could not save to fake names file " FAKE_FILE_PATH);
        return;
    }

    for(p = varietyNames; p != NULL; p = p->next)
    {
        fprintf(file, "%04X:%04X=\"", p->Vid, p->Pid);
        storeString(file, p->name);
        fprintf(file, "\"\n");
    }

    for(p = specificNames; p != NULL; p = p->next)
    {
        fprintf(file, "%04X:%04X:\"", p->Vid, p->Pid);
        storeString(file, p->serial);
        fprintf(file, "\"=\"");
        storeString(file, p->name);
        fprintf(file, "\"\n");
    }

    fclose(file);
}


/* Replace the given name with a fake one, if we have one
 * param    Vid                  Vid of device to replace name for
 * param    Pid                  Pid of device to replace name for
 * param    serial               serial number of device
 * param    name                 buffer to write fake name into, if found
 */
void getFakeName(int Vid, int Pid, const char *serial, Buffer *name)
{
    FakeName *p;

    assert(serial != NULL);
    assert(name != NULL);

    p = findFake(Vid, Pid, serial);

    if(p == NULL)  /* No fake for this name */
        return;

    /* Replace existing name with fake */
    assert(p->name != NULL);
    setBuffer(name, p->name);
}


/* Set the replacement name for the specified device
 * param    Vid                  Vid of device to replace name for
 * param    Pid                  Pid of device to replace name for
 * param    serial               serial number of device
 * param    name                 replacement name
 */
void setFakeName(int Vid, int Pid, const char *serial, const char *name)
{
    FakeName *p;

    assert(serial != NULL);
    assert(name != NULL);

    addName(Vid, Pid, serial, name);
    saveNames();
}


/* Dump out our fake names, for debugging only
 * param    buffer               buffer to write state into
 */
void dumpFakeNames(Buffer *buffer)
{
    FakeName *p;

    assert(buffer != NULL);

    if(varietyNames == NULL && specificNames == NULL)
    {
        catToBuffer(buffer, "No fake names found\n");
        return;
    }

    catToBuffer(buffer, "Fake names:\n");

    for(p = varietyNames; p != NULL; p = p->next)
        catToBuffer(buffer, "  %04X:%04X = \"%s\"\n", p->Vid, p->Pid, p->name);

    for(p = specificNames; p != NULL; p = p->next)
        catToBuffer(buffer, "  %04X:%04X:\"%s\" = \"%s\"\n", p->Vid, p->Pid, p->serial, p->name);
}


/* Sanitise the given name regarding white space and non-printable characters
 * param    buffer               buffer containing string to sanitise
 */
void sanitiseName(Buffer *buffer)
{
    static Buffer t = { NULL, 0 };  /* Temporary use buffer */
    int white;  /* Are we currently in a run of white space */
    char s[2];  /* Each character to add to sanitised buffer */
    int i;

    assert(buffer != NULL);

    clearBuffer(&t);
    white = 1;  /* Ditch leading white space */
    s[1] = '\0';

    for(i = 0; buffer->data[i] != '\0'; ++i)
    {
        s[0] = buffer->data[i];
        if(isspace(s[0]))
        {
            /* This is white space, ditch all but the first character in each contiguous block
             * and make that a space */
            if(white == 0)
                catToBuffer(&t, " ");

            white = 1;
        }
        else if(isprint(s[0]))
        {
            /* This is a printable character */
            catToBuffer(&t, s);
            white = 0;
        }
        else
        {
            /* This is a non-printable character */
            s[0] = '#';
            catToBuffer(&t, s);
            white = 0;
        }
    }

    if(white != 0 && strlen(t.data) > 0)  /* Ditch trailing white space */
        t.data[strlen(t.data) - 1] = '\0';

    setBuffer(buffer, t.data);
}
