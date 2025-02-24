/*
 * Copyright (c) 2025 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

/* Include the generated files that embed raw data from test binary files
 * in various test combinations.
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/*
 * The file.embed.*.inc files embed the binary file data/file1.bin which contains
 * byte values from 1 to 255. The partial versions embed a subset containing
 * byte values from 53 to 89.
 */
#include <file.embed.inc>
#include <file.embed.partial.inc>
#include <file.embed.rw.inc>
#include <file.embed.rw.partial.inc>
#include <file.embed.align.inc>
#include <file.embed.align.partial.inc>
#include <file.embed.align.rw.inc>
#include <file.embed.align.rw.partial.inc>
#include <file.embed.section.inc>
#include <file.embed.section.partial.inc>
#include <file.embed.section.rw.inc>
#include <file.embed.section.rw.partial.inc>
#include <file.embed.align.section.inc>
#include <file.embed.align.section.partial.inc>
#include <file.embed.section.align.rw.inc>
#include <file.embed.section.align.rw.partial.inc>
#include <file.embed.gz.inc>
#include <file.embed.mtime.gz.inc>
#include <file.embed.mtime.now.gz.inc>
#include <file.embed.mtime.now.partial.gz.inc>

/*
 * The files.embed.*.inc files embed two binary files, data/file1.bin (which contains
 * byte values from 1 to 255) and data/file2.bin (which contains the same byte values
 * in reverse order, i.e. 255 down to 1). The partial versions embed a subset from
 * these files, containing byte values 53 to 89 from data/file1.bin, and byte values
 * 203 down to 167 from data/file2.bin.
 */
#include <files.embed.inc>
#include <files.embed.partial.inc>
#include <files.embed.rw.inc>
#include <files.embed.rw.partial.inc>
#include <files.embed.align.inc>
#include <files.embed.align.partial.inc>
#include <files.embed.align.rw.inc>
#include <files.embed.align.rw.partial.inc>
#include <files.embed.section.inc>
#include <files.embed.section.partial.inc>
#include <files.embed.section.rw.inc>
#include <files.embed.section.rw.partial.inc>
#include <files.embed.align.section.inc>
#include <files.embed.align.section.partial.inc>
#include <files.embed.section.align.rw.inc>
#include <files.embed.section.align.rw.partial.inc>
#include <files.embed.gz.inc>
#include <files.embed.mtime.gz.inc>
#include <files.embed.mtime.now.gz.inc>
#include <files.embed.mtime.now.partial.gz.inc>

/**
 * @endcond
 */

static const uint8_t mtime_zero[4] = {0};

/* This value ust match the GZIP_MTIME test_val passed to generate_binary_embed_inc_file() */
static const uint8_t mtime_test_val[4] = {54, 0, 0, 0};

static const uint8_t compressed_inc_file1[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xff, 0x01, 0xff, 0x00, 0x00, 0xff, 0x01,
	0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
	0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
	0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
	0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
	0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
	0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81,
	0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91,
	0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1,
	0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1,
	0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1,
	0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1,
	0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1,
	0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 0x87, 0x1f,
	0x16, 0xd0, 0xff, 0x00, 0x00, 0x00
};

static const uint8_t compressed_inc_file2[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xff, 0x01, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7,
	0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0, 0xef,
	0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8, 0xe7,
	0xe6, 0xe5, 0xe4, 0xe3, 0xe2, 0xe1, 0xe0, 0xdf,
	0xde, 0xdd, 0xdc, 0xdb, 0xda, 0xd9, 0xd8, 0xd7,
	0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 0xd1, 0xd0, 0xcf,
	0xce, 0xcd, 0xcc, 0xcb, 0xca, 0xc9, 0xc8, 0xc7,
	0xc6, 0xc5, 0xc4, 0xc3, 0xc2, 0xc1, 0xc0, 0xbf,
	0xbe, 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8, 0xb7,
	0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0xb0, 0xaf,
	0xae, 0xad, 0xac, 0xab, 0xaa, 0xa9, 0xa8, 0xa7,
	0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0, 0x9f,
	0x9e, 0x9d, 0x9c, 0x9b, 0x9a, 0x99, 0x98, 0x97,
	0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90, 0x8f,
	0x8e, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x88, 0x87,
	0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x80, 0x7f,
	0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77,
	0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70, 0x6f,
	0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x67,
	0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f,
	0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57,
	0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4f,
	0x4e, 0x4d, 0x4c, 0x4b, 0x4a, 0x49, 0x48, 0x47,
	0x46, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40, 0x3f,
	0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37,
	0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2f,
	0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27,
	0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1f,
	0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17,
	0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f,
	0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07,
	0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x4d, 0xaa,
	0x73, 0x54, 0xff, 0x00, 0x00, 0x00
};

