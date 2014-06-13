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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static ssize_t exhaust(int fd, char *buf, int buf_sz)
{
    ssize_t n = 0;
    for (;;) {
        int c;
        c = read(fd, buf+n, buf_sz-n);
        if (c <= 0)
            return n;
        n += c;
    }
}

int runcmd(char *path, char **argv, char *stdout, ssize_t *p_stdout_sz)
{
    pid_t pid;
    int pipefd[2];

    if (pipe(pipefd) == -1)
        return -1;

    pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        int maxfds = sysconf(_SC_OPEN_MAX);
        int i;
        dup2(pipefd[1], STDOUT_FILENO);
        for (i = 0; i < maxfds; ++i)
            if (i != STDOUT_FILENO)
                close(i);
        execv(path, argv);
        exit(1);
    } else {
        ssize_t n;
        int status;
        close(pipefd[1]);
        n = stdout ? exhaust(pipefd[0], stdout, *p_stdout_sz) : 0;
        close(pipefd[0]);
        if (n >= 0 && p_stdout_sz)
            *p_stdout_sz = n;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}
