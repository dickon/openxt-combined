/*
 * Bootage - a parallel initscript launcher
 * a bit like poor man's upstart
 *
 * Thomas Horsten <thomas.horsten@citrix.com>
 * Copyright (C) 2010 Citrix Systems
 * 
 */

/*
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



/*
 * See README for overview and format of /etc/bootage.conf
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#define VERSION "1.0"

#ifdef BOOTAGE_TEST_MODE
# define BOOTAGE_DEBUG 0
# define BOOTAGE_STATS 0
#endif

/* Set to 1 for verbose output or 0 for normal operation */
#ifndef BOOTAGE_DEBUG
#define BOOTAGE_DEBUG 0
#endif

/* Set to 1 to track+display timing stats or 0 for no timing stats */
#ifndef BOOTAGE_STATS
#define BOOTAGE_STATS 1
#endif

#if BOOTAGE_DEBUG
# define dprintf(args...) fprintf(stderr, ##args)
#else
# ifdef BOOTAGE_TEST_MODE   // output is a bit hard to parse for automated tests
#  define dprintf(args...)
# else
#  define dprintf(args...) do { if (testmode) fprintf(stderr, ##args); } while(0)
# endif
#endif

#define MAX_BOOTSCRIPTS 256 /* Multiple of 32 assumed */

static int testmode = 0;
char *etc_base = ""; //will pre-pended to all the paths in test mode

enum { anytime=-1,     // start in runlevel, determine sequence automatically
	as_dependency=-2,  // this is a dependency of something else
	inactive=-3 };      // default. yet unknown if we have to start it at all

struct bootscript {
	char *name;
	char *target;
	int seq;
	int autodep;
	pid_t pid;
#if BOOTAGE_STATS
	struct timespec start, stop;
	double holdup_time;
#endif
};

static uint32_t dependencies[MAX_BOOTSCRIPTS][MAX_BOOTSCRIPTS/32];

static uint32_t runlist[MAX_BOOTSCRIPTS/32];

static struct bootscript bootscript[MAX_BOOTSCRIPTS];
static int bs_count = 0;
static char runlevel = '0';

static const char progname[] = "Bootage " VERSION;

static inline void oom(void *p)
{
	if (!p) {
		fprintf(stderr, "\e[1;31mFatal: Out of memory\e[0m\n");
		exit(1);
	}
}

static inline void bs_add_dep(struct bootscript *bs, struct bootscript *bsdep)
{
	int bsseq = bs-bootscript;
	int depseq = bsdep-bootscript;
	dependencies[bsseq][depseq/32] |= 1 << (depseq%32);
	//dprintf("bs_add_dep %d(%s) %d(%s) %d\n", bsseq, bs->name, depseq, bsdep->name, dependencies[bsseq][depseq/32]);
}

struct bootscript *bs_new(const char *name, char *target)
{
	struct bootscript *bs;
	if (bs_count + 1 >= MAX_BOOTSCRIPTS) {
		fprintf(stderr, "\e[1;31mFatal: Too many bootscripts\e[0m\n");
		exit(1);
	}
	bs = bootscript+bs_count;
	memset(bs, 0, sizeof(*bs));
	bs->name = malloc(strlen(name)+1);
	oom(bs->name);
	strcpy(bs->name, name);
	bs->target = malloc(strlen(target)+1);
	oom(bs->target);
	strcpy(bs->target, target);
	bs_count++;
	return bs;
}

struct bootscript *bs_find(const char *name)
{
	int c;
	for (c=0; c<bs_count; c++)
		if (!strcmp(bootscript[c].name, name))
			return bootscript+c;
	return NULL;
}

void auto_deps()
{
	int c, d;
	for (c=0; c<bs_count; c++) {
		if (bootscript[c].autodep) {
			/* Warn in test mode */
			dprintf("\e[1;33mWarning: %s doesn't have explicit dependencies set in bootage.conf!\n", bootscript[c].name);
			for (d=0; d<bs_count; d++) {
				if (c == d)
					continue;
				if (bootscript[d].seq > 0 &&
				    bootscript[d].seq < bootscript[c].seq)
					bs_add_dep(bootscript+c, bootscript+d);
			}
		}
	}
}

int isscheduled(c) {
	return runlist[c/32] & (1 << (c%32));
}

int isdepof(c, dep) {
	return dependencies[c][dep/32] & (1 << (dep%32));
}

void sched(int s, int* t) {
	runlist[s/32] |= (1 << (s%32));
	(*t)++;
}

