/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generate static IDT and a bitmap of allocated interrupt vectors.
 * Creates a static IA-32 Interrupt Descriptor Table (IDT).
 *
 * This program expects to be invoked as follows:
 *  gen_idt -i <input file> -o <IDT output file> -n <# of interrupt vectors>
 *          -b <allocated vectors bitmap file>
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "version.h"

/* Define __packed for the idtEntry structure defined in idtEnt.h */
#define __packed __attribute__((__packed__))

/* This comes from the shared directory */
#include <segmentation.h>

#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(__WIN32__)
  #define O_BINARY 0
#endif

#define MAX_NUM_VECTORS           256
#define MAX_PRIORITIES            14
#define MAX_VECTORS_PER_PRIORITY  16
#define MAX_IRQS                  256

#define UNSPECIFIED_INT_VECTOR    ((unsigned int) -1)
#define UNSPECIFIED_PRIORITY      ((unsigned int) -1)
#define UNSPECIFIED_IRQ           ((unsigned int) -1)

#define KERNEL_CODE_SEG_SELECTOR	0x0008

static void get_exec_name(char *pathname);
static void usage(int len);
static void get_options(int argc, char *argv[]);
static void open_files(void);
static void read_input_file(void);
static void close_files(void);
static void validate_input_file(void);
static void generate_idt(void);
static void generate_interrupt_vector_bitmap(void);
static void clean_exit(int exit_code);
static void debug(const char *format, ...);

struct genidt_header_s {
	uint32_t spurious_addr;
	uint32_t spurious_no_error_addr;
	unsigned int num_entries;
};

struct genidt_entry_s {
	uint32_t isr;
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
	MFILE,                 /* irq to interrupt vector mapping file */
	NUSERFILES,            /* number of user-provided file names */
	EXECFILE = NUSERFILES, /* for name of executable */
	NFILES                 /* total number of file names */
};
enum { SHORT_USAGE, LONG_USAGE };

static int fds[NUSERFILES] = {-1, -1};
static char *filenames[NFILES];
static unsigned int num_vectors = (unsigned int)-1;
static struct version version = {KERNEL_VERSION, 1, 1, 6};
static unsigned int num_irq_lines = -1;
static int verbose;

static void debug(const char *format, ...)
{
	va_list vargs;

	if (!verbose)
		return;

	va_start(vargs, format);
	vfprintf(stderr, format, vargs);
	va_end(vargs);
}

int main(int argc, char *argv[])
{
	get_exec_name(argv[0]);
	get_options(argc, argv);             /* may exit */
	open_files();                        /* may exit */
	read_input_file();                   /* may exit */
	validate_input_file();               /* may exit */
	generate_interrupt_vector_bitmap();  /* may exit */
	generate_idt();                      /* may exit */
	close_files();
	return 0;
}

