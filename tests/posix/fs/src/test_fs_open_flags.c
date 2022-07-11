/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fcntl.h>
#include <zephyr/posix/unistd.h>
#include "test_fs.h"

#define THE_FILE FATFS_MNTP"/the_file.txt"

static int test_file_open_flags(void)
{
	int fd = 0;
	int data = 0;
	int ret;

	/* 1 Check opening non-existent without O_CREAT */
	TC_PRINT("Open of non-existent file, flags = 0\n");
	fd = open(THE_FILE, 0);
	if (fd >= 0 || errno != ENOENT) {
		TC_PRINT("Expected fail; fd = %d, errno = %d\n", fd, errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Open on non-existent file, flags = O_RDONLY\n");
	fd = open(THE_FILE, O_RDONLY);
	if (fd >= 0 || errno != ENOENT) {
		TC_PRINT("Expected fail; fd = %d, errno = %d\n", fd, errno);
		close(fd);
		return TC_FAIL;
	}
	TC_PRINT("Open on non-existent file, flags = O_WRONLY\n");
	fd = open(THE_FILE, O_WRONLY);
	if (fd >= 0 || errno != ENOENT) {
		TC_PRINT("Expected fail; fd = %d, errno = %d\n", fd, errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Open on non-existent file, flags = O_RDWR\n");
	fd = open(THE_FILE, O_RDWR);
	if (fd >= 0 || errno != ENOENT) {
		TC_PRINT("Expected fail; fd = %d, errno = %d\n", fd, errno);
		close(fd);
		return TC_FAIL;
	}
	/* end 1 */

	/* 2 Create file for read only, attempt to read, attempt to write */
	TC_PRINT("Open on non-existent file, flags = O_CREAT | O_WRONLY\n");
	fd = open(THE_FILE, O_CREAT | O_WRONLY);
	if (fd < 0) {
		TC_PRINT("Expected success; fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	TC_PRINT("Attempt read file opened with flags = O_CREAT | O_WRONLY\n");
	ret = read(fd, &data, sizeof(data));
	if (ret > 0 || errno != EACCES) {
		TC_PRINT("Expected fail, ret = %d, errno = %d\n", ret, errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write file opened with flags = O_CREAT | O_WRONLY\n");
	ret = write(fd, &data, sizeof(data));
	if (ret <= 0 || errno != EACCES) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 2 */


	/* 3 Attempt read/write operation on file opened with flags = 0 */
	TC_PRINT("Attempt open existing with flags = 0\n");
	fd = open(THE_FILE, 0);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	TC_PRINT("Attempt read file opened with flags = 0\n");
	ret = read(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write file opened with flags = 0\n");
	ret = write(fd, &data, sizeof(data));
	if (ret >= 0 || errno != EACCES) {
		TC_PRINT("Expected fail, ret = %d, errno = %d\n", ret, errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 3 */

	/* 4 Attempt read/write on file opened with flags O_RDONLY */
	/* File does have content after previous tests. */
	TC_PRINT("Attempt open existing with flags = O_RDONLY\n");
	fd = open(THE_FILE, O_RDONLY);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	TC_PRINT("Attempt read file opened with flags = O_RDONLY\n");
	ret = read(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write file opened with flags = O_RDONLY\n");
	ret = write(fd, &data, sizeof(data));
	if (ret >= 0 || errno != EACCES) {
		TC_PRINT("Expected fail, ret = %d, errno = %d\n", ret, errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 4 */

	/* 5 Attempt read/write on file opened with flags O_WRONLY */
	/* File does have content after previous tests. */
	TC_PRINT("Attempt open existing with flags = O_WRONLY\n");
	fd = open(THE_FILE, O_WRONLY);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	TC_PRINT("Attempt read file opened with flags = O_WRONLY\n");
	ret = read(fd, &data, sizeof(data));
	if (ret >= 0 || errno != EACCES) {
		TC_PRINT("Expected fail, ret = %d, errno = %d\n", ret, errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write file opened with flags = O_WRONLY\n");
	ret = write(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 5 */

	/* 6 Attempt read/write on file opened with flags O_APPEND | O_WRONLY */
	TC_PRINT("Attempt open existing with flags = O_APPEND | O_WRONLY\n");
	fd = open(THE_FILE, O_APPEND | O_WRONLY);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	TC_PRINT("Attempt read file opened with flags = O_APPEND | O_WRONLY\n");
	ret = read(fd, &data, sizeof(data));
	if (ret >= 0 || errno != EACCES) {
		TC_PRINT("Expected fail, ret = %d, errno = %d\n", ret, errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write to opened with flags = O_APPEND | O_WRONLY\n");
	ret = write(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 6 */

	/* 7 Attempt read/write on file opened with flags O_APPEND | O_RDWR */
	TC_PRINT("Attempt open existing with flags = O_APPEND | O_RDWR\n");
	fd = open(THE_FILE, O_APPEND | O_RDWR);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}


	TC_PRINT("Attempt read file opened with flags = O_APPEND | O_RDWR\n");
	lseek(fd, 0, SEEK_SET);
	ret = read(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	TC_PRINT("Attempt write file opened with flags = O_APPEND | O_RDWR\n");
	ret = write(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 7 */

	/* 8 Check if appended file is always written at the end */
	TC_PRINT("Attempt write to file opened with O_APPEND | O_RDWR\n");
	/* Clean start */
	unlink(THE_FILE);
	fd = open(THE_FILE, O_CREAT | O_WRONLY);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	ret = write(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);

	fd = open(THE_FILE, O_APPEND | O_RDWR);
	if (fd < 0) {
		TC_PRINT("Expected success, fd = %d, errno = %d\n", fd, errno);
		return TC_FAIL;
	}

	lseek(fd, 0, SEEK_SET);
	ret = write(fd, &data, sizeof(data));
	if (ret < 0) {
		TC_PRINT("Expected success, ret = %d, errno = %d\n", ret,
			 errno);
		close(fd);
		return TC_FAIL;
	}

	ret = lseek(fd, 0, SEEK_END);

	if (ret != (sizeof(data) * 2)) {
		TC_PRINT("Expected file size %zu, ret = %d, errno = %d\n",
			 sizeof(data) * 2, ret, errno);
		close(fd);
		return TC_FAIL;
	}

	close(fd);
	/* end 8 */

	return TC_PASS;
}

/**
 * @brief Test for POSIX open flags
 *
 * @details Test attempts to open file with different combinations of open
 * flags and checks if operations on files are permitted according to flags.
 */
ZTEST(posix_fs_test, test_fs_open_flags)
{
	zassert_true(test_file_open_flags() == TC_PASS, NULL);
}
