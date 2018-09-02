/*
 * Copyright (c) 2016 forsenonlhaimaisentito
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>


#ifndef DEVNULL
#define DEVNULL "/dev/null"
#endif /* DEVNULL */

/* The permissions assigned to the redirection files */
#define RED_PERMS S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH

#define OPT_OUTRED 'o'
#define OPT_ERRRED 'e'
#define OPT_HELP 'h'
#define OPT_VERSION 'v'

/* 
 * FIXME: This will only work in GNU CPP, because of
 * the nonstandard ## comma deletion feature
 */
#define errx(fmt, ...)  do { \
		fprintf(stderr, "%s: ", name); \
		if (strlen(fmt)) { \
			fprintf(stderr, fmt, ##__VA_ARGS__); \
			fprintf(stderr, ": "); \
		} \
		fprintf(stderr, "%s\n", strerror(errno)); \
		exit(EXIT_FAILURE); \
	} while (0)										

static char *name;
static const char *version = "v0.1";

static void print_usage(char **, struct option const *);
static void redirect_fd(int, const char *, int);


int main(int argc, char **argv) {
	pid_t child;
	struct sigaction sa;

	int retopt = -1;
	struct {
		char command[PATH_MAX];
		char outred[PATH_MAX];
		char errred[PATH_MAX];

		char *const *arguments;
	} pargs = {"", "", "", NULL};

	int optindex;
	char const *shortopts = "+ h v o: e:";
	const struct option longopts[] = {
		{"out", required_argument, NULL, OPT_OUTRED},
		{"err", required_argument, NULL, OPT_ERRRED},
		{"help", no_argument, NULL, OPT_HELP},
		{"version", no_argument, NULL, OPT_VERSION},
	};

	name = argv[0];

	do {
		retopt = getopt_long(argc, argv, shortopts, longopts, &optindex);

		switch (retopt) {
			case OPT_OUTRED:
				strncpy(pargs.outred, optarg, PATH_MAX);
				break;
			case OPT_ERRRED:
				strncpy(pargs.errred, optarg, PATH_MAX);
				break;
			case OPT_HELP:
				print_usage(argv, longopts);
				exit(EXIT_SUCCESS);
			case OPT_VERSION:
				puts(version);
				exit(EXIT_SUCCESS);
			case -1:
			default:
				break;
		}
	} while (retopt > -1);
	
	/* Consume the remaining non-option arguments from getopt_long() */
	strncpy(pargs.command, argv[optind], NAME_MAX);
	/* If other non-option argument values are left besides the filename, we
	 * add them to pargs.arguments (whilst also including the filename value,
	 * hence pargs.command == pargs.arguments if the if-condition holds true)
	 */
	pargs.arguments = argv + optind;

	if ((child = fork()) == 0) {
		/* Ignore SIGHUP */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigaction(SIGHUP, &sa, NULL);

		/* Create a new session */
		if (setsid() < 0) {
			errx("can't create a new session");
		}

		/* Keep a reference to the original stderr for error reporting */
		stderr = fdopen(dup(STDERR_FILENO), "w+");

		redirect_fd(STDIN_FILENO, NULL, O_RDONLY);
		redirect_fd(STDOUT_FILENO, pargs.outred, O_WRONLY | O_CREAT);
		redirect_fd(STDERR_FILENO, pargs.errred, O_WRONLY | O_CREAT);

		/* Execute target command */
		if (execvp(pargs.command, pargs.arguments) < 0) {
			errx("can't exec");
		}
	} else if (child < 0) {
		errx("fork");
	}
	
	return EXIT_SUCCESS;
}


static void print_usage(char **argv, struct option const *longopts) {
	fprintf(stderr,
		"%s: [-%c --%s <filename>] [-%c --%s <filename>] <CMD>\n",
		argv[0], longopts[0].val, longopts[0].name, longopts[1].val,
		longopts[1].name
	);
	fprintf(stderr,
		"%s: [-%c --%s]\n", argv[0], longopts[2].val, longopts[2].name
	);
	fprintf(stderr,
		"%s: [-%c --%s]\n", argv[0], longopts[3].val, longopts[3].name
	);
}

/* If target is not NULL or an empty string, this function
 * redirects fd to it, otherwise, if fd is a TTY, 
 * it redirects fd to DEVNULL.
 */
static void redirect_fd(int fd, const char *target, int flags) {
	int target_fd;

	if (target != NULL && strlen(target)){
		if ((target_fd = open(target, flags, RED_PERMS)) < 0)
			errx("can't open %s", target);
	} else if (isatty(fd)){
		if ((target_fd = open(DEVNULL, flags, RED_PERMS)) < 0)
			errx("can't open %s", DEVNULL);
	} else {
		return;
	}

	dup2(target_fd, fd);
}