static void get_options(int argc, char *argv[])
{
	char *endptr;
	int ii, opt;

	while ((opt = getopt(argc, argv, "hi:o:m:n:vl:d")) != -1) {
		switch (opt) {
		case 'i':
			filenames[IFILE] = optarg;
			break;
		case 'o':
			filenames[OFILE] = optarg;
			break;
		case 'm':
			filenames[MFILE] = optarg;
			break;
		case 'h':
			usage(LONG_USAGE);
			exit(0);
		case 'l':
			num_irq_lines = (unsigned int) strtoul(optarg, &endptr, 10);
			if ((*optarg == '\0') || (*endptr != '\0')) {
				usage(SHORT_USAGE);
				exit(-1);
				}
			break;
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
		case 'd':
			verbose = 1;
			break;
		default:
			usage(SHORT_USAGE);
			exit(-1);
	    }
	}

	if (num_vectors > MAX_NUM_VECTORS) {
		usage(SHORT_USAGE);
		exit(-1);
	}

	if (num_irq_lines > MAX_IRQS) {
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
	fds[MFILE] = open(filenames[MFILE], O_WRONLY | O_BINARY | O_CREAT |
						O_TRUNC | O_BINARY,
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
	debug("Vector %3d: ISR 0x%08x IRQ %3d PRI %2d DPL %2x\n",
			entry->vector_id, entry->isr, entry->irq, entry->priority,
			entry->dpl);
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

	debug("Spurious interrupt handlers found at 0x%x and 0x%x.\n",
		    genidt_header.spurious_addr, genidt_header.spurious_no_error_addr);
	debug("There are %d ISR(s).\n", genidt_header.num_entries);


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

	int i;

	for (i = 0; i < genidt_header.num_entries; i++) {
		show_entry(&supplied_entry[i]);
	}
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

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].vector_id == UNSPECIFIED_INT_VECTOR) {
			/*
			 * Vector is to be allocated.  No further validation to be
			 * done at the moment.
			 */
			continue;
		}

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

	/* Validate the priority. */
	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].priority == UNSPECIFIED_PRIORITY) {
			if (supplied_entry[i].vector_id == UNSPECIFIED_INT_VECTOR) {
				fprintf(stderr,
						"Either priority or vector ID must be specified.\n");
				show_entry(&supplied_entry[i]);
				clean_exit(-1);
			}
		} else if (supplied_entry[i].priority >= MAX_PRIORITIES) {
			fprintf(stderr, "Priority must not exceed %d.\n",
					MAX_PRIORITIES - 1);
			show_entry(&supplied_entry[i]);
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
		if (supplied_entry[i].irq == UNSPECIFIED_IRQ) {
			if (supplied_entry[i].vector_id == UNSPECIFIED_INT_VECTOR) {
				fprintf(stderr,
						"Either IRQ or vector ID must be specified.\n");
				show_entry(&supplied_entry[i]);
				clean_exit(-1);
			}
			continue;
		}

		if (supplied_entry[i].irq >= num_irq_lines) {
			/*
			 * If code to support the PIC is re-introduced, then this
			 * check will need to be updated.
			 */
			fprintf(stderr, "IRQ must be between 0 and %d inclusive.\n",
					num_irq_lines - 1);
			show_entry(&supplied_entry[i]);
			clean_exit(-1);
		}
		num_irqs[supplied_entry[i].irq]++;
	}

	for (i = 0; i < num_irq_lines; i++) {
		if (num_irqs[i] > 1) {
			fprintf(stderr, "Multiple requests (%d) for IRQ %d detected.\n",
				num_irqs[i], i);
			clean_exit(-1);
		}
	}
}

static void validate_input_file(void)
{
	validate_dpl();          /* exits on error */
	validate_irq();          /* exits on error */
	validate_vector_id();    /* exits on error */
	validate_priority();     /* exits on error */
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
		/* Initialize the [irq] and [priority] fields to aid in debugging.  */
		generated_entry[i].irq = UNSPECIFIED_IRQ;
		generated_entry[i].priority = UNSPECIFIED_PRIORITY;
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
		struct segment_descriptor idt_entry;
		ssize_t bytes_written;

		_init_irq_gate(&idt_entry, KERNEL_CODE_SEG_SELECTOR,
			       generated_entry[i].isr, generated_entry[i].dpl);

		bytes_written = write(fds[OFILE], &idt_entry, sizeof(idt_entry));
		if (bytes_written != sizeof(idt_entry)) {
			fprintf(stderr, "Failed to write IDT entry %u.\n", num_vectors);
			clean_exit(-1);
		}
	}

	return;
}

static int find_first_set_lsb(unsigned int value)
{
	int i;

	for (i = 0; i < 32; i++) {
		if ((value & (1 << i)) != 0) {
			return i;
		}
	}

	return -1;
}

