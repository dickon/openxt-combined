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

/*
 * Sample client program.
 */

#include "project.h"

ICBINN *icb;

struct cmd
{
  const char *name;
  int (*handler) (int, char **);
  int nargs;
  char *help;
};


static int cmd_help (int argc, char *argv[]);
static int cmd_quit (int argc, char *argv[]);

static struct cmd cmd_tab[] = {
  {"get", cmd_get, 3,
   "get pathspec local-pathspec    - fetches a file from the server\n"},
  {"help", cmd_help, 1, "help                           - shows this text\n"},
  {"ls", cmd_ls, 2, "ls pathspec                    - list a directory\n"},
  {"mkdir", cmd_mkdir, 2,
   "mkdir pathspec                 - makes a directory\n"},
  {"null", cmd_null, 1, "null                           - does nothing\n"},
  {"put", cmd_put, 3,
   "put locapathspec pathspec      - sends a file to the server\n"},
  {"quit", cmd_quit, 1, "quit                           - quit\n"},
  {"rename", cmd_rename, 3,
   "rename oldpathspec newpathspec - renames an object\n"},
  {"rm", cmd_rm, 2, "rm pathspec                    - deletes a file\n"},
  {"rmdir", cmd_rmdir, 2,
   "rmdir pathspec                 - removes an empty directory\n"},
  {"stat", cmd_stat, 2, "stat pathspec                  - stats an object\n"},
  {NULL, NULL, 0, NULL},
};


static int
cmd_quit (int argc, char *argv[])
{
  icbinn_clnt_destroy (icb);
  exit (0);
}

static int
cmd_help (int argc, char *argv[])
{
  struct cmd *c;

  for (c = cmd_tab; c->name; c++)
    {
      fputs (c->help, stdout);
    }

  return 0;
}


/*Find the best match for a command */

struct cmd *
find_command (const char *name)
{
  const char *cptr;
  const char *nptr;
  struct cmd *c, *best = NULL;

  int most_matched = 0;
  int num_matched = 0;

  for (c = cmd_tab; c->name; c++)
    {

      for (cptr = c->name, nptr = name; *cptr == *nptr; cptr++, nptr++)
        {
          if (!*cptr)
            return c;
        }

      if (*nptr)
        continue;

      if (nptr - name > most_matched)
        {
          most_matched = nptr - name;
          num_matched = 1;
          best = c;
        }
      else if (nptr - name == most_matched)
        {
          num_matched++;
        }
    }

  switch (num_matched)
    {
    case 0:
      printf ("?Invalid command\n");
      break;
    case 1:
      return best;
    default:
      printf ("?Ambiguous command\n");
      break;
    }


  return NULL;
}


/*FIXME doesn't handle quoting spaces or escapes - sucks if you have a space in a filename*/

void
parse_cmd (char *cmd_string)
{
  char *argv[32];
  char **argvp = argv;
  int argc = 0;
  struct cmd *cmd;
  int ret;

  char *ptr;

  for (ptr = cmd_string; *ptr; ptr++)
    {
      while (isspace (*ptr))
        ptr++;
      if (!*ptr)
        break;
      *(argvp++) = ptr;
      argc++;
      while (*ptr && !isspace (*ptr))
        ptr++;
      if (!*ptr)
        break;
      *ptr = 0;
    }
  *(argvp++) = ptr;


  if (!argc)
    return;

  cmd = find_command (argv[0]);
  if (!cmd)
    return;


  if (cmd->nargs != argc)
    {
      printf ("Incorrect number of commands found %d, expected %d.\n", argc,
              cmd->nargs);
      fputs (cmd->help, stdout);
      return;
    }

  ret = cmd->handler (argc, argv);

  if (ret)
    {
      fprintf (stderr, "Command returns an error\n");
    }
  else
    {
      printf ("Command succeeded\n");
    }

}

static void
usage (char *name)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "%s {-d domid|-h host} [-p port] [-c cmd]\n", name);
  exit (1);
}


int
main (int argc, char *argv[])
{
  static char line[1024];
  int c;
  int domid = 0;
  int port = 0;
  char *host = NULL;
  char *cmd = NULL;
  int dflag = 0, hflag = 0;



  while ((c = getopt (argc, argv, "d:p:h:c:")) != EOF)
    {
      switch (c)
        {
        case 'd':
          domid = atoi (optarg);
          dflag++;
          break;
        case 'p':
          port = atoi (optarg);
          break;
        case 'h':
          host = optarg;
          hflag++;
          break;
        case 'c':
          cmd = optarg;
          break;
        default:
          usage (argv[0]);
        }
    }

  if (!dflag ^ !!hflag)
    usage (argv[0]);

  if (dflag)
    icb = icbinn_clnt_create_v4v (domid, port);
  else
    icb = icbinn_clnt_create_tcp (host, port);

  if (!icb)
    {
      fprintf (stderr, "failed to connect to server\n");
      exit (1);
    }

  if (cmd)
    {
      strncpy (line, cmd, sizeof (line) - 1);
      line[sizeof (line) - 1] = 0;
      parse_cmd (line);
    }
  else
    {
      for (;;)
        {
          fputs ("icbinn> ", stdout);
          if (fgets (line, sizeof (line), stdin) == 0)
            {
              if (feof (stdin))
                break;
              continue;
            }

          if (!*line)
            continue;
          if (*line == '\n')
            continue;

          parse_cmd (line);
        }
    }


  icbinn_clnt_destroy (icb);

  return 0;
}
