/*
 * Copyright (c) 2012 Citrix Systems, Inc.
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

/* client.c */
ICBINN *icb;
struct cmd *find_command (const char *name);
void parse_cmd (char *cmd_string);
int main (int argc, char *argv[]);
/* get.c */
int cmd_get (int argc, char *argv[]);
/* ls.c */
int cmd_ls (int argc, char *argv[]);
/* mkdir.c */
int cmd_mkdir (int argc, char *argv[]);
/* null.c */
int cmd_null (int argc, char *argv[]);
/* put.c */
int cmd_put (int argc, char *argv[]);
/* rename.c */
int cmd_rename (int argc, char *argv[]);
/* rm.c */
int cmd_rm (int argc, char *argv[]);
/* rmdir.c */
int cmd_rmdir (int argc, char *argv[]);
/* stat.c */
int cmd_stat (int argc, char *argv[]);
/* util.c */
char icb_type_to_char (int t);
