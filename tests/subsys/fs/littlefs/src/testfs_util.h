/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Constants, data types, and functions that support testing the Zephyr FS API.
 *
 * Includes:
 *
 * * A data type that supports building and modifying absolute paths
 *   without worrying about snprintf or buffer overflow;
 * * Functions to write known content into files, and to verify that
 *   content when reading the file.
 * * A data type that is used to describe file system contents, with
 *   functions to create the file system and verify file system
 *   content against the build command description.
 */

#ifndef _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_UTIL_H_
#define _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_UTIL_H_

#include <stdarg.h>
#include <string.h>
#include <zephyr/types.h>
#include <fs/fs.h>

/** Maximum length of a path supported by the test infrastructure. */
#define TESTFS_PATH_MAX 127

/** Structure holding an absolute file system path. */
struct testfs_path {
	/* Storage for a maximal path plus NUL */
	char path[TESTFS_PATH_MAX + 1];

	/* Pointer to the NUL character marking end of string. */
	char *eos;
};

/** A properly cast terminator for variadic arguments to testfs_path
 * functions.
 */
#define TESTFS_PATH_END ((const char *)NULL)

#define TESTFS_BUFFER_SIZE 64

/** Initialize the file system path within a mount point.
 *
 * Creates an absolute path that begins with the mount point, then
 * extends it with an arbitrary number of path elements as with
 * testfs_path_extend().
 *
 * @param pp a pointer to the path structure.
 *
 * @param mp pointer to the mount point.  A null pointer may be passed
 * to create the root path without a mount point.
 *
 * @param... as with testfs_path_extend()
 *
 * @return a pointer to the start of the path.
 */
const char *testfs_path_init(struct testfs_path *pp,
			     const struct fs_mount_t *mp,
			     ...);

/** Extend/modify an existing file system path.
 *
 * Given an absolute path this function extends it with additional
 * path elements.  A forward slash is added between each element.
 *
 * If ".." is passed the last element of the path is removed.
 *
 * The final argument must be a const char * null pointer such as @ref
 * TESTFS_PATH_END.
 *
 * If adding an element would exceed the maximum allowed path length
 * extension stops, and the path existing to that point is returned.
 *
 * @param pp a pointer to the path structure.
 *
 * @param... a sequence of const char * pointers terminating with a
 * null pointer.
 *
 * @return a pointer to the start of the path.
 */
const char *testfs_path_extend(struct testfs_path *pp,
			       ...);

static inline struct testfs_path *testfs_path_copy(struct testfs_path *dp,
						   const struct testfs_path *sp)
{
	size_t len = sp->eos - sp->path;

	memcpy(dp->path, sp->path, len + 1);
	dp->eos = dp->path + len;

	return dp;
}

/** Write a sequence of constant data to the file.
 *
 * Writes are generated in blocks up to TESTFS_BUFFER_SIZE in length.
 *
 * @param fp pointer to the file to write
 *
 * @param value value of the bytes to write
 *
 * @param len number of octets to write
 *
 * @return number of octets written, or a negative error code.
 */
int testfs_write_constant(struct fs_file_t *fp,
			  u8_t value,
			  unsigned int len);

/** Verify that the file contains a sequence of constant data.
 *
 * Reads are performed in blocks up to TESTFS_BUFFER_SIZE in length.
 *
 * @param fp pointer to the file to read
 *
 * @param value the expected value of the constant data.
 *
 * @param len the number of times the constant value should appear
 *
 * @return the number of times the constant value did appear before
 * len was reached or a mismatch occurred, or a negative error on file
 * read failure.
 */
int testfs_verify_constant(struct fs_file_t *fp,
			   u8_t value,
			   unsigned int len);

/** Write an increasing sequence of bytes to the file.
 *
 * Writes are generated in blocks up to TESTFS_BUFFER_SIZE in length.
 *
 * @param fp pointer to the file to write
 *
 * @param value value of the first byte to write
 *
 * @param len number of octets to write
 *
 * @return number of octets written, or a negative error code.
 */
int testfs_write_incrementing(struct fs_file_t *fp,
			      u8_t value,
			      unsigned int len);

/** Verify that the file contains a sequence of increasing data.
 *
 * Reads are performed in blocks up to TESTFS_BUFFER_SIZE in length.
 *
 * @param fp pointer to the file to read
 *
 * @param value the expected value of the first byte of data.
 *
 * @param len the number of successive increasing values expected
 *
 * @return the number of times the expected value did appear before
 * len was reached or a mismatch occurred, or a negative error on file
 * read failure.
 */
int testfs_verify_incrementing(struct fs_file_t *fp,
			       u8_t value,
			       unsigned int len);

