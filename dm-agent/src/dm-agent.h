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

#ifndef DM_AGENT_H_
# define DM_AGENT_H_

# include "domain.h"
# include "types.h"

typedef struct dm_agent
{
    /* Where do I run */
    domid_t domid;
    /* Target domid */
    domid_t target_domid;
    /* Xenstore DM agent path */
    char *dmapath;
    struct event_base *ev_base;
} s_dm_agent;

const char *dm_agent_get_path (void);
struct event_base *dm_agent_get_ev_base (void);
void dm_agent_destroy (bool killall);
bool dm_agent_in_stubdom (void);
domid_t dm_agent_get_target (void);

#endif /* !DM_AGENT_H_ */
