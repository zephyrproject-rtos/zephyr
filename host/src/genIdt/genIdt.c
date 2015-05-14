/* genIdt.c - Create static IDT */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
Creates a static IA-32 Interrupt Descriptor Table (IDT).

This program expects to be invoked as follows:
    genIdt <file name> <number of interrupt vectors>
All parameters are required.

<file name> is assumed to be a binary file containing the intList section from
the VxMicro ELF image (microkernel.elf, nanokernel.elf, etc.)

<number of interrupt vectors> is the same as CONFIG_IDT_NUM_VECTORS.

No help on usage is provided as it is expected that this program is invoked
from within the VxMicro build system during the link stage.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vxmicro/version.h>

/* These come from the shared directory */
#include "segselect.h"
#include "idtEnt.h"

/* Define the following macro to see a verbose or debug output from the tool */
/*#define DEBUG_GENIDT*/

#ifdef DEBUG_GENIDT
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if !defined(WINDOWS)
  #define O_BINARY 0
#endif

static void get_exec_name(char *pathname);
static void usage(const int len);
static void get_options(int argc, char *argv[]);
static void open_files(void);
static void close_files(void);
static void genIdt(void);
static void clean_exit(const int exit_code);

typedef struct s_isrList {
	void *fnc;
	unsigned int dpl;
} ISR_LIST;

static ISR_LIST idt[256];

enum {
	IFILE=0,             /* input file */
	OFILE,               /* output file */
	NUSERFILES,          /* number of user-provided file names */
	EXECFILE=NUSERFILES, /* for name of executable */
	NFILES               /* total number of files open */
};
enum { SHORT_USAGE, LONG_USAGE };

static int fds[NUSERFILES] = {-1, -1};
static char *filenames[NFILES];
static unsigned int numVecs = (unsigned int)-1;
static struct version version = {KERNEL_VERSION, 1, 1, 0};

int main(int argc, char *argv[])
{
	get_exec_name(argv[0]);
	get_options(argc, argv); /* may exit */
	open_files();	     /* may exit */
	genIdt();
	close_files();
	return 0;
}

static void get_options(int argc, char *argv[])
{
	char *endptr;
	int ii, opt;

	while ((opt = getopt(argc, argv, "hi:o:n:v")) != -1) {
		switch(opt) {
		case 'i':
			filenames[IFILE] = optarg;
			break;
		case 'o':
			{
			filenames[OFILE] = optarg;
			break;
			}
		case 'h':
			usage(LONG_USAGE);
			exit(0);
		case 'n':
			numVecs = (unsigned int)strtoul(optarg, &endptr, 10);
			if (*optarg == '\0' || *endptr != '\0') {
				usage(SHORT_USAGE);
				exit(-1);
				}
			break;
		case 'v':
			show_version(filenames[EXECFILE], &version);
			exit(0);
		default:
			usage(SHORT_USAGE);
			exit(-1);
	    }
	}

	if ((unsigned int)-1 == numVecs) {
		usage(SHORT_USAGE);
		exit(-1);
	}

	for(ii = IFILE; ii < NUSERFILES; ii++) {
		if (!filenames[ii]) {
			usage(SHORT_USAGE);
			exit(-1);
		}
	}
}

static void get_exec_name(char *pathname)
{
	int end = strlen(pathname)-1;

	while(end != -1) {
		#if defined(WINDOWS) /* Might have both slashes in path */
		if (pathname[end] == '/' || pathname[end] == '\\')
		#else
		if (pathname[end] == '/')
		#endif
		{
			if (0 == end || pathname[end-1] != '\\') {
				++end;
				break;
			}
		}
		--end;
	}
	filenames[EXECFILE] = &pathname[end];
}

