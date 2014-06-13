/*
 * Copyright (c) 2013 Citrix Systems, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* server.c */
void init_rand(void);
void *icbinn_prot_null_1_svc(void *argp, struct svc_req *rqstp);
icbinn_prot_readdirres *icbinn_prot_readdir_1_svc(icbinn_prot_readdirargs *argp, struct svc_req *rqstp);
icbinn_prot_statres *icbinn_prot_stat_1_svc(icbinn_prot_path *argp, struct svc_req *rqstp);
icbinn_prot_openres *icbinn_prot_open_1_svc(icbinn_prot_openargs *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_lock_1_svc(icbinn_prot_lockargs *argp, struct svc_req *rqstp);
icbinn_prot_preadres *icbinn_prot_pread_1_svc(icbinn_prot_preadargs *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_ftruncate_1_svc(icbinn_prot_ftruncateargs *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_pwrite_1_svc(icbinn_prot_pwriteargs *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_close_1_svc(icbinn_prot_fd *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_mkdir_1_svc(icbinn_prot_path *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_rmdir_1_svc(icbinn_prot_path *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_rename_1_svc(icbinn_prot_renameargs *argp, struct svc_req *rqstp);
icbinn_prot_errnostat *icbinn_prot_unlink_1_svc(icbinn_prot_path *argp, struct svc_req *rqstp);
icbinn_prot_randres *icbinn_prot_rand_1_svc(icbinn_prot_randargs *argp, struct svc_req *rqstp);
/* svc_main.c */
int main(int argc, char *argv[]);
/* util.c */
void message(int flags, const char *file, const char *function, int line, const char *fmt, ...);
/* dbusrpc.c */
int get_icbinn_path(int domid, char *path_ptr, int buf_size, int path_index);
/* version.c */
char *libicbinn_get_version(void);
/* ../rpc/icbinn_prot_svc.c */
void icbinn_prot_program_1(struct svc_req *rqstp, register SVCXPRT *transp);
/* ../rpc/icbinn_prot_xdr.c */
bool_t xdr_icbinn_prot_path(XDR *xdrs, icbinn_prot_path *objp);
bool_t xdr_icbinn_prot_filename(XDR *xdrs, icbinn_prot_filename *objp);
bool_t xdr_icbinn_prot_fd(XDR *xdrs, icbinn_prot_fd *objp);
bool_t xdr_icbinn_prot_ltype(XDR *xdrs, icbinn_prot_ltype *objp);
bool_t xdr_icbinn_prot_type(XDR *xdrs, icbinn_prot_type *objp);
bool_t xdr_icbinn_prot_stat(XDR *xdrs, icbinn_prot_stat *objp);
bool_t xdr_icbinn_prot_readdirargs(XDR *xdrs, icbinn_prot_readdirargs *objp);
bool_t xdr_icbinn_prot_entry(XDR *xdrs, icbinn_prot_entry *objp);
bool_t xdr_icbinn_prot_dirlist(XDR *xdrs, icbinn_prot_dirlist *objp);
bool_t xdr_icbinn_prot_readdirres(XDR *xdrs, icbinn_prot_readdirres *objp);
bool_t xdr_icbinn_prot_openargs(XDR *xdrs, icbinn_prot_openargs *objp);
bool_t xdr_icbinn_prot_openokres(XDR *xdrs, icbinn_prot_openokres *objp);
bool_t xdr_icbinn_prot_openres(XDR *xdrs, icbinn_prot_openres *objp);
bool_t xdr_icbinn_prot_statokres(XDR *xdrs, icbinn_prot_statokres *objp);
bool_t xdr_icbinn_prot_statres(XDR *xdrs, icbinn_prot_statres *objp);
bool_t xdr_icbinn_prot_preadargs(XDR *xdrs, icbinn_prot_preadargs *objp);
bool_t xdr_icbinn_prot_preadokres(XDR *xdrs, icbinn_prot_preadokres *objp);
bool_t xdr_icbinn_prot_preadres(XDR *xdrs, icbinn_prot_preadres *objp);
bool_t xdr_icbinn_prot_pwriteargs(XDR *xdrs, icbinn_prot_pwriteargs *objp);
bool_t xdr_icbinn_prot_lockargs(XDR *xdrs, icbinn_prot_lockargs *objp);
bool_t xdr_icbinn_prot_renameargs(XDR *xdrs, icbinn_prot_renameargs *objp);
bool_t xdr_icbinn_prot_ftruncateargs(XDR *xdrs, icbinn_prot_ftruncateargs *objp);
bool_t xdr_icbinn_prot_randargs(XDR *xdrs, icbinn_prot_randargs *objp);
bool_t xdr_icbinn_prot_randokres(XDR *xdrs, icbinn_prot_randokres *objp);
bool_t xdr_icbinn_prot_randres(XDR *xdrs, icbinn_prot_randres *objp);
/* ../rpc/xdr_fixup.c */
bool_t xdr_quad_t(XDR *x, quad_t *q);
bool_t xdr_u_quad_t(XDR *x, u_quad_t *q);
