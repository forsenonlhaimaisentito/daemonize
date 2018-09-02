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

// The permissions assigned to the redirection files
#define RED_PERMS S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH

#define OPT_OUTRED 'o'
#define OPT_ERRRED 'e'
#define OPT_HELP 'h'
#define OPT_VERSION 'v'

/* 
 * FIXME: This will only work in GNU CPP, because of
 * the nonstandard ## comma deletion feature
 */
#define errx(fmt, ...)  do {						\
		fprintf(stderr, "%s: ", name);				\
		if (strlen(fmt)) {							\
			fprintf(stderr, fmt, ##__VA_ARGS__);	\
			fprintf(stderr, ": ");					\
		}											\
		fprintf(stderr, "%s\n", strerror(errno));	\
		exit(EXIT_FAILURE);							\
	} while (0)										

static char *name;
static char *version = "v0.1";

static void print_usage(char **, struct option const *);


int main(int argc, char **argv) {
	int nullfd, outredfd, errredfd;
	int old_errno;
	pid_t child;
	struct sigaction sa;

	int retopt = -1;
	struct {
		char command[NAME_MAX];
		char outred[NAME_MAX];
		char errred[NAME_MAX];

		char *const *arguments;
	} pargs = {"", DEVNULL, DEVNULL, NULL};

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
				strncpy(pargs.outred, optarg, NAME_MAX);
				break;
			case OPT_ERRRED:
				strncpy(pargs.errred, optarg, NAME_MAX);
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

	/*
	 * Consume the remaining non-option arguments from getopt_long().
	 * TO-DO: here is what the man-page reads about argv:
	 * «[...] The first argument, by convention, should point to the filename
	 * associated with the file being executed.  The array of pointers must be
	 * terminated by a null pointer. [...]»
	 * So shall `pargs.arguments` actually point to the name of the file, and
	 * not just the argument(s) coming next?
	 */
	// Copy the command filename from the first remaining non-option argument
	if (optind < argc) {
		strncpy(pargs.command, argv[optind], NAME_MAX);
	}
	// If other non-option argument values are left besides the filename, we
	// add them to pargs.arguments (whilst also including the filename value,
	// hence pargs.command == pargs.arguments if the if-condition holds true)
	if (optind + 1 < argc) {
		pargs.arguments = argv + optind;
	}

	if ((child = fork()) == 0) {
		/* Ignore SIGHUP */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigaction(SIGHUP, &sa, NULL);

		/* Create a new session */
		if (setsid() < 0) {
			errx("can't create a new session");
		}

		/* Redirect input to /dev/null open for writing */
		if ((nullfd = open(DEVNULL, O_WRONLY)) < 0)
			errx("can't open %s", DEVNULL);
		if (isatty(STDIN_FILENO))
			dup2(nullfd, STDIN_FILENO);

		if ((outredfd = open(pargs.outred, O_WRONLY | O_CREAT, RED_PERMS)) < 0)
			errx("can't open %s", pargs.outred);
		if (isatty(STDOUT_FILENO))
			dup2(outredfd, STDOUT_FILENO);

		if ((errredfd = open(pargs.errred, O_WRONLY | O_CREAT, RED_PERMS)) < 0)
			errx("can't open %s", pargs.errred);
		if (isatty(STDERR_FILENO))
			dup2(errredfd, STDERR_FILENO);

		/* Execute target command */
		if (execvp(pargs.command, pargs.arguments) < 0) {
			/* close() may alter errno */
			old_errno = errno;

			close(nullfd);
			close(outredfd);
			close(errredfd);

			errno = old_errno;
			errx("can't exec");
		}
	} else if (child < 0) {
		errx("fork");
	}
	
	return EXIT_SUCCESS;
}


static void print_usage(char **argv, struct option const *longopts) {
	printf(
		"%s: [-%c --%s <filename>] [-%c --%s <filename>] <CMD>\n",
		argv[0], longopts[0].val, longopts[0].name, longopts[1].val,
		longopts[1].name
	);
	printf(
		"%s: [-%c --%s]\n", argv[0], longopts[2].val, longopts[2].name
	);
	printf(
		"%s: [-%c --%s]\n", argv[0], longopts[3].val, longopts[3].name
	);
}