static void open_files(void)
{
	int ii;

	fds[IFILE] = open(filenames[IFILE], O_RDONLY|O_BINARY);
	fds[OFILE] = open(filenames[OFILE], O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,
		                                S_IWUSR|S_IRUSR);
	for(ii = 0; ii < NUSERFILES; ii++) {
		int invalid = fds[ii] == -1;
		if (invalid) {
			char *invalid = filenames[ii];
			fprintf(stderr, "invalid file %s\n", invalid);
			for(--ii; ii >= 0; ii--) {
				close(fds[ii]);
			}
			exit(-1);
		}
	}
}

static void genIdt(void)
{
	unsigned int i;
	unsigned int size;
	void *spurAddr;
	void *spurNoErrAddr;

    /*
     * First find the address of the spurious interrupt handlers. They are the
     * contained in the first 8 bytes of the input file.
     */
	if (read(fds[IFILE], &spurAddr, 4) < 4) {
		goto readError;
	}

	if (read(fds[IFILE], &spurNoErrAddr, 4) < 4) {
		goto readError;
	}

	PRINTF("Spurious int handlers found at %p and %p\n",
		    spurAddr, spurNoErrAddr);

	/* Initially fill in the IDT array with the spurious handlers */
	for (i = 0; i < numVecs; i++) {
		if ((((1 << i) & _EXC_ERROR_CODE_FAULTS)) && (i < 32)) {
			idt[i].fnc = spurAddr;
		}
		else {
			idt[i].fnc = spurNoErrAddr;
		}
	}

    /*
     * Now parse the rest of the input file for the other ISRs. The number of
     * entries is the next 4 bytes
     */

	if (read(fds[IFILE], &size, 4) < 4) {
		goto readError;
	}

	PRINTF("There are %d ISR(s)\n", size);

	if (size > numVecs) {
		fprintf(stderr, "Too many ISRs found. Got %u. Expected less than %u."
			             " Malformed input file?\n", size, numVecs);
		clean_exit(-1);
	}

	while (size--) {
		void *addr;
		unsigned int vec;
		unsigned int dpl;

		/* Get address */
		if (read(fds[IFILE], &addr, 4) < 4) {
			goto readError;
		}
		/* Get vector */
		if (read(fds[IFILE], &vec, 4) < 4) {
			goto readError;
		}
		/* Get dpl */
		if (read(fds[IFILE], &dpl, 4) < 4) {
			goto readError;
		}

		PRINTF("ISR @ %p on Vector %d: dpl %d\n", addr, vec, dpl);
		idt[vec].fnc = addr;
		idt[vec].dpl = dpl;
	}

    /*
     * We now have the address of all ISR stub/functions captured in idt[].
     * Now construct the actual idt.
     */

	for (i = 0; i < numVecs; i++) {
		unsigned long long idtEnt;

		 _IdtEntCreate(&idtEnt, idt[i].fnc, idt[i].dpl);

		write(fds[OFILE], &idtEnt, 8);
	}

	return;

readError:
	fprintf(stderr, "Error occured while reading input file. Aborting...\n");
	clean_exit(-1);
}

static void close_files(void)
{
	int ii;

	for(ii = 0; ii < NUSERFILES; ii++) {
		close(fds[ii]);
	}
}

static void clean_exit(const int exit_code)
{
	close_files();
	exit(exit_code);
}

static void usage(const int len)
{
	fprintf(stderr, "\n%s -i <input file> -n <n>\n", filenames[EXECFILE]);

	if (len == SHORT_USAGE) {
		return;
	}

	fprintf(stderr,
	    "\n"
	    " options\n"
	    "\n"
	    "    -i <binary file>\n\n"
	    "        [Mandatory] The input file in binary format.\n\n"
	    "    -o <output file>\n\n"
	    "        [Mandatory] The output file.\n\n"
	    "    -n <n>\n\n"
	    "        [Mandatory] Number of vectors\n\n"
	    "    -v  Display version.\n\n"
	    "    -h  Display this help.\n\n"
	    "\nReturns -1 on error, 0 on success\n\n");
}
