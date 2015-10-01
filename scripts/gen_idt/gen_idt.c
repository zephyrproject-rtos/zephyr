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

/**
 * @file
 * @brief Create static IDT
 * Creates a static IA-32 Interrupt Descriptor Table (IDT).
 *
 * This program expects to be invoked as follows:
 *  gen_idt -i <input file> -o <output file> -n <number of interrupt vectors>
 * All parameters are required.
 *
 * The <input file> is assumed to be a binary file containing the intList
 * section from the Zephyr Kernel ELF image.
 *
 * <number of interrupt vectors> is the same as CONFIG_IDT_NUM_VECTORS.
 *
 * It is expected that this program is invoked from within the build system
 * during the link stage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "version.h"

/* Define __packed for the idtEntry structure defined in idtEnt.h */
#define __packed __attribute__((__packed__))

/* These come from the shared directory */
#include "segselect.h"
#include "idtEnt.h"

/* Define the following macro to see a verbose or debug output from the tool */
/* #define DEBUG_GENIDT */

#ifdef DEBUG_GENIDT
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#if !defined(WINDOWS)
	#define O_BINARY 0
#endif

#define MAX_NUM_VECTORS           256
#define MAX_PRIORITIES            16
#define MAX_VECTORS_PER_PRIORITY  16
#define MAX_IRQS                  256

static void get_exec_name(char *pathname);
static void usage(int len);
static void get_options(int argc, char *argv[]);
static void open_files(void);
static void read_input_file(void);
static void close_files(void);
static void validate_input_file(void);
static void generate_idt(void);
static void clean_exit(int exit_code);

struct genidt_header_s {
	void *spurious_addr;
	void *spurious_no_error_addr;
	unsigned int num_entries;
};

struct genidt_entry_s {
	void *isr;
	unsigned int irq;
	unsigned int priority;
	unsigned int vector_id;
	unsigned int dpl;
};

static struct genidt_header_s  genidt_header;
static struct genidt_entry_s   supplied_entry[MAX_NUM_VECTORS];
static struct genidt_entry_s   generated_entry[MAX_NUM_VECTORS];

enum {
	IFILE = 0,             /* input file */
	OFILE,                 /* output file */
	NUSERFILES,            /* number of user-provided file names */
	EXECFILE = NUSERFILES, /* for name of executable */
	NFILES                 /* total number of file names */
};
enum { SHORT_USAGE, LONG_USAGE };

static int fds[NUSERFILES] = {-1, -1};
static char *filenames[NFILES];
static unsigned int num_vectors = (unsigned int)-1;
static struct version version = {KERNEL_VERSION, 1, 1, 3};

int main(int argc, char *argv[])
{
	get_exec_name(argv[0]);
	get_options(argc, argv); /* may exit */
	open_files();            /* may exit */
	read_input_file();       /* may exit */
	validate_input_file();   /* may exit */
	generate_idt();          /* may exit */
	close_files();
	return 0;
}

