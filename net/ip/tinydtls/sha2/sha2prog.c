/*
 * FILE:	sha2prog.c
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 * 
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sha2prog.c,v 1.1 2001/11/08 00:02:11 adg Exp adg $
 */

#include <stdio.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "sha2.h"

void usage(char *prog, char *msg) {
	fprintf(stderr, "%s\nUsage:\t%s [options] [<file>]\nOptions:\n\t-256\tGenerate SHA-256 hash\n\t-384\tGenerate SHA-284 hash\n\t-512\tGenerate SHA-512 hash\n\t-ALL\tGenerate all three hashes\n\t-q\tQuiet mode - only output hexadecimal hashes, one per line\n\n", msg, prog);
	exit(-1);
}

#define BUFLEN 16384

int main(int argc, char **argv) {
	int		kl, l, fd, ac;
	int		quiet = 0, hash = 0;
	char		*av, *file = (char*)0;
	FILE		*IN = (FILE*)0;
	SHA256_CTX	ctx256;
	SHA384_CTX	ctx384;
	SHA512_CTX	ctx512;
	unsigned char	buf[BUFLEN];

	SHA256_Init(&ctx256);
	SHA384_Init(&ctx384);
	SHA512_Init(&ctx512);

	/* Read data from STDIN by default */
	fd = fileno(stdin);

	ac = 1;
	while (ac < argc) {
		if (*argv[ac] == '-') {
			av = argv[ac] + 1;
			if (!strcmp(av, "q")) {
				quiet = 1;
			} else if (!strcmp(av, "256")) {
				hash |= 1;
			} else if (!strcmp(av, "384")) {
				hash |= 2;
			} else if (!strcmp(av, "512")) {
				hash |= 4;
			} else if (!strcmp(av, "ALL")) {
				hash = 7;
			} else {
				usage(argv[0], "Invalid option.");
			}
			ac++;
		} else {
			file = argv[ac++];
			if (ac != argc) {
				usage(argv[0], "Too many arguments.");
			}
			if ((IN = fopen(file, "r")) == NULL) {
				perror(argv[0]);
				exit(-1);
			}
			fd = fileno(IN);
		}
	}
	if (hash == 0)
		hash = 7;	/* Default to ALL */

	kl = 0;
	while ((l = read(fd,buf,BUFLEN)) > 0) {
		kl += l;
		SHA256_Update(&ctx256, (unsigned char*)buf, l);
		SHA384_Update(&ctx384, (unsigned char*)buf, l);
		SHA512_Update(&ctx512, (unsigned char*)buf, l);
	}
	if (file) {
		fclose(IN);
	}

	if (hash & 1) {
		SHA256_End(&ctx256, buf);
		if (!quiet)
			printf("SHA-256 (%s) = ", file);
		printf("%s\n", buf);
	}
	if (hash & 2) {
		SHA384_End(&ctx384, buf);
		if (!quiet)
			printf("SHA-384 (%s) = ", file);
		printf("%s\n", buf);
	}
	if (hash & 4) {
		SHA512_End(&ctx512, buf);
		if (!quiet)
			printf("SHA-512 (%s) = ", file);
		printf("%s\n", buf);
	}

	return 1;
}

