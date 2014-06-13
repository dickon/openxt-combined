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
 * "Always" assigned handling module
 */

#include "project.h"


#define ALWAYS_FILE_PATH "/config/etc/USB_always.conf"


/* We identify devices by Vid, Pid and serial string */

typedef struct _always {
    int Vid;
    int Pid;
    char *serial;
    char *VM;
    struct _always *next;
} Always;

static Always *knownAlways = NULL;



/* File loading utilities */

/* Dump the rest of the line in the file
 * param    file                 file being processed
 * return                        <0 if is line is last in file, 0 otherwise
 */
int dumpLine(FILE *file)
{
    int c;  /* Next character */

    assert(file != NULL);

    while(1)
    {
        c = fgetc(file);

        if(c == EOF)
            return -1;

        if(c == '\n')
            return 0;
    }
}


/* Get the next character from the given file, ignoring whitespace and comments
 * param    file                 file being processed
 * return                        next character
 */
int getNextChar(FILE *file)
{
    int c;

    assert(file != NULL);

    while(1)
    {
        c = fgetc(file);

        if(c == '#')
        {
            /* Comment ignore rest of line */
            while(c != '\n' && c != EOF)
                c = fgetc(file);

            continue;
        }

        if(c == ' ' || c == '\t' || c == '\n' || c == '\r' )  /* ignore whitespace */
            continue;

        return c;
    }
}


/* Load a hex number with the specified number of digits from a file
 * param    file                 file being processed
 * param    digits               number of digits requried
 * param    filename             file name, for error reporting
 * return                        number loaded, <0 if not found
 */
int loadHex(FILE *file, int digits, const char *filename)
{
    int c;      /* Next character */
    int value;  /* Current value read */
    int count;  /* Digit counter */

    assert(file != NULL);
    assert(digits >= 0);
    assert(filename != NULL);

    value = 0;
    for(count = 0; count < digits; ++count)
    {
        c = getNextChar(file);

        if(c >= 'a' && c <= 'f')
        {
            value = (value * 16) + c - 'a' + 10;
        }
        else if(c >= 'A' && c <= 'F')
        {
            value = (value * 16) + c - 'A' + 10;
        }
        else if(c >= '0' && c <= '9')
        {
            value = (value * 16) + c - '0';
        }
        else if(c == EOF)
        {
            return -1;
        }
        else
        {
            LogError("Unexpected character '%c' in file %s", c, filename);
            return -1;
        }
    }
    
    return value;
}


/* Load a specific character from a file
 * param    file                 file being processed
 * param    character            character expected
 * param    filename             file name, for error reporting
 * return                        0 on success, <0 if not found
 */
int loadChar(FILE *file, char character, const char *filename)
{
    int c;      /* Next character */

    assert(filename != NULL);

    c = getNextChar(file);

    if(c == character)
        return 0;

    if(c == EOF)
        LogError("Unexpected EOF in file %s", filename);
    else
        LogError("Unexpected character '%c' in file %s", c, filename);

    return -1;
}


/* Load a quoted string from a file
 * param    file                 file being processed
 * param    filename             file name, for error reporting
 * return                        0 on success, non-0 if not found
 */
int loadString(FILE *file, Buffer *buffer, const char *filename)
{
    int c;  /* Next character */

    assert(file != NULL);
    assert(buffer != NULL);
    assert(filename != NULL);

    setBuffer(buffer, "");

    if(loadChar(file, '\"', filename) != 0)
        return -1;

    while(1)
    {
        c = fgetc(file);

        if(c == '\"')
            return 0;

        if(c == EOF)
        {
            LogError("Unexpected EOF in string in file %s", filename);
            return -1;
        }

        if(c == '\\')
        {
            /* Check escape sequence */
            c = fgetc(file);

            if(c == EOF)
            {
                LogError("Unexpected EOF in string in file %s", filename);
                return -3;
            }

            if(c == '\\')
            {
                catToBuffer(buffer, "\\");
            }
            else if(c == '\"')
            {
                catToBuffer(buffer, "\"");
            }
            else
            {
                LogError("Unrecognised escape sequence \\%c in file %s", c, filename);
                return -2;
            }
        }
        else
        {
            char d[2] = {c, '\0'};
            catToBuffer(buffer, d);
        }
    }
}


