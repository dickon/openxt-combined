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
 * Read the system USB vendor and product name table
 */

#include "project.h"


#define TABLE_FILE_PATH "/usr/share/usb.ids"


/* We parse the system provided table of USB vendor and product names each time we are asked
 * for a match. We could read the file once and cache the information, but that would
 * massively increase our memory footprint and since this only needs to be done when a USB
 * device is physically plugged into the system performance is not a major problem.
 */

/* Each line in the table file is exactly one of:
 * * Start of a vendor list
 * * A product within the current vendor list
 * * An interface within the current product
 * * The end of the file
 * * Nothing (whitespace or comment)
 */

#define LINE_TYPE_VENDOR     0
#define LINE_TYPE_PRODUCT    1
#define LINE_TYPE_INTERFACE  2
#define LINE_TYPE_EOF        3


/* Copy the rest of the current line into the given buffer, ignroing leading whitespace
 * param    file                 file being processed
 * param    buffer               buffer to copy line into
 * return                        0 on success, -ENOENT if name string not found
 */
static int loadName(FILE *file, Buffer *buffer)
{
    char d[2] = {'#', '\0'};  /* String for each character to add to buffer */
    int c;                    /* Next character */

    assert(file != NULL);
    assert(buffer != NULL);

    setBuffer(buffer, "");    

    /* First dump leading whitespace */
    while(1)
    {
        c = fgetc(file);

        if(c == EOF || c == '\n')  /* No name found */
            return -ENOENT;

        if(c != ' ' && c != '\t')  /* Non-whitespace found */
            break;
    }

    /* Copy remaining characters to buffer */
    while(1)
    {
        d[0] = c;
        catToBuffer(buffer, d);

        c = fgetc(file);

        if(c == EOF || c == '\n')  /* End of line found */
            return 0;
    }
}


/* Load a line of the table file
 * param    file                 file being processed
 * param    value                location to store vendor / product id from line
 * return                        line type, a LINE_TYPE_* value
 */
static int getLine(FILE *file, int *value)
{
    int c;     /* Next character */
    int type;  /* Type of current line */
    int v;     /* Definition id */
    int i;     /* Digit counter */

    assert(file != NULL);
    assert(value != NULL);

    while(1)
    {
        c = fgetc(file);

        if(c == '#')
        {
            /* Ignore comment lines */
            if(dumpLine(file) < 0)
                return LINE_TYPE_EOF;
        }
        else if(c != '\n')
            break;
    }

    if(c == EOF)
        return LINE_TYPE_EOF;

    if(c != '\t')
    {
        /* No leading tabs */
        type = LINE_TYPE_VENDOR;
    }
    else
    {
        /* First character of line is tab, check the second */
        c = fgetc(file);
        if(c != '\t')
        {
            /* One tab only, product */
            type = LINE_TYPE_PRODUCT;
        }
        else
        {
            /* Two tabs, interface */
            type = LINE_TYPE_INTERFACE;
            c = fgetc(file);
        }
    }

    /* We have a definition, c is the first character of the id */
    v = 0;
    for(i = 0; i < 4; ++i)
    {
        if(i > 0)
            c = fgetc(file);

        if(c >= 'a' && c <= 'f')
        {
            v = (v * 16) + c - 'a' + 10;
        }
        else if(c >= 'A' && c <= 'F')
        {
            v = (v * 16) + c - 'A' + 10;
        }
        else if(c >= '0' && c <= '9')
        {
            v = (v * 16) + c - '0';
        }
        else
        {
            LogError("Error parsing %s, unexpected character '%c' (%x)", TABLE_FILE_PATH, c, v);
            return LINE_TYPE_EOF;
        }
    }

    *value = v;
    return type;
}


/* Find the definition of the specified product within the current vendor
 * param    file                 file being processed
 * param    Pid                  id of product to find
 * param    name                 buffer to put product name in
 * return                        0 on success, -ENOENT if definition not found
 */
static int findProduct(FILE *file, int Pid, Buffer *name)
{
    int type;  /* Type of each line */
    int id;    /* Id of each product definition found */

    assert(name != NULL);

    while(1)
    {
        type = getLine(file, &id);
        
        if(type == LINE_TYPE_INTERFACE)
        {
            /* Useless line, try the next one */
            if(dumpLine(file) < 0)
                return LINE_TYPE_EOF;

            continue;
        }

        if(type == LINE_TYPE_EOF || type == LINE_TYPE_VENDOR)  /* We've gone too far */
            return -ENOENT;

        assert(type == LINE_TYPE_PRODUCT);

        if(id > Pid)  /* Entries are ordered so we've gone too far */
            return -ENOENT;

        if(id == Pid)
        {
            /* Definition found */
            return loadName(file, name);
        }

        /* We're not there yet, carry on to next line */
        if(dumpLine(file) < 0)
            return LINE_TYPE_EOF;
    }
}


/* Find the definition of the specified vendor
 * param    file                 file being processed
 * param    Vid                  id of vendor to find
 * param    name                 buffer to put vendor name in
 * return                        0 on success, -ENOENT if definition not found
 */
static int findVendor(FILE *file, int Vid, Buffer *name)
{
    int type;  /* Type of each line */
    int id;    /* Id of each vendor definition found */

    assert(name != NULL);

    while(1)
    {
        type = getLine(file, &id);
        
        if(type == LINE_TYPE_PRODUCT || type == LINE_TYPE_INTERFACE)
        {
            /* Useless line, try the next one */
            if(dumpLine(file) < 0)
                return LINE_TYPE_EOF;

            continue;
        }

        if(type == LINE_TYPE_EOF)  /* We've gone too far */
            return -ENOENT;

        assert(type == LINE_TYPE_VENDOR);

        if(id > Vid)  /* Entries are ordered so we've gone too far */
            return -ENOENT;

        if(id == Vid)
        {
            /* Definition found */
            return loadName(file, name);
        }

        /* We're not there yet, carry on to next line */
        if(dumpLine(file) < 0)
            return LINE_TYPE_EOF;
    }
}


/* Find the table names for the specified Vid, Pid pair
 * param    Vid                  id of vendor to find
 * param    Pid                  id of product to find
 * param    vendor               buffer to put vendor name in
 * param    product              buffer to put product name in
 */
void findTableName(int Vid, int Pid, Buffer *vendor, Buffer *product)
{
    FILE *file;  /* File definitions are being read from */

    file = fopen(TABLE_FILE_PATH, "r");
    if(file == NULL)
    {
        LogInfo("Could not open USB names file " TABLE_FILE_PATH);
        return;
    }

    setBuffer(vendor, "");
    setBuffer(product, "");

    if(findVendor(file, Vid, vendor) == 0)
        findProduct(file, Pid, product);

    fclose(file);
}
