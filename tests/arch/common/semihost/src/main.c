/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/arch/common/semihost.h>

ZTEST(semihost, test_file_ops)
{
	const char *test_file = "./test.bin";
	uint8_t w_buffer[16] = { 1, 2, 3, 4, 5 };
	uint8_t r_buffer[16];
	long read, fd;

	/* Open in write mode */
	fd = semihost_open(test_file, SEMIHOST_OPEN_WB);
	zassert_true(fd > 0, "Bad handle (%d)", fd);
	zassert_equal(semihost_flen(fd), 0, "File not empty");

	/* Write some data */
	zassert_equal(semihost_write(fd, w_buffer, sizeof(w_buffer)), 0, "Write failed");
	zassert_equal(semihost_flen(fd), sizeof(w_buffer), "Size not updated");
	zassert_equal(semihost_write(fd, w_buffer, sizeof(w_buffer)), 0, "Write failed");
	zassert_equal(semihost_flen(fd), 2 * sizeof(w_buffer), "Size not updated");

	/* Reading should fail in this mode */
	read = semihost_read(fd, r_buffer, sizeof(r_buffer));
	zassert_equal(read, -EIO, "Read from write-only file");

	/* Close the file */
	zassert_equal(semihost_close(fd), 0, "Close failed");

	/* Open the same file again for reading */
	fd = semihost_open(test_file, SEMIHOST_OPEN_RB);
	zassert_true(fd > 0, "Bad handle (%d)", fd);
	zassert_equal(semihost_flen(fd), 2 * sizeof(w_buffer), "Data not preserved");

	/* Check reading data */
	read = semihost_read(fd, r_buffer, sizeof(r_buffer));
	zassert_equal(read, sizeof(r_buffer), "Read failed %d", read);
	zassert_mem_equal(r_buffer, w_buffer, sizeof(r_buffer), "Data not read");
	read = semihost_read(fd, r_buffer, sizeof(r_buffer));
	zassert_equal(read, sizeof(r_buffer), "Read failed");
	zassert_mem_equal(r_buffer, w_buffer, sizeof(r_buffer), "Data not read");

	/* Read past end of file */
	read = semihost_read(fd, r_buffer, sizeof(r_buffer));
	zassert_equal(read, -EIO, "Read past end of file");

	/* Seek to file offset */
	zassert_equal(semihost_seek(fd, 1), 0, "Seek failed");

	/* Read from offset */
	read = semihost_read(fd, r_buffer, sizeof(r_buffer) - 1);
	zassert_equal(read, sizeof(r_buffer) - 1, "Read failed");
	zassert_mem_equal(r_buffer, w_buffer + 1, sizeof(r_buffer) - 1, "Data not read");

	/* Close the file */
	zassert_equal(semihost_close(fd), 0, "Close failed");

	/* Opening again in write mode should erase the file */
	fd = semihost_open(test_file, SEMIHOST_OPEN_WB);
	zassert_true(fd > 0, "Bad handle (%d)", fd);
	zassert_equal(semihost_flen(fd), 0, "File not empty");
	zassert_equal(semihost_close(fd), 0, "Close failed");
}

ZTEST_SUITE(semihost, NULL, NULL, NULL, NULL, NULL);
