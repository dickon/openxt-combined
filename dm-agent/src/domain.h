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

#ifndef DOMAIN_H_
# define DOMAIN_H_

# include <event.h>
# include "device-model.h"
# include "list.h"
# include "types.h"

void domains_watch (const char *path, void *priv);

struct domain
{
    domid_t domid;
    char *dompath;
    /* number of time we try to destroy domain */
    unsigned int num_try_destroy;
    struct event ev_destroy;
    /* amount of domain RAM */
    unsigned long memkb;
    /* number of vpcus */
    unsigned int vcpus;
    /* boot order */
    char *boot;
    LIST_ENTRY (struct domain) link;
    /* List of device models */
    LIST_HEAD (, struct device_model) devmodels;
};

/* Watch on xenstore if there is already some domain created */
bool domains_create (void);

struct domain *domain_by_domid (domid_t domid);
struct domain *domain_create (domid_t domid);
/* Notify domain that it will be destroyed */
void domain_notify_destroy (struct domain *d);
/* This function will destroy domain and didn't wait device model killing */
void domain_destroy (struct domain *d, bool killall);
void domains_destroy (bool killall);

#define domain_info(d, fmt, ...) info ("Domain %u: "fmt, (d)->domid, ## __VA_ARGS__)
#define domain_error(d, fmt, ...)                       \
    error ("Domain %u: "fmt, (d)->domid, ## __VA_ARGS__)

#endif /* !DOMAIN_H_ */
