/* gen_config_abssym.c - generate configs.c from autoconf.h
*/

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

Generate configs.c file containing absolute symbols for kconfig options from
autoconf.h.

configs.c is responsible for the generation of the absolute symbols whose
value represents the settings of various CONFIG configuration options.
These symbols can be used by the VxMicro host tools to determine the
configuration options in effect when the microkernel or nanokernel was built.

If a configuration macro is defined, a corresponding absolute symbol should
be generated.  The absolute symbol should have the same name as the
configuration macro.  The symbol value should be:
 - the integer 1 for configuration macros of type "bool" (which is the value
   assigned to these macros in <autoconf.h>)
 - the macro value for configuration macros of type "int" or "hex"
 - the integer 1 for configuration macros of type "string", as it is not
   possible to represent the actual value of the configuration macro.

No corresponding symbol should be generated when a macro is not defined.

All of the absolute symbols defined by configs.c will be present in the
final microkernel or nanokernel ELF image (due to the linker's reference to
the _ConfigAbsSyms symbol).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vxmicro.h>

static void usage(const int len);
static void get_options(int argc, char *argv[]);
static void open_files(void);
static void close_files(void);
static void generate_files(void);
static void clean_exit(const int exit_code);

enum {
	IFILE=0,		/* input file */
	OFILE,			/* C output file */
	NUSERFILES,		/* number of user-provided file names */
	EXECFILE=NUSERFILES,	/* for name of executable */
	NFILES			/* total number of files open */
};
enum { SHORT_USAGE, LONG_USAGE };

static int fds[NUSERFILES] = {-1, -1};
static const char *filenames[NFILES];
static struct version version = {KERNEL_VERSION, 1, 0, 0};

int main(int argc, char *argv[])
{
	filenames[EXECFILE] = basename(argv[0]);
	get_options(argc, argv); /* may exit */
	open_files();		 /* may exit */
	generate_files();
	close_files();
	return 0;
}

static void get_options(int argc, char *argv[])
{
	int ii, opt;

	while ((opt = getopt(argc, argv, "hi:o:v")) != -1) {
		switch(opt) {
		case 'i':
			filenames[IFILE] = optarg;
			break;
		case 'o':
			filenames[OFILE] = optarg;
			break;
		case 'h':
			usage(LONG_USAGE);
			exit(0);
		case 'v':
			show_version(filenames[EXECFILE], &version);
			exit(0);
		default:
			usage(SHORT_USAGE);
			exit(-1);
		}
	}
	for (ii = IFILE; ii < NUSERFILES; ii++) {
		if (!filenames[ii]) {
			usage(SHORT_USAGE);
			exit(-1);
		}
	}
}

static void open_files(void)
{
	const int open_w_flags = O_WRONLY|O_CREAT|O_TRUNC;
	const mode_t open_mode = S_IWUSR|S_IRUSR;
	int ii;

	fds[IFILE] = open_binary(filenames[IFILE], O_RDONLY);
	fds[OFILE] = open(filenames[OFILE], open_w_flags, open_mode);

	for (ii = 0; ii < NUSERFILES; ii++) {
		int invalid = fds[ii] == -1;
		if (invalid) {
			const char *invalid = filenames[ii];
			fprintf(stderr, "invalid file %s\n", invalid);
			for (--ii; ii >= 0; ii--) {
				close(fds[ii]);
			}
			exit(-1);
		}
	}
}

static void write_str(const char *str)
{
	int len = strlen(str);

	if (write(fds[OFILE], str, len) != len) {
		perror("write");
		clean_exit(-1);
	}
}

static void generate_files(void)
{
	char *buf, *pBuf;
	struct stat autoconf_h_stat;
	off_t size;

	if (stat(filenames[IFILE], &autoconf_h_stat) == -1) {
		perror("stat");
		clean_exit(-1);
	}

	buf = malloc(autoconf_h_stat.st_size);
	if (!buf) {
		perror("out-of-memory");
		clean_exit(-1);
	}

	size = read(fds[IFILE], buf, autoconf_h_stat.st_size);
	if (autoconf_h_stat.st_size != size) {
		perror("read");
		clean_exit(-1);
	}

	pBuf = buf;
	pBuf = strstr(pBuf, "#define AUTOCONF_INCLUDED");

	write_str("/* file is auto-generated, do not modify ! */\n\n");
	write_str("#include <abs_sym.h>\n\n");
	write_str("GEN_ABS_SYM_BEGIN (_ConfigAbsSyms)\n\n");

	for (;;) {
		char *sym, *val, *endl, *str = "1";

		pBuf = strstr(pBuf, "#");
		if (!strncmp(pBuf, "#endif", strlen("#endif"))) {
			break;
		}

		sym = strstr(pBuf, "CONFIG");
		val = strstr(sym, " ") + 1;
		endl = strstr(val, ENDL);

		val[-1] = 0;
		endl[0] = 0;

		if (val[0] == '\"') {
			val = str;
		}

		write_str("GEN_ABSOLUTE_SYM(");
		write_str(sym);
		write_str(", ");
		write_str(val);
		write_str(");\n");

		pBuf = endl + 1;
	}

	write_str("\nGEN_ABS_SYM_END\n");

	free(buf);
}

static void close_files(void)
{
	int ii;

	for (ii = 0; ii < NUSERFILES; ii++) {
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
	fprintf(stderr,
		"\n%s -i <autoconf.h> -o <output>\n",
		filenames[EXECFILE]);

	if (len == SHORT_USAGE) {
		return;
	}

	fprintf(stderr,
		"\n"
		" options\n"
		"\n"
		"    -i <autoconf.h>\n\n"
		"        [Mandatory] The pathname of autoconf.h.\n\n"
		"    -o <output>\n\n"
		"        [Mandatory] The pathname of output configs.c.\n"
		"    -v  Display version.\n\n"
		"    -h  Display this help.\n\n"
		"\nReturns -1 on error, 0 on success\n\n");
}