/* Write the given string to the specified file, hanlding \ and " escapes
 * param    file                 file to write string to
 * param    string               string to write to file
 */
void storeString(FILE *file, const char *string)
{
    const char *p;

    assert(file != NULL);
    assert(string != NULL);

    for(p = string; *p != '\0'; ++p)
    {
        if(*p == '\"')
            fprintf(file, "\\\"");
        else if(*p == '\\')
            fprintf(file, "\\\\");
        else
            fprintf(file, "%c", *p);
    }
}



/* Functions to handle always flag store */

/* Initialise our known spec list by reading the always file */
void alwaysInit(void)
{
    static Buffer serial = { NULL, 0 };  /* Each serial number */
    static Buffer uuid = { NULL, 0 };    /* UUID of each VM */
    FILE *file;    /* Definition file */
    int Vid;       /* Device Vid */
    int Pid;       /* Device Pid */
    Always *p;     /* New record */

    file = fopen(ALWAYS_FILE_PATH, "r");
    if(file == NULL)
    {
        LogInfo("Could not open always attached file " ALWAYS_FILE_PATH);
        return;
    }

    LogInfo("Reading always attached file " ALWAYS_FILE_PATH);

    while(1)
    {
        Vid = loadHex(file, 4, ALWAYS_FILE_PATH);

        if(Vid < 0)  /* End of file reached */
            break;

        if(loadChar(file, ':', ALWAYS_FILE_PATH) < 0)  /* error */
            break;

        Pid = loadHex(file, 4, ALWAYS_FILE_PATH);
        if(Pid < 0)  /* error */
            break;

        if(loadChar(file, ':', ALWAYS_FILE_PATH) < 0)  /* error */
            break;

        if(loadString(file, &serial, ALWAYS_FILE_PATH) != 0)  /* error */
            break;

        if(loadChar(file, '=', ALWAYS_FILE_PATH) < 0)  /* error */
            break;

        if(loadString(file, &uuid, ALWAYS_FILE_PATH) != 0)  /* error */
            break;

        /* we have a complete always spec, check it doesn't already exist */
        if(alwaysGet(Vid, Pid, serial.data) != NULL)
        {
            LogInfo("Ignoring duplicate always attach spec for device %04X:%04X:\"%s\"", Vid, Pid, serial.data);
        }
        else
        {
            p = malloc(sizeof(Always));
            if(p == NULL)
            {
                LogError("Could not allocate always attached spec (%d bytes)", sizeof(Always));
                exit(2);
            }

            p->Vid = Vid;
            p->Pid = Pid;
            p->serial = strcopy(serial.data);
            p->VM = strcopy(uuid.data);
            p->next = knownAlways;
            knownAlways = p;
        }
    }

    fclose(file);
}


/* Save our known always specs to file */
static void alwaysSave(void)
{
    FILE *file;
    Always *p;

    file = fopen(ALWAYS_FILE_PATH, "w");
    if(file == NULL)
    {
        LogError("Could not save to always attached file " ALWAYS_FILE_PATH);
        return;
    }

    for(p = knownAlways; p != NULL; p = p->next)
    {
        fprintf(file, "%04X:%04X:\"", p->Vid, p->Pid);
        storeString(file, p->serial);
        fprintf(file, "\"=\"");
        storeString(file, p->VM);
        fprintf(file, "\"\n");
    }

    fclose(file);
}


/* Check whether we have an always flag for the specified device
 * param    Vid                  Vid of device to check for
 * param    Pid                  Pid of device to check for
 * param    serial               serial number of device to check for
 * return                        uuid of VM to always connect to, or NULL for none
 */
char *alwaysGet(int Vid, int Pid, const char *serial)
{
    Always *p;

    assert(serial != NULL);

    /* Check our known always specifications */
    for(p = knownAlways; p != NULL; p = p->next)
    {
        if(p->Vid == Vid && p->Pid == Pid && strcmp(p->serial, serial) == 0)  /* match */
            return p->VM;
    }

    /* no match */
    return NULL;
}