static void get_options(int argc, char *argv[])
{
	char *endptr;
	int ii, opt;

	while ((opt = getopt(argc, argv, "hi:o:n:v")) != -1) {
		switch (opt) {
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
			num_vectors = (unsigned int) strtoul(optarg, &endptr, 10);
			if ((*optarg == '\0') || (*endptr != '\0')) {
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

	if (num_vectors > MAX_NUM_VECTORS) {
		usage(SHORT_USAGE);
		exit(-1);
	}

	for (ii = IFILE; ii < NUSERFILES; ii++) {
		if (!filenames[ii]) {
			usage(SHORT_USAGE);
			exit(-1);
		}
	}
}

static void get_exec_name(char *pathname)
{
	int end = strlen(pathname) - 1;

	while (end != -1) {
		#if defined(WINDOWS) /* Might have both slashes in path */
		if ((pathname[end] == '/') || (pathname[end] == '\\'))
		#else
		if (pathname[end] == '/')
		#endif
		{
			if ((0 == end) || (pathname[end - 1] != '\\')) {
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

	fds[IFILE] = open(filenames[IFILE], O_RDONLY | O_BINARY);
	fds[OFILE] = open(filenames[OFILE], O_WRONLY | O_BINARY |
						O_TRUNC | O_CREAT,
						S_IWUSR | S_IRUSR);

	for (ii = 0; ii < NUSERFILES; ii++) {
		int invalid = fds[ii] == -1;

		if (invalid) {
			char *invalid = filenames[ii];

			fprintf(stderr, "invalid file %s\n", invalid);
			for (--ii; ii >= 0; ii--) {
				close(fds[ii]);
			}
			exit(-1);
		}
	}
}

static void show_entry(struct genidt_entry_s *entry)
{
	fprintf(stderr,
			"ISR Address: %p\n"
			"IRQ: %d\n"
			"Priority: %d\n"
			"Vector ID: %d\n"
			"DPL: %d\n"
			"---------------\n",
			entry->isr, entry->irq, entry->priority,
			entry->vector_id, entry->dpl);
}

static void read_input_file(void)
{
	ssize_t  bytes_read;
	size_t   bytes_to_read;

	/* Read the header information. */
	bytes_read = read(fds[IFILE], &genidt_header, sizeof(genidt_header));
	if (bytes_read != sizeof(genidt_header)) {
		goto read_error;
	}

	PRINTF("Spurious interrupt handlers found at %p and %p.\n",
		    genidt_header.spurious_addr, genidt_header.spurious_no_error_addr);
	PRINTF("There are %d ISR(s).\n", genidt_header.num_entries);


	if (genidt_header.num_entries > num_vectors) {
		fprintf(stderr,
				"Too many ISRs found. Got %u. Expected no more than %u.\n"
				"Malformed input file?\n",
				genidt_header.num_entries, num_vectors);
		clean_exit(-1);
	}

	bytes_to_read = sizeof(struct genidt_entry_s) * genidt_header.num_entries;
	bytes_read = read(fds[IFILE], supplied_entry, bytes_to_read);
	if (bytes_read != bytes_to_read) {
		goto read_error;
	}

#ifdef DEBUG_GENIDT
	int i;

	for (i = 0; i < genidt_header.num_entries; i++) {
		show_entry(&supplied_entry[i]);
		fprintf(stderr, "----------------------------\n");
	}
#endif
	return;

read_error:
	fprintf(stderr, "Error occurred while reading input file. Aborting...\n");
	clean_exit(-1);
}

static void validate_dpl(void)
{
	int  i;

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].dpl != 0) {
			fprintf(stderr,
					"Invalid DPL bits specified.  Must be zero.\n");
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}
	}
}

static void validate_vector_id(void)
{
	int  i;
	int  vectors[MAX_NUM_VECTORS] = {0};

	/*
	 * NOTE: Future versions of the gen_idt tool may generate the vector ID
	 * instead of relying upon values already existing in the input file.
	 */

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].vector_id >= num_vectors) {
			fprintf(stderr,
					"Vector ID exceeds specified # of vectors (%d).\n",
					num_vectors);
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}

		if (vectors[supplied_entry[i].vector_id] != 0) {
			fprintf(stderr, "Duplicate vector ID found.\n");
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}
	vectors[supplied_entry[i].vector_id]++;
	}
}

