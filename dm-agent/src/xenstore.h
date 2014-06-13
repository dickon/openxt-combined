/*
 * Copyright (c) 2013 Citrix Systems, Inc.
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

#ifndef XENSTORE_H__
# define XENSTORE_H__

# include <xcxenstore/xcxenstore.h>
# include "dm-agent.h"

bool xenstore_dm_agent_create (s_dm_agent *dm_agent);
void xenstore_dm_agent_destroy (s_dm_agent *dm_agent, bool killall);

#endif /* !XENSTORE_H__ */