int sched_with_deps(int s, int* t, int* recursion_depth) {
	int dep;

	if ((*recursion_depth) > bs_count) {
		fprintf(stderr, 
			"\e[1;31mFail: loop max depth detected for %s, "
			"skipping its dependencies\e[0m\n", bootscript[s].name);

		return -1;
	}

	if (!isscheduled(s)) {
		/* this is the only place where we schedule something meaning, if 
		   something has already been scheduled, its dependencies will be, too */

		sched(s, t);
		*recursion_depth += 1;

		for (dep=0; dep<bs_count; dep++) {
			if (isdepof(s, dep)) sched_with_deps(dep, t, recursion_depth);
		}
	}

	return 0;
}

static void dump_dep(int bsseq) {
	int c, d, v;
	for (c=0; c<MAX_BOOTSCRIPTS/32; c++) {
		v = dependencies[bsseq][c] & runlist[c/32];
		for (d=0; d<32; d++)
			if (v & (1 << d)) {
				dprintf("[%s]", bootscript[d].name);
			}
	}
}

void prepare_runlist()
{
	int c, t=0, recursion_depth, sched_reason;

	memset(runlist, 0, sizeof(runlist));

	for (c=0; c<bs_count; c++) {
		sched_reason = bootscript[c].seq;

		if ((inactive == sched_reason) || (as_dependency == sched_reason)) {
			// skip scripts that are inactive or only dependencies
			continue;
		}

		recursion_depth = 0;
		if (sched_with_deps(c, &t, &recursion_depth)) {
			fprintf(stderr, 
				"\e[1;31mFail: there seems to be a loop in the dependencies of %s!\e[0m\n",
				bootscript[c].name);

			dump_dep(c);
		}
			
	}
#ifndef BOOTAGE_TEST_MODE
	fprintf(stderr, "\e[1;34m%s: %d scripts to run\e[0m\n", progname, t);
#endif
}

void spawn(int bsseq)
{
	pid_t pid;
	pid = fork();
	int shellscript = 0;
	const char *t = bootscript[bsseq].target;
	static const char sh[] = "/bin/sh";


	if (pid == 0) {
#ifdef BOOTAGE_TEST_MODE
        printf("%s\n", bootscript[bsseq].name);
#endif
		if (strstr(t, ".sh"))
			shellscript = 1;
		if (!shellscript) {
			dprintf("execl(\"%s\", \"%s\", \"start\", NULL)\n",
				t, t);
			if (testmode)
				exit(0);
			execl(t, t, "start", NULL);
		} else {
			dprintf("execl(\"%s\", \"%s\", \"-c\", \"%s\", \"start\", NULL)\n",
				sh, sh, t);
			if (testmode)
				exit(0);
			execl(sh, sh, t, "start", NULL);
		}			
		exit(255);
	} else if (pid < 0) {
		fprintf(stderr, "\e[1;31mFatal: fork() failed for %s\e[0m", t);
		exit(1);
	}
	bootscript[bsseq].pid = pid;
#if BOOTAGE_STATS
	clock_gettime(CLOCK_MONOTONIC, &bootscript[bsseq].start);
#endif
}

#if BOOTAGE_STATS
static inline double clock_elapsed(struct timespec *stop, struct timespec *start)
{
	return ( stop->tv_sec + stop->tv_nsec / 1000000000.0 ) -
		( start->tv_sec + start->tv_nsec / 1000000000.0 );
}
#endif