static const uint8_t compressed_partial_inc_file1[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xff, 0x33, 0x35, 0x33, 0xb7, 0xb0, 0xb4,
	0xb2, 0xb6, 0xb1, 0xb5, 0xb3, 0x77, 0x70, 0x74,
	0x72, 0x76, 0x71, 0x75, 0x73, 0xf7, 0xf0, 0xf4,
	0xf2, 0xf6, 0xf1, 0xf5, 0xf3, 0x0f, 0x08, 0x0c,
	0x0a, 0x0e, 0x09, 0x0d, 0x0b, 0x8f, 0x88, 0x04,
	0x00, 0x81, 0x5d, 0x01, 0xc4, 0x25, 0x00, 0x00,
	0x00
};

static const uint8_t compressed_partial_inc_file2[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0xff, 0x01, 0x25, 0x00, 0xda, 0xff, 0xcb,
	0xca, 0xc9, 0xc8, 0xc7, 0xc6, 0xc5, 0xc4, 0xc3,
	0xc2, 0xc1, 0xc0, 0xbf, 0xbe, 0xbd, 0xbc, 0xbb,
	0xba, 0xb9, 0xb8, 0xb7, 0xb6, 0xb5, 0xb4, 0xb3,
	0xb2, 0xb1, 0xb0, 0xaf, 0xae, 0xad, 0xac, 0xab,
	0xaa, 0xa9, 0xa8, 0xa7, 0x7a, 0x38, 0x72, 0x20,
	0x25, 0x00, 0x00, 0x00
};

