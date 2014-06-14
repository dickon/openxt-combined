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

#include <stdio.h>
#include <string.h>
#include "arguments.h"
#include "util.h"

#define ARGS_BATCH 10

void arguments_init (s_arguments *args)
{
    args->pos = 1;
    args->args = xmalloc (ARGS_BATCH * sizeof (*args->args));
    args->size = ARGS_BATCH;
    args->args[0] = NULL;
}

char * const * arguments_get (s_arguments *args)
{
    return args->args;
}

void arguments_add (s_arguments *args, const char *arg)
{
    if (args->pos == args->size)
    {
        args->size += ARGS_BATCH;
        args->args = xrealloc (args->args, args->size * sizeof (*args->args));
    }

    args->args[args->pos - 1] = xstrdup (arg);
    args->args[args->pos++] = NULL;
}

char * const *find_arg (char * const *argv, const char *arg)
{
    int i = 0;

    for (i = 0; argv[i]; i++)
    {
        if (!strcmp (arg, argv[i]))
            return &argv[i];
    }

    return NULL;
}