/** Structure used to describe a filesystem layout. */
struct testfs_bcmd {
	enum fs_dir_entry_type type;
	const char *name;
	u32_t size;
	u8_t value;
	bool matched;
};

/* Specify that a directory named _n is to be created, and all entries
 * up to the next matching EXIT_DIR command are to be created within
 * that directory.
 */
#define TESTFS_BCMD_ENTER_DIR(_n) {	  \
		.type = FS_DIR_ENTRY_DIR, \
		.name = _n,		  \
}

/* Specify that a file named _n is to be created, with _sz bytes of
 * content that starts with _val and increments with each byte.
 */
#define TESTFS_BCMD_FILE(_n, _val, _sz) {  \
		.type = FS_DIR_ENTRY_FILE, \
		.name = _n,		   \
		.size = _sz,		   \
		.value = _val,		   \
}

/* Specify that the content of the previous matching ENTER_DIR is
 * complete and subsequent entries should be created in the parent
 * directory.
 */
#define TESTFS_BCMD_EXIT_DIR(_n) {	  \
		.type = FS_DIR_ENTRY_DIR, \
}

/* Specify that all build commands have been provided. */
#define TESTFS_BCMD_END() { \
}

#define TESTFS_BCMD_IS_ENTER_DIR(cp) (((cp)->type == FS_DIR_ENTRY_DIR) \
				      && ((cp)->name != NULL))
#define TESTFS_BCMD_IS_EXIT_DIR(cp) (((cp)->type == FS_DIR_ENTRY_DIR) \
				     && ((cp)->name == NULL))
#define TESTFS_BCMD_IS_FILE(cp) (((cp)->type == FS_DIR_ENTRY_FILE) \
				 && ((cp)->name != NULL))
#define TESTFS_BCMD_IS_END(cp) (((cp)->type == FS_DIR_ENTRY_FILE) \
				&& ((cp)->name == NULL))

/** Create a file system hierarchy.
 *
 * If an error is returned the
 *
 * @param root a path to the directory in which the hierarchy will be
 * created.  If an error is returned the content of this may identify
 * where the problem occurred.
 *
 * @param cp pointer to a sequence of commands that specify the
 * directory layout.
 *
 * @return Zero after successfully building the hierarchy, or a
 * negative errno code.
 */
int testfs_build(struct testfs_path *root,
		 const struct testfs_bcmd *cp);

/** Get the end range pointer for a sequence of build commands.
 *
 * @param cp a command within the range
 *
 * @return a pointer to the END command in the range.
 */
static inline struct testfs_bcmd *testfs_bcmd_end(struct testfs_bcmd *cp)
{
	while (!TESTFS_BCMD_IS_END(cp)) {
		++cp;
	}
	return cp;
}

/** Verify file system contents against build commands.
 *
 * The specified directory is opened and its contents recursively
 * compared against the build commands for name, size, and content.
 * Build command record entries that are that are matched will have
 * their matched flag set; unmatched entries will have a cleared
 * matched flag.
 *
 * @param pp the path to a directory in the file system.
 *
 * @param scp the first build command relevant to the directory.
 *
 * @param ecp the exclusive last entry in the sequence of build
 * commands relevant to the directory.
 *
 * @return the number of file system entries found that did not match
 * build command content, or a negative error code on compare or file
 * system failures.
 */
int testfs_bcmd_verify_layout(struct testfs_path *pp,
			      struct testfs_bcmd *scp,
			      struct testfs_bcmd *ecp);

/** Search a build command range for a match to an entry.
 *
 * A match is identified if the directory entry type, name, and size
 * match between the build command and the file status.  The content
 * of the file is not verified.
 *
 * @param statp pointer to a file system entry
 *
 * @param scp pointer to the first build command associated with the
 * hierarchy level of the entry.
 *
 * @param ecp pointer to the exclusive last entry in the build command
 * range.
 *
 * @return a pointer to a build command that matches in type and name,
 * or a null pointer if no match is found at the top level.
 */
struct testfs_bcmd *testfs_bcmd_find(struct fs_dirent *statp,
				     struct testfs_bcmd *scp,
				     struct testfs_bcmd *ecp);

/** Find the exit dir command that balances the enter dir command.
 *
 * @param cp pointer to an ENTER_DIR command.
 *
 * @param ecp pointer to the exclusive range end of this level of the
 * hierarchy.
 *
 * @return a pointer to the paired EXIT_DIR command.
 */
struct testfs_bcmd *testfs_bcmd_exitdir(struct testfs_bcmd *cp,
					struct testfs_bcmd *ecp);

#endif /* _ZEPHYR_TESTS_SUBSYS_FS_LITTLEFS_TESTFS_UTIL_H_ */