#define	ASSERT_IN_SECTION(symbol_name, symbol_size, section_name)				\
	extern uint8_t __##section_name##_start[];						\
	extern uint8_t __##section_name##_end[];						\
												\
	zassert_true((&symbol_name[0] >= (uint8_t *)&__##section_name##_start) &&		\
		     (&symbol_name[symbol_size - 1] <= (uint8_t *)&__##section_name##_end));

static void do_test_gen_inc_file1(const uint8_t inc_file[], int inc_file_size, int offset)
{
	for (int i = 0; i < inc_file_size; i++) {
		zassert_equal(inc_file[i], i + 1 + offset, "Invalid data value in inc file");
	}
}

static void do_test_gen_inc_file2(const uint8_t inc_file[], int inc_file_size, int offset)
{
	for (int i = 0; i < inc_file_size; i++) {
		zassert_equal(inc_file[i], (255 - i - offset), "Invalid data value in inc file");
	}
}

static void write_data_test_file1(uint8_t inc_file[], int inc_file_size, int offset)
{
	for (int i = 0; i < inc_file_size; i++) {
		inc_file[i] = i + 1 + offset;
	}
}

static void write_data_test_file2(uint8_t inc_file[], int inc_file_size, int offset)
{
	for (int i = 0; i < inc_file_size; i++) {
		inc_file[i] = 255 - i - offset;
	}
}

#define	RW_TEST_FILE_NUM_FOR_FILE1	2
#define	RW_TEST_FILE_NUM_FOR_FILE2	1

#define	_DO_TEST_ONE_FILE(file_num, file_data_symbol, test_size, test_offset,			\
			  check_rw, test_align, check_section)					\
	zassert_equal(sizeof(file_data_symbol), test_size, "Invalid file size");		\
	zassert_equal(file_data_symbol##_SIZE, test_size, "Invalid file size");			\
												\
	do_test_gen_inc_file##file_num(file_data_symbol, sizeof(file_data_symbol), test_offset);\
												\
	if (test_align) {									\
		zassert_true(IS_ALIGNED(file_data_symbol, 32));					\
	}											\
												\
	if (check_section) {									\
		if (check_rw) {									\
			ASSERT_IN_SECTION(file_data_symbol, test_size, test_rw_section);	\
		}										\
		else {										\
			ASSERT_IN_SECTION(file_data_symbol, test_size, test_section);		\
		}										\
	}											\
												\
	if (check_rw) {										\
		UTIL_CAT(write_data_test_file, RW_TEST_FILE_NUM_FOR_FILE##file_num)		\
			((uint8_t *)file_data_symbol, test_size, test_offset);			\
		UTIL_CAT(do_test_gen_inc_file, RW_TEST_FILE_NUM_FOR_FILE##file_num)		\
			(file_data_symbol, test_size, test_offset);				\
	}

#define	DO_TEST_ONE_FILE(file_num, file_data_symbol,						\
			 check_section, check_align, check_rw, check_partial)			\
	_DO_TEST_ONE_FILE(file_num, file_data_symbol, (check_partial ? 37 : 255),		\
		(check_partial ? 52 : 0), check_rw, check_align, check_section)

/* Test inclusion of data with default settings. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_one_file)
{
	DO_TEST_ONE_FILE(1, inc_file, false, false, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_two_files)
{
	DO_TEST_ONE_FILE(1, inc_file1, false, false, false, false);
	DO_TEST_ONE_FILE(2, inc_file2, false, false, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_partial_inc_one_file)
{
	DO_TEST_ONE_FILE(1, partial_inc_file, false, false, false, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_partial_inc_two_files)
{
	DO_TEST_ONE_FILE(1, partial_inc_file1, false, false, false, true);
	DO_TEST_ONE_FILE(2, partial_inc_file2, false, false, false, true);
}

/* Test inclusion of data as writable. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_rw_one_file)
{
	DO_TEST_ONE_FILE(1, rw_inc_file, false, false, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_rw_two_files)
{
	DO_TEST_ONE_FILE(1, rw_inc_file1, false, false, true, false);
	DO_TEST_ONE_FILE(2, rw_inc_file2, false, false, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_rw_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_rw_inc_file, false, false, true, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_rw_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_rw_inc_file1, false, false, true, true);
	DO_TEST_ONE_FILE(2, partial_rw_inc_file2, false, false, true, true);
}

/* Test inclusion of data with alignment specified. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_one_file)
{
	DO_TEST_ONE_FILE(1, aligned_inc_file, false, true, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_two_files)
{
	DO_TEST_ONE_FILE(1, aligned_inc_file1, false, true, false, false);
	DO_TEST_ONE_FILE(2, aligned_inc_file2, false, true, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_aligned_inc_file, false, true, false, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_aligned_inc_file1, false, true, false, true);
	DO_TEST_ONE_FILE(2, partial_aligned_inc_file2, false, true, false, true);
}

/* Test inclusion of data as writable, with alignment specified. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_rw_one_file)
{
	DO_TEST_ONE_FILE(1, rw_aligned_inc_file, false, true, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_rw_two_files)
{
	DO_TEST_ONE_FILE(1, rw_aligned_inc_file1, false, true, true, false);
	DO_TEST_ONE_FILE(2, rw_aligned_inc_file2, false, true, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_rw_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_rw_aligned_inc_file, false, true, true, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_rw_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_rw_aligned_inc_file1, false, true, true, true);
	DO_TEST_ONE_FILE(2, partial_rw_aligned_inc_file2, false, true, true, true);
}

/* Test inclusion of data into a custom linker section. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_one_file)
{
	DO_TEST_ONE_FILE(1, section_inc_file, true, false, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_two_files)
{
	DO_TEST_ONE_FILE(1, section_inc_file1, true, false, false, false);
	DO_TEST_ONE_FILE(2, section_inc_file2, true, false, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_section_inc_file, true, false, false, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_section_inc_file1, true, false, false, true);
	DO_TEST_ONE_FILE(2, partial_section_inc_file2, true, false, false, true);
}

/* Test inclusion of data as writable, into a custom linker section. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_rw_one_file)
{
	DO_TEST_ONE_FILE(1, rw_section_inc_file, true, false, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_rw_two_files)
{
	DO_TEST_ONE_FILE(1, rw_section_inc_file1, true, false, true, false);
	DO_TEST_ONE_FILE(2, rw_section_inc_file2, true, false, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_rw_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_rw_section_inc_file, true, false, true, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_rw_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_rw_section_inc_file1, true, false, true, true);
	DO_TEST_ONE_FILE(2, partial_rw_section_inc_file2, true, false, true, true);
}

/* Test inclusion of data into a custom linker section, with alignment specified. */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_section_one_file)
{
	DO_TEST_ONE_FILE(1, aligned_section_inc_file, true, true, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_section_two_files)
{
	DO_TEST_ONE_FILE(1, aligned_section_inc_file1, true, true, false, false);
	DO_TEST_ONE_FILE(2, aligned_section_inc_file2, true, true, false, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_section_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_aligned_section_inc_file, true, true, false, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_aligned_section_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_aligned_section_inc_file1, true, true, false, true);
	DO_TEST_ONE_FILE(2, partial_aligned_section_inc_file2, true, true, false, true);
}

/* Test inclusion of data as writable, into a custom linker section, with
 * alignment also specified.
 */

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_align_rw_one_file)
{
	DO_TEST_ONE_FILE(1, rw_aligned_section_inc_file, true, true, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_align_rw_two_files)
{
	DO_TEST_ONE_FILE(1, rw_aligned_section_inc_file1, true, true, true, false);
	DO_TEST_ONE_FILE(2, rw_aligned_section_inc_file2, true, true, true, false);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_align_rw_partial_one_file)
{
	DO_TEST_ONE_FILE(1, partial_rw_aligned_section_inc_file, true, true, true, true);
}

ZTEST(gen_binary_embed_inc_file, test_gen_inc_section_align_rw_partial_two_files)
{
	DO_TEST_ONE_FILE(1, partial_rw_aligned_section_inc_file1, true, true, true, true);
	DO_TEST_ONE_FILE(2, partial_rw_aligned_section_inc_file2, true, true, true, true);
}

static void do_test_gen_gz_inc_file(const uint8_t gz_inc_file[],
				    const uint8_t ref_file[], int ref_file_size,
				    const uint8_t mtime[4])
{
	/* Check the 4-byte modification time field in the gzip header,
	 * unless it is to be ignored (e.g. when verifying the data
	 * compressed with mtime specified as "now").
	 */
	zassert_true(mtime == NULL || (memcmp(&gz_inc_file[4], mtime, 4) == 0),
		"Invalid mtime in inc file");

	zassert_true(memcmp(&gz_inc_file[0], &ref_file[0], 4) == 0 &&
		     memcmp(&gz_inc_file[8], &ref_file[8], ref_file_size - 8) == 0,
		     "Invalid data value in inc file");
}

#define	_DO_TEST_ONE_FILE_GZ(file_num, file_data_symbol, ref_file,				\
			     test_size_uncompressed, test_mtime)				\
	zassert_equal(sizeof(file_data_symbol), sizeof(ref_file),				\
		      "Invalid compressed file size");						\
	zassert_equal(file_data_symbol##_SIZE, sizeof(ref_file),				\
		      "Invalid compressed file size");						\
	zassert_equal(file_data_symbol##_SIZE_UNCOMPRESSED, test_size_uncompressed,		\
		      "Invalid uncompressed file size");					\
												\
	do_test_gen_gz_inc_file(file_data_symbol, ref_file, sizeof(ref_file), test_mtime)	\

#define	DO_TEST_ONE_FILE_GZ(file_num, file_data_symbol, test_mtime)				\
	_DO_TEST_ONE_FILE_GZ(file_num, file_data_symbol, compressed_inc_file##file_num,		\
			     255, test_mtime)

#define	DO_TEST_ONE_FILE_GZ_PARTIAL(file_num, file_data_symbol, test_mtime)			\
	_DO_TEST_ONE_FILE_GZ(file_num, file_data_symbol, compressed_partial_inc_file##file_num,	\
			     37, test_mtime)

/* Test inclusion of data compressed with gzip, without mtime. */

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_no_mtime_one_file)
{
	DO_TEST_ONE_FILE_GZ(1, no_mtime_gz_inc_file, mtime_zero);
}

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_no_mtime_two_files)
{
	DO_TEST_ONE_FILE_GZ(1, no_mtime_gz_inc_file1, mtime_zero);
	DO_TEST_ONE_FILE_GZ(2, no_mtime_gz_inc_file2, mtime_zero);
}

/* Test inclusion of data compressed with gzip, with mtime specified. */

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_one_file)
{
	DO_TEST_ONE_FILE_GZ(1, mtime_gz_inc_file, mtime_test_val);
}

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_two_files)
{
	DO_TEST_ONE_FILE_GZ(1, mtime_gz_inc_file1, mtime_test_val);
	DO_TEST_ONE_FILE_GZ(2, mtime_gz_inc_file2, mtime_test_val);
}

/* Test inclusion of data compressed with gzip, with mtime specified as "now". */

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_now_one_file)
{
	DO_TEST_ONE_FILE_GZ(1, mtime_now_gz_inc_file, NULL);
}

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_now_two_files)
{
	DO_TEST_ONE_FILE_GZ(1, mtime_now_gz_inc_file1, NULL);
	DO_TEST_ONE_FILE_GZ(2, mtime_now_gz_inc_file2, NULL);
}

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_now_partial_one_file)
{
	DO_TEST_ONE_FILE_GZ_PARTIAL(1, partial_mtime_now_gz_inc_file, NULL);
}

ZTEST(gen_binary_embed_inc_file, test_gen_gz_inc_mtime_now_partial_two_files)
{
	DO_TEST_ONE_FILE_GZ_PARTIAL(1, partial_mtime_now_gz_inc_file1, NULL);
	DO_TEST_ONE_FILE_GZ_PARTIAL(2, partial_mtime_now_gz_inc_file2, NULL);
}

ZTEST_SUITE(gen_binary_embed_inc_file, NULL, NULL, NULL, NULL, NULL);