void run_scripts()
{
	int c, d, n, t;
	int status;
	pid_t pid;
#if BOOTAGE_STATS
	struct timespec start, stop;
	/* Mask of the scripts that are currently blocking something */
	uint32_t blocklist[MAX_BOOTSCRIPTS/32];
#endif

	do {
		n=0;
		t=0;
#if BOOTAGE_STATS
		memset(blocklist, 0, sizeof(blocklist));
#endif
		for (c=0; c<bs_count; c++) {
			t |= runlist[c/32];
			if (runlist[c/32] & (1<<(c%32)) && bootscript[c].pid==0) {
				for (d=0; d<MAX_BOOTSCRIPTS/32; d++)
					if (dependencies[c][d] & runlist[d]) {
						dprintf("\e[1;37m[%s] blocked by: \e[0;33m",
							bootscript[c].name);
						dump_dep(c);
						dprintf("\e[0m\n");
#if BOOTAGE_STATS
						blocklist[d] |= dependencies[c][d] & runlist[d];
#endif
						break;
					}
				if (d==MAX_BOOTSCRIPTS/32) {
#ifndef BOOTAGE_TEST_MODE
					fprintf(stderr, "\e[1;32m[%s]\e[0m\n", bootscript[c].name);
#endif
					spawn(c);
					n++;
				}
			}
		}
		dprintf("t=%d n=%d\n", t, n);
		if (t == 0) {
#ifndef BOOTAGE_TEST_MODE
			fprintf(stderr, "\e[1;34mAll done\e[0m\n");
#endif
			return;
		}
		if (n == 0) {
			for (c=0; c<bs_count;c++)
				if (bootscript[c].pid != 0)
					break;
			if (c == bs_count) {
				fprintf(stderr, "\e[1;31mFatal: Nothing ready to run, circular dependencies?\e[0m\n");
				exit(1);
			}
		}
		do {
#if BOOTAGE_STATS
			clock_gettime(CLOCK_MONOTONIC, &start);
#endif
			pid = wait(&status);
			if (pid < 0) {
				if (errno == EAGAIN)
					continue;
				fprintf(stderr, "\e[1;31mFatal: wait() failed: %s\e[0m\n", strerror(errno));
				exit(1);
			}
			//dprintf("Pid %d done\n", pid);
#if BOOTAGE_STATS
			clock_gettime(CLOCK_MONOTONIC, &stop);
			for (c=0; c<bs_count; c++)
				if ((blocklist[c/32] & (1<<(c%32))) && bootscript[c].pid) {
					bootscript[c].holdup_time += clock_elapsed(&stop, &start);
				}
#endif

			for (c=0; c<bs_count; c++)
				if (bootscript[c].pid == pid) {
					runlist[c/32] &= ~(1 << (c%32));
#ifndef BOOTAGE_TEST_MODE
					dprintf("\e[1;36m[%s]\e[0m - done\n", bootscript[c].name);
#endif
					bootscript[c].pid = 0;
#if BOOTAGE_STATS
					memcpy(&bootscript[c].stop, &stop, sizeof(struct timespec));
#endif
					break;
				}
		} while (0);
	} while(t);
}

#if BOOTAGE_STATS
static int dump_comp(const void *a, const void *b)
{
	int n1, n2;
	n1 = *(const int *)a;
	n2 = *(const int *)b;
	if (bootscript[n1].holdup_time == bootscript[n2].holdup_time)
		return 0;
	return bootscript[n1].holdup_time > bootscript[n2].holdup_time ? -1 : 1;
}

void dump_stats()
{
	int c;
	int *order;
	order = malloc(bs_count * sizeof(int *));
	for (c=0; c<bs_count; c++)
		order[c] = c;
	qsort(order, bs_count, sizeof(int *), dump_comp);
	fprintf(stderr, "Bootage stats, worst sinners first: \e[1;32mruntime\e[0m \e[1;31mholdup time\e[0m\n");
	for (c=0; c<bs_count; c++) {
		if (bootscript[order[c]].seq != -1)
			fprintf(stderr, "\e[1;36m[%s]: \e[1;32m%2.03f \e[1;31m%2.03f\e[0m\n",
				bootscript[order[c]].name,
				clock_elapsed(&bootscript[order[c]].stop,
					      &bootscript[order[c]].start),
				bootscript[order[c]].holdup_time);
	}
}
#endif

/* Read /etc/rcX.d */
void read_rcd()
{
	DIR *d;
	char buf[256];
	struct dirent *de;
	struct bootscript *bs;
	
	snprintf(buf, sizeof(buf), "%s/etc/rc%c.d", etc_base, runlevel);
	dprintf("initscript dir: %s\n", buf);
	d = opendir(buf);
	if (!d) {
		fprintf(stderr, "\e[1;31mFatal: Couldn't open dir %s: %s\e[0m\n", buf, 
			strerror(errno));
		exit(1);
	}
	while ( (de = readdir(d)) ) {
		if (de->d_name[0] == 'S') {
			if (!isdigit(de->d_name[1]) ||
			    !isdigit(de->d_name[2]) ||
			    !de->d_name[3]) {
				fprintf(stderr, "\e[1;33mWarning: Malformed entry %s\e[0m\n", de->d_name);
				continue;
			}
			if (bs_find(de->d_name+3)) {
				fprintf(stderr, "\e[1;33mWarning: Script %s started more than once, not added again\n", de->d_name+3);
				continue;
			}
			snprintf(buf, sizeof(buf), "%s/etc/rc%c.d/%s", etc_base, runlevel, de->d_name);
			bs = bs_new(de->d_name+3, buf);
			bs->seq = (de->d_name[2]-'0') + 10*(de->d_name[1]-'0');
			bs->autodep = 1;
			//dprintf("Start %02d: %s\n", bs->seq, bs->name);
		}
	}
	closedir(d);
}

