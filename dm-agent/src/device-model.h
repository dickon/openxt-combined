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

#ifndef DEVICE_MODEL_H_
# define DEVICE_MODEL_H_

# include "domain.h"
# include "list.h"
# include "types.h"

/* Type of device model */
enum devmodel_type
{
    DEVMODEL_INVALID,
    DEVMODEL_SPAWN,
    DEVMODEL_DMBUS,
};

/* Device model status */
enum devmodel_status
{
    DEVMODEL_STARTING,
    DEVMODEL_RUNNING,
    DEVMODEL_DIED,
    DEVMODEL_KILLED,
};

/* Describe a device model */
struct device_model
{
    const struct domain *domain;
    dmid_t dmid;
    /* Ops to handle the device model */
    const struct device_model_ops *ops;
    void *type_priv; /* Private data for the type */
    /* Path in xenstore */
    char *dmpath;

    /* Device model status */
    enum devmodel_status status;

    LIST_ENTRY (struct device_model) link;
};

/* Describe device model operations */
struct device_model_ops
{
    enum devmodel_type type;
    /* Prepare internal structure */
    bool (*init) (void);
    /* Create a new device model. The type is used for internal purpose */
    bool (*create) (struct device_model *devmodel, unsigned int type);
    /* Start the device model */
    bool (*start) (struct device_model *devmodel);
    /* Kill the device model, this function doesn't release memory */
    void (*kill) (struct device_model *devmodel);
    /* Destroy the device model */
    void (*destroy) (struct device_model *devmodel, bool killall);

    LIST_ENTRY (struct device_model_ops) link;
};

/* Initialize device models structure */
bool device_models_init (void);

/* Watch on xenstore if there is already some device models running */
bool device_models_create (struct domain *d);

/* Create the devmodel dmid for the domain d */
struct device_model *device_model_create (struct domain *d, dmid_t dmid);

void device_model_notify_destroy (struct device_model *devmodel);
void device_model_destroy (struct device_model *devmodel, bool killall);

struct device_model *device_model_by_dmid (struct domain *d, dmid_t dmid);

/* Change the status of the device model */
bool device_model_status (struct device_model *devmodel,
                          enum devmodel_status status);

void device_models_watch (const char *path, void *priv);

void register_device_model (struct device_model_ops *ops);

#define devmodel_init(devmodel_ops)                             \
static void __attribute__((constructor)) do_devmodel_init (void)\
{                                                               \
    register_device_model (&devmodel_ops);                      \
}

#define devmodel_warning(dm, fmt, ...)                      \
   warning ("Domain %u: Devmodel %u: "fmt,                  \
            (dm)->domain->domid,                            \
            (dm)->dmid, ## __VA_ARGS__)

#define devmodel_error(dm, fmt, ...)                        \
   error ("Domain %u: Devmodel %u: "fmt,                    \
          (dm)->domain->domid,                              \
          (dm)->dmid, ## __VA_ARGS__)

#define devmodel_info(dm, fmt, ...)                         \
   info ("Domain %u: Devmodel %u: "fmt,                     \
            (dm)->domain->domid,                            \
            (dm)->dmid, ## __VA_ARGS__)

#endif /* !DEVICE_MODEL_H_ */
