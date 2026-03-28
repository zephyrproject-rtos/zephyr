/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test_filesystem
 * Tests the fs_open flags
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <string.h>

/* Path for testr file should be provided by test runner and should start
 * with mount point.
 */
extern char *test_fs_open_flags_file_path;

static const char something[] = "Something";
static char buffer[sizeof(something)];
#define RDWR_SIZE sizeof(something)

struct test_state {
	/* Path to file */
	char *file_path;
	struct fs_file_t file;
	/* Read write buffer info */
	const char *write;
	int write_size;
	char *read;
	int read_size;
};

/* ZEQ decides whether test completed successfully and prints appropriate
 * information.
 */
static void ZEQ(int ret, int expected)
{
	zassert_equal(ret, expected,
		      "FAILED: expected = %d, ret = %d, errno = %d\n",
		      expected, ret, errno);
	TC_PRINT("SUCCESS\n");
}

/* NOTE: Below functions have C preprocessor redefinitions that automatically
 * fill in the line parameter, so when invoking, do not provide the line
 * parameter.
 */

/* Test fs_open, expected is the return value expected from fs_open */
static void ZOPEN(struct test_state *ts, int flags, int expected, int line)
{
	TC_PRINT("# %d: OPEN %s with flags %x\n", line, ts->file_path, flags);
	ZEQ(fs_open(&ts->file, ts->file_path,  flags), expected);
}

/* Close file. Will automatically fail test case if unsuccessful. */
static void ZCLOSE(struct test_state *ts, int line)
{
	TC_PRINT("# %d: CLOSE %s\n", line, ts->file_path);
	ZEQ(fs_close(&ts->file), 0);
}

/* Attempt to write to file; expected is what fs_write is supposed to return */
static void ZWRITE(struct test_state *ts, int expected, int line)
{
	TC_PRINT("# %d: WRITE %s\n", line, ts->file_path);
	ZEQ(fs_write(&ts->file, ts->write, ts->write_size), expected);
}

/* Attempt to read from file; expected is what fs_read is supposed to return */
static void ZREAD(struct test_state *ts, int expected, int line)
{
	TC_PRINT("# %d: READ %s\n", line, ts->file_path);
	ZEQ(fs_read(&ts->file, ts->read, ts->read_size), expected);
}

/* Unlink/delete file. Will automatically fail test case if unsuccessful. */
static void ZUNLINK(struct test_state *ts, int line)
{
	int ret;

	TC_PRINT("# %d: UNLINK %s\n", line, ts->file_path);
	ret  = fs_unlink(ts->file_path);
	zassert((ret == 0 || ret == -ENOENT), "Done", "Failed");
	TC_PRINT("SUCCESS\n");
}

/* Check file position; expected is a position file should be at. */
static void ZCHKPOS(struct test_state *ts, off_t expected, int line)
{
	TC_PRINT("# %d: CHKPOS\n", line);
	ZEQ(fs_tell(&ts->file), expected);
}

/* Rewind file. */
static void ZREWIND(struct test_state *ts, int line)
{
	TC_PRINT("# %d: REWIND\n", line);
	ZEQ(fs_seek(&ts->file, 0, FS_SEEK_SET), 0);
}

/* Banner definitions, print BEGIN/END banner with block number.
 * Require definition of block variable with vale that will represent first
 * test block number; the variable is automatically incremented by END.
 */
#define ZBEGIN(info) \
	TC_PRINT("\n## BEGIN %d: %s (line %d)\n", block, info, __LINE__)
#define ZEND() TC_PRINT("## END %d\n", block++)

/* C preprocessor redefinitions that automatically fill in line parameter */
#define ZOPEN(ts, flags, expected) ZOPEN(ts, flags, expected, __LINE__)
#define ZCLOSE(ts) ZCLOSE(ts, __LINE__)
#define ZWRITE(ts, expected) ZWRITE(ts, expected, __LINE__)
#define ZREAD(ts, expected) ZREAD(ts, expected, __LINE__)
#define ZUNLINK(ts) ZUNLINK(ts, __LINE__)
#define ZCHKPOS(ts, expected) ZCHKPOS(ts, expected, __LINE__)
#define ZREWIND(ts) ZREWIND(ts, __LINE__)