static void generate_interrupt_vector_bitmap(void)
{
	int i;
	unsigned int num_elements = (num_vectors + 31) / 32;
	unsigned int interrupt_vector_bitmap[num_elements];
	uint8_t  map_irq_to_vector_id[MAX_IRQS] = {0};
	unsigned int value;
	unsigned int index;
	unsigned int mask_index;
	int bit;
	size_t bytes_to_write;
	ssize_t bytes_written;
	static unsigned int mask[2] = {0x0000ffff, 0xffff0000};

	/* Initially mark each interrupt vector as available */
	for (i = 0; i < num_elements; i++) {
		interrupt_vector_bitmap[i] = 0xffffffff;
	}

	/* Ensure that any leftover entries are marked as allocated. */
	for (i = num_vectors; i < num_elements * 32; i++) {
		index = i / 32;
		bit = i & 0x1f;

		interrupt_vector_bitmap[index] &= ~(1 << bit);
	}

	/*
	 * Vector allocation is done in two steps.
	 * 1. Loop through each supplied entry and if an explicit vector was
	 *    specified, mark it as allocated.
	 * 2. Loop through each supplied entry and allocate the vector if
	 *    it is required.
	 * This approach guarantees that explicitly specified interrupt vectors
	 * will get their requested slots.
	 */

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].vector_id == UNSPECIFIED_INT_VECTOR) {
			/* This vector will be allocated in the next for-loop. */
			continue;
		}

		index = supplied_entry[i].vector_id / 32;
		bit = supplied_entry[i].vector_id & 31;

		interrupt_vector_bitmap[index] &= ~(1 << bit);
	}

	for (i = 0; i < genidt_header.num_entries; i++) {
		if (supplied_entry[i].vector_id != UNSPECIFIED_INT_VECTOR) {
			/* This vector has already been processed. */
			continue;
		}

		/* We can assume priority has been explicitly set as
		 * validate_priority() enforces that you can't use both
		 * UNSPECIFIED_INT_VECTOR and UNSPECIFIED_INT_PRIORITY
		 */
		index = (supplied_entry[i].priority + 2) / 2;
		mask_index = (supplied_entry[i].priority + 2) & 1;
		value = interrupt_vector_bitmap[index] & mask[mask_index];
		bit = find_first_set_lsb(value);
		if (bit < 0) {
			fprintf(stderr,
					"No remaining vectors for priority %d are available.\n",
					supplied_entry[i].priority);
			clean_exit(-1);
		}

		interrupt_vector_bitmap[index] &= ~(1 << bit);
		map_irq_to_vector_id[supplied_entry[i].irq] = (index * 32) + bit;
		supplied_entry[i].vector_id = (index * 32) + bit;
		debug("Assigned IRQ %3d (priority %2d) vector %3d\n",
		      supplied_entry[i].irq, supplied_entry[i].priority,
		      supplied_entry[i].vector_id);
	}

	bytes_to_write = num_irq_lines;
	bytes_written = write(fds[MFILE], map_irq_to_vector_id, bytes_to_write);
	if (bytes_written != bytes_to_write) {
		fprintf(stderr, "Failed to write all data to '%s'.\n",
				filenames[MFILE]);
		clean_exit(-1);
	}
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
	    "    -b <allocated interrupt vector bitmap output file>\n\n"
	    "        [Mandatory] The interrupt vector bitmap output file.\n\n"
	    "    -i <binary file>\n\n"
	    "        [Mandatory] The input file in binary format.\n\n"
	    "    -o <IDT output file>\n\n"
	    "        [Mandatory] The IDT output file.\n\n"
	    "    -m <IRQ to interrupt vector map file>\n\n"
	    "        [Mandatory] The IRQ to interrupt vector output file\n\n"
	    "    -n <n>\n\n"
	    "        [Mandatory] Number of vectors\n\n"
	    "    -l <n>\n\n"
	    "        [Mandatory] Number of IRQ lines\n\n"
	    "    -v  Display version.\n\n"
	    "    -h  Display this help.\n\n"
	    "    -d  Display extra debugging output.\n\n"
	    "\nReturns -1 on error, 0 on success\n\n");
}
