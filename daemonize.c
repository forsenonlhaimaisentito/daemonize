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

#ifndef DEVNULL
#define DEVNULL "/dev/null"
#endif /* DEVNULL */

/* 
 * FIXME: This will only work in GNU CPP, because of
 * the nonstandard ## comma deletion feature
 */
#define errx(fmt, ...)  do {						\
		fprintf(stderr, "%s: ", name);				\
		if (strlen(fmt)){							\
			fprintf(stderr, fmt, ##__VA_ARGS__);	\
			fprintf(stderr, ": ");					\
		}											\
		fprintf(stderr, "%s\n", strerror(errno));	\
		exit(EXIT_FAILURE);							\
	} while (0)										

static void print_usage();

static char *name;

int main(int argc, char **argv){
	int fd;
	int old_errno;
	pid_t child;
	struct sigaction sa;

	name = argv[0];
	
	if (argc < 2){
		print_usage();
		return EXIT_SUCCESS;
	}
	
	if ((child = fork()) == 0){
		/* Ignore SIGHUP */
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigaction(SIGHUP, &sa, NULL);

		/* Create a new session */
		if (setsid() < 0){
			errx("can't create a new session");
		}

		/* Redirect I/O to /dev/null open for writing */
		if ((fd = open(DEVNULL, O_WRONLY)) < 0){
			errx("can't open %s", DEVNULL);
		}

		if (isatty(STDIN_FILENO)){
			dup2(fd, STDIN_FILENO);
		}

		if (isatty(STDOUT_FILENO)){
			dup2(fd, STDOUT_FILENO);
		}

		if (isatty(STDERR_FILENO)){
			dup2(STDOUT_FILENO, STDERR_FILENO);
		}

		/* Execute target command */
		if (execvp(argv[1], &argv[1]) < 0){
			/* close() may alter errno */
			old_errno = errno;
			close(fd);
			errno = old_errno;
			errx("can't exec");
		}
	} else if (child < 0){
		errx("fork");
	}
	
	return EXIT_SUCCESS;
}

static void print_usage(){
	fprintf(stderr, "Usage: %s COMMAND\n", name);
}
