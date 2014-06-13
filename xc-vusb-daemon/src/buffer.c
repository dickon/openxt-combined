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
 * Growable text buffers
 */

#include "project.h"

/* 0 terminated character based buffers (for strings) */

/* Make sure that the given buffer is at least the specified size
 * param    buffer               buffer to size
 * param    size                 required size of buffer
 */
void bufferSize(Buffer *buffer, int size)
{
    assert(buffer != NULL);
    assert(size > 0);

    if(buffer->len >= size)
    {
        /* It's already big enough */
        assert(buffer->data != NULL);
        return;
    }

    //LogDebug("Growing buffer %p to %d bytes\n", buffer, size);

    if(buffer->data != NULL)
        free(buffer->data);

    buffer->data = malloc(size);
    if(buffer->data == NULL)
    {
        LogError("Could not allocate buffer size %d", size);
        exit(2);
    }

    buffer->len = size;
}


/* Make sure that the given buffer is at least the specified size, without destroying its contents
 * param    buffer               buffer to size
 * param    size                 required size of buffer
 */
void growBuffer(Buffer *buffer, int size)
{
    assert(buffer != NULL);
    assert(size > 0);

    if(buffer->len >= size)
    {
        /* It's already big enough */
        assert(buffer->data != NULL);
        return;
    }

    buffer->data = realloc(buffer->data, size);
    if(buffer->data == NULL)
    {
        LogError("Could not allocate buffer size %d", size);
        exit(2);
    }

    buffer->len = size;
}


/* Clear the given buffer to empty
 * param    buffer               buffer to clear
 */
void clearBuffer(Buffer *buffer)
{
    assert(buffer != NULL);
    bufferSize(buffer, 1);
    assert(buffer->data != NULL);
    buffer->data[0] = '\0';
}


/* Make the specified buffer contain the given string
 * param    buffer               buffer to write string to
 * param    string               string to write to buffer
 */
void setBuffer(Buffer *buffer, const char *string)
{
    int i;
    assert(buffer != NULL);
    assert(string != NULL);

    bufferSize(buffer, strlen(string) + 1);  /* +1 for terminator */

    for(i = 0; i < strlen(string); ++i)
        buffer->data[i] = string[i];

    buffer->data[i] = '\0';
}


/* Append the given string to the end of the specified buffer
 * param    buffer               buffer to add string to
 * param    string               string to add to end of buffer
 */
static void appendToBuffer(Buffer *buffer, const char *string)
{
    int len;  /* Required length */

    assert(buffer != NULL);
    assert(string != NULL);

    /* +1 for terminator, +1 cause we'll often need it for separator character */
    len = strlen(buffer->data) + strlen(string) + 2;
    growBuffer(buffer, len);
    strcpy(buffer->data + strlen(buffer->data), string);
}    


#define CHAR_BUF_LEN 1024

/* Append the given string to the specified buffer, with printf style additional arguments
 * param    buffer               buffer to add string to
 * param    format               string to add to end of buffer
 * param    ...                  additional arguments
 */
void catToBuffer(Buffer *buffer, const char *format, ...)
{
    va_list args;
    int len;  /* Length printed */
    static char buf[CHAR_BUF_LEN + 1];

    assert(buffer != NULL);
    assert(format != NULL);

    va_start(args, format);
    len = vsnprintf(buf, CHAR_BUF_LEN, format, args);
    va_end(args);

    if(len < 0)
        appendToBuffer(buffer, "#PRINT ERROR#");
    else if(len > 0)
    {
        buf[len] = '\0';
        appendToBuffer(buffer, buf);
    }
}


/* Remove the end of the given buffer, from the first instance (if any) of the specified terminator
 * param    buffer               buffer to trim
 * param    terminator           termination character
 * return                        0 if termination occured, non-0 otherwise
 */
int trimEndBuffer(Buffer *buffer, char terminator)
{
    int i;

    assert(buffer != NULL);

    for(i = 0; i < buffer->len && buffer->data[i] != '\0'; ++i)
    {
        if(buffer->data[i] == terminator)
        {
            /* Trim point found */
            buffer->data[i] = '\0';
            return 0;
        }
    }

    /* Terminator not found */
    return 1;
}


/* Copy the contents of the specified buffer to newly allcoated storage
 * param    buffer               buffer to copy
 * return                        newly allocated copy of buffer contents
 */
char *copyBuffer(Buffer *buffer)
{
    char *p;  /* Copy */
    int len;  /* Required length */
    int i;    /* Character counter */

    assert(buffer != NULL);

    if(buffer->data == NULL)
        return NULL;

    /* +1 for terminator */
    len = strlen(buffer->data) + 1;

    p = malloc(len * sizeof(char));
    if(p == NULL)
    {
        LogError("Could not allocate memory for buffer size %d", len);
        exit(-2);
    }

    for(i = 0; i < len; ++i)
        p[i] = buffer->data[i];

    return p;
}    


/* Does this buffer contain only whitespace (or nothing)
 * param    buffer               buffer to test
 * return                        0 if buffer contains any non-whitespace characters, non-0 otherwise
 */
int isBufferWhite(Buffer *buffer)
{
    int i;

    assert(buffer != NULL);

    for(i = 0; i < buffer->len && buffer->data[i] != '\0'; ++i)
        if(isprint(buffer->data[i]) == 0 || isspace(buffer->data[i]) != 0)  /* TODO: non-ASCII? */
            return 0;

    return 1;
}



/* Length recorded integer based buffers */

/* Make sure that the given integer buffer can hold at least the specified number of integers
 * param    buffer               buffer to size
 * param    size                 required size of buffer (in integers)
 */
void IBufferSize(IBuffer *buffer, int size)
{
    assert(buffer!= NULL);
    assert(size >= 0);

    if(size == 0)  /* Always allocate something */
        size = 1;

    if(buffer->allocated >= size)
    {
        /* It's already big enough */
        assert(buffer->data != NULL);
        return;
    }

    if(buffer->data != NULL)
        free(buffer->data);

    buffer->data = malloc(size * sizeof(int));
    if(buffer->data == NULL)
    {
        LogError("Could not allocate buffer size %d", size * sizeof(int));
        exit(2);
    }

    buffer->allocated = size;
    buffer->len = 0;
}


/* Clear the given integer buffer to empty
 * param    buffer               buffer to clear
 */
void clearIBuffer(IBuffer *buffer)
{
    assert(buffer != NULL);
    buffer->len = 0;
}


/* Add the given value to the end of the given integer buffer
 * param    buffer               buffer to add value to
 * param    value                value to append to buffer
 */
void appendIBuffer(IBuffer *buffer, int value)
{
    int len;  /* new length */

    assert(buffer != NULL);
    assert(buffer->len >= 0);

    len = buffer->len + 1;

    if(buffer->allocated <= len)
    {
        /* Need more space, add on a bit extra to reduce reallocs */
        len += 8;

        buffer->data = realloc(buffer->data, len * sizeof(int));
        if(buffer->data == NULL)
        {
            LogError("Could not allocate buffer size %d", len * sizeof(int));
            exit(2);
        }

        buffer->allocated = len;
    }

    buffer->data[buffer->len] = value;
    ++buffer->len;
}



/* String utilities */

/* Make a new copy of the given string. This is the same as strdup, but handling the fail case
 * param    string               string to copy
 * return                        new copy of string
 */
char *strcopy(const char *string)
{
    char *s;

    assert(string != NULL);

    s = strdup(string);

    if(s == NULL)
    {
        LogError("Could not allocate string");
        exit(2);
    }

    return s;
}
