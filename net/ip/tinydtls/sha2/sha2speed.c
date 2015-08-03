/*
 * FILE:	sha2speed.c
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
 * $Id: sha2speed.c,v 1.1 2001/11/08 00:02:23 adg Exp adg $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "sha2.h"

#define BUFSIZE	16384

void usage(char *prog) {
	fprintf(stderr, "Usage:\t%s [<num-of-bytes>] [<num-of-loops>] [<fill-byte>]\n", prog);
	exit(-1);
}

void printspeed(char *caption, unsigned long bytes, double time) {
	if (bytes / 1073741824UL > 0) {
                printf("%s %.4f sec (%.3f GBps)\n", caption, time, (double)bytes/1073741824UL/time);
        } else if (bytes / 1048576 > 0) {
                printf("%s %.4f (%.3f MBps)\n", caption, time, (double)bytes/1048576/time);
        } else if (bytes / 1024 > 0) {
                printf("%s %.4f (%.3f KBps)\n", caption, time, (double)bytes/1024/time);
        } else {
		printf("%s %.4f (%f Bps)\n", caption, time, (double)bytes/time);
	}
}


int main(int argc, char **argv) {
	SHA256_CTX	c256;
	SHA384_CTX	c384;
	SHA512_CTX	c512;
	char		buf[BUFSIZE];
	char		md[SHA512_DIGEST_STRING_LENGTH];
	int		bytes, blocks, rep, i, j;
	struct timeval	start, end;
	double		t, ave256, ave384, ave512;
	double		best256, best384, best512;

	if (argc > 4) {
		usage(argv[0]);
	}

	/* Default to 1024 16K blocks (16 MB) */
	bytes = 1024 * 1024 * 16;
	if (argc > 1) {
		blocks = atoi(argv[1]);
	}
	blocks = bytes / BUFSIZE;

	/* Default to 10 repetitions */
	rep = 10;
	if (argc > 2) {
		rep = atoi(argv[2]);
	}

	/* Set up the input data */
	if (argc > 3) {
		memset(buf, atoi(argv[2]), BUFSIZE);
	} else {
		memset(buf, 0xb7, BUFSIZE);
	}

	ave256 = ave384 = ave512 = 0;
	best256 = best384 = best512 = 100000;
	for (i = 0; i < rep; i++) {
		SHA256_Init(&c256);
		SHA384_Init(&c384);
		SHA512_Init(&c512);
	
		gettimeofday(&start, (struct timezone*)0);
		for (j = 0; j < blocks; j++) {
			SHA256_Update(&c256, (unsigned char*)buf, BUFSIZE);
		}
		if (bytes % BUFSIZE) {
			SHA256_Update(&c256, (unsigned char*)buf, bytes % BUFSIZE);
		}
		SHA256_End(&c256, md);
		gettimeofday(&end, (struct timezone*)0);
		t = ((end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec)) / 1000000.0;
		ave256 += t;
		if (t < best256) {
			best256 = t;
		}
		printf("SHA-256[%d] (%.4f/%.4f/%.4f seconds) = 0x%s\n", i+1, t, ave256/(i+1), best256, md);

		gettimeofday(&start, (struct timezone*)0);
		for (j = 0; j < blocks; j++) {
			SHA384_Update(&c384, (unsigned char*)buf, BUFSIZE);
		}
		if (bytes % BUFSIZE) {
			SHA384_Update(&c384, (unsigned char*)buf, bytes % BUFSIZE);
		}
		SHA384_End(&c384, md);
		gettimeofday(&end, (struct timezone*)0);
		t = ((end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec)) / 1000000.0;
		ave384 += t;
		if (t < best384) {
			best384 = t;
		}
		printf("SHA-384[%d] (%.4f/%.4f/%.4f seconds) = 0x%s\n", i+1, t, ave384/(i+1), best384, md);

		gettimeofday(&start, (struct timezone*)0);
		for (j = 0; j < blocks; j++) {
			SHA512_Update(&c512, (unsigned char*)buf, BUFSIZE);
		}
		if (bytes % BUFSIZE) {
			SHA512_Update(&c512, (unsigned char*)buf, bytes % BUFSIZE);
		}
		SHA512_End(&c512, md);
		gettimeofday(&end, (struct timezone*)0);
		t = ((end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec)) / 1000000.0;
		ave512 += t;
		if (t < best512) {
			best512 = t;
		}
		printf("SHA-512[%d] (%.4f/%.4f/%.4f seconds) = 0x%s\n", i+1, t, ave512/(i+1), best512, md);
	}
	ave256 /= rep;
	ave384 /= rep;
	ave512 /= rep;
	printf("\nTEST RESULTS SUMMARY:\nTEST REPETITIONS: %d\n", rep);
	if (bytes / 1073741824UL > 0) {
		printf("TEST SET SIZE: %.3f GB\n", (double)bytes/1073741824UL);
	} else if (bytes / 1048576 > 0) {
		printf("TEST SET SIZE: %.3f MB\n", (double)bytes/1048576);
	} else if (bytes /1024 > 0) {
		printf("TEST SET SIZE: %.3f KB\n", (double)bytes/1024);
	} else {
		printf("TEST SET SIZE: %d B\n", bytes);
	}
	printspeed("SHA-256 average:", bytes, ave256);
	printspeed("SHA-256 best:   ", bytes, best256);
	printspeed("SHA-384 average:", bytes, ave384);
	printspeed("SHA-384 best:   ", bytes, best384);
	printspeed("SHA-512 average:", bytes, ave512);
	printspeed("SHA-512 best:   ", bytes, best512);

	return 1;
}