/* Check whether the always flag for the specified device (if any) is the same as the given uuid
 * param    Vid                  Vid of device to check for
 * param    Pid                  Pid of device to check for
 * param    serial               serial number of device to check for
 * param    uuid                 uuid to compare to
 * return                        non-0 for match, 0 otherwise
 */
int alwaysCompare(int Vid, int Pid, const char *serial, const char *uuid)
{
    Always *p;

    assert(serial != NULL);
    assert(uuid != NULL);

    /* Check our known always specifications */
    for(p = knownAlways; p != NULL; p = p->next)
    {
        if(p->Vid == Vid && p->Pid == Pid && strcmp(p->serial, serial) == 0)  /* Device found */
            return strcmp(p->VM, uuid) == 0;
    }

    /* Not found, no match */
    return 0;
}


/* Set the always attached flag for the given device
 * param    Vid                  Vid of device to check for
 * param    Pid                  Pid of device to check for
 * param    serial               serial number of device to check for
 * param    VM                   uuid of VM to always connect to
 */
void alwaysSet(int Vid, int Pid, const char *serial, const char *VM)
{
    Always *p;

    assert(serial != NULL);
    assert(VM != NULL);

    /* Check our existing always specifications */
    for(p = knownAlways; p != NULL; p = p->next)
    {
        if(p->Vid == Vid && p->Pid == Pid && strcmp(p->serial, serial) == 0)
        {
            /* match, update VM */
            LogInfo("Update always spec %04X:%04X:%s = VM %s", Vid, Pid, serial, VM);
            assert(p->VM != NULL);
            free(p->VM);
            p->VM = strcopy(VM);
            alwaysSave();
            return;
        }
    }

    /* No match, add new spec */
    LogInfo("Add always spec %04X:%04X:%s = VM %s", Vid, Pid, serial, VM);
    p = malloc(sizeof(Always));
    if(p == NULL)
    {
        LogError("Could not allocate always attached spec (size %d)", sizeof(Always));
        exit(2);
    }

    p->Vid = Vid;
    p->Pid = Pid;
    p->serial = strcopy(serial);
    p->VM = strcopy(VM);
    p->next = knownAlways;
    knownAlways = p;
    alwaysSave();
}


/* Clear the always attached flag for the given device
 * param    Vid                  Vid of device to check for
 * param    Pid                  Pid of device to check for
 * param    serial               serial number of device to check for
 */
void alwaysClear(int Vid, int Pid, const char *serial)
{
    alwaysClearOther(Vid, Pid, serial, NULL);
}


/* Clear the always attached flag for the given device, unless it is for the specified uuid
 * param    Vid                  Vid of device to check for
 * param    Pid                  Pid of device to check for
 * param    serial               serial number of device to check for
 * param    uuid                 allowed uuid
 */
void alwaysClearOther(int Vid, int Pid, const char *serial, const char *uuid)
{
    Always *p, **q;

    assert(serial != NULL);

    /* Check our existing always specifications */
    p = knownAlways;
    q = &knownAlways;
    while(p != NULL)
    {
        assert(p->serial != NULL);

        if(p->Vid == Vid && p->Pid == Pid && strcmp(p->serial, serial) == 0)
        {
            /* Match */
            if(uuid != NULL && strcmp(p->VM, uuid) == 0)  /* Allowed */
                return;

            /* Remove spec */
            LogInfo("Removing always spec %04X:%04X:%s = VM %s", Vid, Pid, serial, p->VM);
            free(p->serial);
            free(p->VM);
            *q = p->next;
            free(p);
            alwaysSave();
            return;
        }

        q = &p->next;
        p = p->next;
    }

    /* No match */
}


/* Dump all our always attached specs to syslog, for debugging only
 * param    buffer               buffer to write state into
 */
void alwaysDump(Buffer *buffer)
{
    Always *p;

    assert(buffer != NULL);

    if(knownAlways == NULL)
    {
        catToBuffer(buffer, "No always flags\n");
        return;
    }

    catToBuffer(buffer, "Always attached:\n");

    /* Check our existing always specifications */
    for(p = knownAlways; p != NULL; p = p->next)
    {
        assert(p->serial != NULL);
        assert(p->VM != NULL);
        catToBuffer(buffer, "  %04X:%04X:%s = VM %s\n", p->Vid, p->Pid, p->serial, p->VM);
    }
}