/* Create empty file */
#define ZMKEMPTY(ts)			\
do {					\
	ZUNLINK(ts);			\
	ZOPEN(ts, FS_O_CREATE, 0);	\
	ZCLOSE(ts);			\
} while (0)

void test_fs_open_flags(void)
{
	struct test_state ts = {
		*&test_fs_open_flags_file_path,
		{ 0 },
		something,
		RDWR_SIZE,
		buffer,
		RDWR_SIZE,
	};
	int block = 1;

	fs_file_t_init(&ts.file);

	ZBEGIN("Attempt open non-existent");
	ZOPEN(&ts, 0, -ENOENT);
	ZOPEN(&ts, FS_O_WRITE, -ENOENT);
	ZOPEN(&ts, FS_O_READ, -ENOENT);
	ZOPEN(&ts, FS_O_RDWR, -ENOENT);
	ZOPEN(&ts, FS_O_APPEND, -ENOENT);
	ZOPEN(&ts, FS_O_TRUNC, -EACCES);
	ZOPEN(&ts, FS_O_APPEND | FS_O_READ, -ENOENT);
	ZOPEN(&ts, FS_O_APPEND | FS_O_WRITE, -ENOENT);
	ZOPEN(&ts, FS_O_APPEND | FS_O_RDWR, -ENOENT);
	ZOPEN(&ts, FS_O_TRUNC | FS_O_RDWR, -ENOENT);
	ZOPEN(&ts, FS_O_TRUNC | FS_O_APPEND, -EACCES);
	ZOPEN(&ts, FS_O_TRUNC | FS_O_RDWR | FS_O_APPEND, -ENOENT);
	ZEND();


	/* Attempt create new file with no read/write access and check
	 * operations on it.
	 */
	ZBEGIN("Attempt create new with no R/W access");
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZOPEN(&ts, FS_O_CREATE | 0, 0);
	ZWRITE(&ts, -EACCES);
	ZREAD(&ts, -EACCES);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	#else
	TC_PRINT("Bypassed test\n");
	#endif
	ZEND();


	ZBEGIN("Attempt create new with READ access");
	ZOPEN(&ts, FS_O_CREATE | FS_O_READ, 0);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZWRITE(&ts, -EACCES);
	#else
	TC_PRINT("Write bypassed\n");
	#endif
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt create new with WRITE access");
	ZOPEN(&ts, FS_O_CREATE | FS_O_WRITE, 0);
	ZWRITE(&ts, ts.write_size);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZREAD(&ts, -EACCES);
	#else
	TC_PRINT("Read bypassed\n");
	#endif
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt create new with R/W access");
	ZOPEN(&ts, FS_O_CREATE | FS_O_RDWR, 0);
	ZWRITE(&ts, ts.write_size);
	/* Read is done at the end of file, so 0 bytes will be read */
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt open existing with no R/W access");
	ZMKEMPTY(&ts);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_RW_IS_DEFAULT
	ZOPEN(&ts, 0,  0);
	ZWRITE(&ts, -EACCES);
	ZREAD(&ts, -EACCES);
	ZCLOSE(&ts);
	#else
	TC_PRINT("Bypassed test\n");
	#endif
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt open existing with READ access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_READ,  0);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZWRITE(&ts, -EACCES);
	#else
	TC_PRINT("Write bypassed\n");
	#endif
	/* File is empty */
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt open existing with WRITE access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_WRITE,  0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, ts.write_size);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZREAD(&ts, -EACCES);
	#else
	TC_PRINT("Read bypassed\n");
	#endif
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt open existing with R/W access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_RDWR,  0);
	ZWRITE(&ts, ts.write_size);
	/* Read is done at the end of file, so 0 bytes will be read */
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt append existing with no R/W access");
	ZMKEMPTY(&ts);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_RW_IS_DEFAULT
	ZOPEN(&ts, FS_O_APPEND,  0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, -EACCES);
	ZREAD(&ts, -EACCES);
	ZCLOSE(&ts);
	#else
	TC_PRINT("Test bypassed\n");
	#endif
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt append existing with READ access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_APPEND | FS_O_READ,  0);
	ZCHKPOS(&ts, 0);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZWRITE(&ts, -EACCES);
	#else
	TC_PRINT("Write bypassed\n");
	#endif
	/* File is empty */
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt append existing with WRITE access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_APPEND | FS_O_WRITE,  0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, ts.write_size);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZREAD(&ts, -EACCES);
	#else
	TC_PRINT("Read bypassed\n");
	#endif
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Attempt append existing with R/W access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_APPEND | FS_O_RDWR,  0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, ts.write_size);
	/* Read is done at the end of file, so 0 bytes will be read */
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	/** FS_O_TRUNC tests */
	ZBEGIN("Attempt truncate a new file without write access");
	ZOPEN(&ts, FS_O_CREATE | FS_O_TRUNC, -EACCES);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt truncate a new file with write access");
	ZOPEN(&ts, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt truncate existing with no write access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_TRUNC, -EACCES);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt truncate existing with write access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_TRUNC | FS_O_WRITE, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt truncate existing with read access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_READ | FS_O_TRUNC, -EACCES);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt truncate existing with R/W access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_RDWR | FS_O_TRUNC, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt read on truncated file but no read access");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_WRITE | FS_O_TRUNC, 0);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZREAD(&ts, -EACCES);
	#else
	TC_PRINT("Read bypassed\n");
	#endif
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();

	ZBEGIN("Attempt append existing with WRITE access truncated file");
	ZMKEMPTY(&ts);
	ZOPEN(&ts, FS_O_APPEND | FS_O_WRITE | FS_O_TRUNC,  0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, ts.write_size);
	#ifndef BYPASS_FS_OPEN_FLAGS_LFS_ASSERT_CRASH
	ZREAD(&ts, -EACCES);
	#else
	TC_PRINT("Read bypassed\n");
	#endif
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	/* This is simple check by file position, not contents. Since writing
	 * same pattern twice, the position of file should be twice the
	 * ts.write_size.
	 */
	ZBEGIN("Check if append adds data to file");
	/* Prepare file */
	ZUNLINK(&ts);
	ZOPEN(&ts, FS_O_CREATE | FS_O_WRITE, 0);
	ZWRITE(&ts, ts.write_size);
	ZCLOSE(&ts);

	ZOPEN(&ts, FS_O_APPEND | FS_O_WRITE, 0);
	ZCHKPOS(&ts, 0);
	ZWRITE(&ts, ts.write_size);
	ZCHKPOS(&ts, ts.write_size * 2);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Check if appended forwards file before write");
	/* Prepare file */
	ZUNLINK(&ts);
	ZOPEN(&ts, FS_O_CREATE | FS_O_WRITE, 0);
	ZWRITE(&ts, ts.write_size);
	ZCLOSE(&ts);

	ZOPEN(&ts, FS_O_APPEND | FS_O_WRITE, 0);
	ZCHKPOS(&ts, 0);
	ZREWIND(&ts);
	ZWRITE(&ts, ts.write_size);
	ZCHKPOS(&ts, ts.write_size * 2);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();


	ZBEGIN("Check if file is truncated with data");
	/* Prepare file */
	ZUNLINK(&ts);
	ZOPEN(&ts, FS_O_CREATE | FS_O_WRITE, 0);
	ZWRITE(&ts, ts.write_size);
	ZCLOSE(&ts);

	/* Make sure file has the content */
	ZOPEN(&ts, FS_O_CREATE | FS_O_READ, 0);
	ZREAD(&ts, ts.write_size);
	ZCLOSE(&ts);

	ZOPEN(&ts, FS_O_TRUNC | FS_O_RDWR, 0);
	ZCHKPOS(&ts, 0);
	ZREAD(&ts, 0);
	ZCLOSE(&ts);
	ZUNLINK(&ts);
	ZEND();
}