void read_cfg()
{
	FILE *f;
	char buf[1024];
	char tbuf[256];
	char fnbuf[256];
	char *p;
	int line = 0;
	struct bootscript *bs = NULL, *bsdep;
	static const char delim[] = " \t\r\n";

	snprintf(fnbuf, sizeof(fnbuf), "%s/etc/bootage.conf", etc_base);
	f = fopen(fnbuf, "r");
	if (!f) {
		dprintf("No config file\n");
		return;
	}
	while (fgets(buf, sizeof(buf), f)) {
		line++;
		p = strtok(buf, delim);
		if (!p)
			continue;
		if (!p[0] || p[0] == '#')
			continue;
		if (!strcmp(p, "script")) {
			p = strtok(NULL, delim);
			if (!p) {
				fprintf(stderr, "\e[1;31mFatal: Missing script name line %d\e[0m\n",
					line);
				exit(1);
			}
			bs = bs_find(p);
			if (!bs) {
				sprintf(tbuf, "%s/etc/init.d/%s", etc_base, p);
				bs = bs_new(p, tbuf);
				bs->seq = inactive;
			}
		} else if (!strcmp(p, "dep") || !strcmp(p, "depend")) {
			if (!bs) {
				fprintf(stderr, "\e[1;31mFatal: dep before script on line %d\e[0m\n",
					line);
				exit(1);
			}
			bs->autodep = 0;
			while ((p = strtok(NULL, delim))) {
				//dprintf("%s depends on %s\n",
				//	bs->name, p);
				bsdep = bs_find(p);
				if (!bsdep) {
					dprintf("Create %s which %s depends on\n", p, bs->name);
					sprintf(tbuf, "%s/etc/init.d/%s", etc_base, p);
					bsdep = bs_new(p, tbuf);
					bsdep->seq = as_dependency;
				}
				bs_add_dep(bs, bsdep);
			}
		} else if (!strcmp(p, "start")) {
			if (!bs) {
				fprintf(stderr, "\e[1;31mFatal: start before script on line %d\e[0m\n",
					line);
				exit(1);
			}
			while ((p = strtok(NULL, delim))) {
				if (index(p, runlevel)) {
					if (bs->seq < 0) {
						// was either 'anytime', 'as_dependency', 'inactive'
						// now it's clear we should start it. accordingly:
						bs->seq = anytime;
					}
					dprintf("Will start %s at this runlevel\n", bs->name);
				}
			}
		} else if (!strcmp(p, "block")) {
			if (!bs) {
				fprintf(stderr, "\e[1;31mFatal: block before script on line %d\e[0m\n",
					line);
				exit(1);
			}
			while ((p = strtok(NULL, delim))) {
				if (index(p, runlevel)) {
					bs->seq = inactive;
					dprintf("Will block %s at this runlevel\n", bs->name);
				}
			}
		} else if (!strcmp(p, "stop")) {
			continue;
		} else {
			fprintf(stderr, "\e[1;31mFatal: Unknown directive in config line %d: %s\e[0m\n",
				line, p);
			exit(1);
		}
	}
	fclose(f);
}

int main(int argc, char **argv)
{
	memset(dependencies, 0, sizeof(dependencies));
	memset(bootscript, 0, sizeof(bootscript));
	if (argc >= 3 && !strcmp(argv[2], "test")) {
		dprintf("Test mode\n");
		testmode = 1;
		if (argc == 4) {
		    etc_base = argv[3];
		}
		argc = 2;
	}
	if (argc != 2) {
		fprintf(stderr, "Usage: %s RUNLEVEL\n", argv[0]);
		exit(1);
	}
	if (strlen(argv[1]) != 1) {
		fprintf(stderr, "%s: RUNLEVEL must be a single character\n", argv[0]);
		exit(1);
	}
	runlevel = argv[1][0];
	if (runlevel < 1) {
		fprintf(stderr, "Usage: %s RUNLEVEL\n", argv[0]);
		exit(1);
	}
	if (argc == 3)
		testmode = 1;
	
	read_rcd();
	read_cfg();
	auto_deps();
	prepare_runlist();
	run_scripts();
#if BOOTAGE_STATS
	if (!testmode)
		dump_stats();
#endif
	exit(0);
}