static void validate_priority(void)
{
	int  i;
	int  num_priorities[MAX_PRIORITIES] = {0};
	unsigned expected_priority;

	/*
	 * Validate the priority.
	 *
	 * As the current implementation of the gen_idt tool takes the vector ID
	 * from the input file, the priority is ignored. In fact, the priority
	 * is overridden to match <vector ID> / MAX_VECTORS_PER_PRIORITY.
	 */

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].irq == -1) {
			/*
			 * This is a software interrupt.
			 * The priority is currently ignored.
			 */
			continue;
		}

		if (supplied_entry[i].priority >= MAX_PRIORITIES) {
			fprintf(stderr, "Priority must not exceed %d.\n",
					MAX_PRIORITIES - 1);
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}

		expected_priority = supplied_entry[i].vector_id /
							MAX_VECTORS_PER_PRIORITY;
		if (expected_priority != supplied_entry[i].priority) {
			supplied_entry[i].priority = expected_priority;
			fprintf(stderr,
					"Warning! Overriding IRQ %d priority to %d!\n",
					supplied_entry[i].irq, supplied_entry[i].priority);
		}

		num_priorities[supplied_entry[i].priority]++;
	}

	for (i = 0; i < MAX_PRIORITIES; i++) {
		if (num_priorities[i] > MAX_VECTORS_PER_PRIORITY) {
			fprintf(stderr, "Too many entries for priority level %d", i);
			clean_exit(-1);
		}
	}
}

static void validate_irq(void)
{
	int  i;
	int  num_irqs[MAX_IRQS] = {0};

	/* Validate the IRQ number */
	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].irq == (unsigned int) -1) {
			/* This is a request for a software interrupt */
			continue;
		}

		if (supplied_entry[i].irq >= MAX_IRQS) {
			/*
			 * If code to support the PIC is re-introduced, then this
			 * check will need to be updated.
			 */
			fprintf(stderr, "IRQ must be between 0 and %d inclusive.\n",
					MAX_IRQS - 1);
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}
		num_irqs[i]++;
	}

	for (i = 0; i < MAX_IRQS; i++) {
		if (num_irqs[i] > 1) {
			fprintf(stderr, "Multiple requests for IRQ %d detected.\n", i);
			clean_exit(-1);
		}
	}
}

static void validate_input_file(void)
{
	validate_dpl();          /* exits on error */
	validate_vector_id();    /* exits on error */
	validate_priority();     /* exits on error */
	validate_irq();          /* exits on error */
}

static void generate_idt(void)
{
	unsigned int  i;
	unsigned int  vector_id;

	/*
	 * Initialize the generated entries with default
	 * spurious interrupt handlers.
	 */

	for (i = 0; i < num_vectors; i++) {
		if ((((1 << i) & _EXC_ERROR_CODE_FAULTS)) && (i < 32)) {
			generated_entry[i].isr = genidt_header.spurious_addr;
		} else {
			generated_entry[i].isr = genidt_header.spurious_no_error_addr;
		}
		generated_entry[i].irq = -1;       /* Set to -1 to aid in debugging */
		generated_entry[i].priority = -1;  /* Set to -1 to aid in debugging */
		generated_entry[i].vector_id = i;
		generated_entry[i].dpl = 0;
	}

	/*
	 * Overwrite the generated entries as appropriate with the
	 * validated supplied entries.
	 */

	for (i = 0; i < genidt_header.num_entries; i++) {
		vector_id = supplied_entry[i].vector_id;
		generated_entry[vector_id] = supplied_entry[i];
	}

    /*
     * We now have the address of all ISR stub/functions captured in
     * generated_entry[]. Now construct the actual IDT.
     */

	for (i = 0; i < num_vectors; i++) {
		unsigned long long idt_entry;
		ssize_t bytes_written;

		 _IdtEntCreate(&idt_entry, generated_entry[i].isr,
						generated_entry[i].dpl);

		bytes_written = write(fds[OFILE], &idt_entry, sizeof(idt_entry));
		if (bytes_written != sizeof(idt_entry)) {
			fprintf(stderr, "Failed to write IDT entry %u.\n", num_vectors);
			clean_exit(-1);
		}
	}

	return;
}

static void close_files(void)
{
	int ii;

	for (ii = 0; ii < NUSERFILES; ii++) {
		close(fds[ii]);
	}
}

static void clean_exit(int exit_code)
{
	close_files();
	exit(exit_code);
}

static void usage(int len)
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
