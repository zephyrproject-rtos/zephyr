/*
 * Copyright (c) 2023 Husqvarna AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"

#ifdef CONFIG_FS_FATFS_REENTRANT

#define REENTRANT_TEST_STACK_SIZE 500
#define SEMAPHORE_OP_SUCCESS 0
#define TEST_FILE2 FATFS_MNTP"/tfile2.txt"

void tlock_mutex(void *p1, void *p2, void *p3);
void tfile2_access(void *p1, void *p2, void *p3);

K_THREAD_STACK_DEFINE(tlock_mutex_stack_area, REENTRANT_TEST_STACK_SIZE);
K_THREAD_STACK_DEFINE(tfile2_access_stack_area, REENTRANT_TEST_STACK_SIZE);
struct k_thread tlock_mutex_data;
struct k_thread tfile2_access_data;
struct k_sem mutex_unlocked_sem;
struct k_sem run_non_thread_sem;

static int test_reentrant_access(void)
{
	int res;

	TC_PRINT("\nReentrant tests:\n");
	zassert_ok(k_sem_init(&mutex_unlocked_sem, 0, 1));
	zassert_ok(k_sem_init(&run_non_thread_sem, 0, 1));

	/* Start mutex locking thread */
	k_tid_t tid = k_thread_create(&tlock_mutex_data, tlock_mutex_stack_area,
			K_THREAD_STACK_SIZEOF(tlock_mutex_stack_area),
			tlock_mutex,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Make sure thread was able to lock mutex */
	k_sem_take(&run_non_thread_sem, K_FOREVER);

	/* File open should wait here, as the fs is locked. Therefore, automatic switch back to
	 * thread
	 */
	TC_PRINT("Open file\n");
	res = fs_open(&filep, TEST_FILE, FS_O_CREATE | FS_O_RDWR);
	zassert_ok(res, "Err: File could not be opened [%d]\n", res);
	TC_PRINT("File opened\n");

	/* Check if mutex thread really unlocked the mutexes */
	zassert_equal(SEMAPHORE_OP_SUCCESS, k_sem_take(&mutex_unlocked_sem, K_NO_WAIT),
		      "File open with locked mutex");

	/* Cleanup */
	res = fs_close(&filep);
	zassert_ok(res, "Error closing file [%d]\n", res);
	res = fs_unlink(TEST_FILE);
	zassert_ok(res, "Error deleting file [%d]\n", res);

	k_thread_join(tid, K_FOREVER);

	return res;
}

static int test_reentrant_parallel_file_access(void)
{
	int res;

	TC_PRINT("\nParallel reentrant-safe file access test:\n");

	TC_PRINT("Open file 1\n");
	res = fs_open(&filep, TEST_FILE, FS_O_CREATE | FS_O_RDWR);
	zassert_ok(res, "Err: File 1 could not be opened [%d]\n", res);
	TC_PRINT("File 1 opened 1\n");

	/* Start 2nd file access thread */
	k_tid_t tid = k_thread_create(&tfile2_access_data, tfile2_access_stack_area,
				      K_THREAD_STACK_SIZEOF(tfile2_access_stack_area),
				      tfile2_access,
				      NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Wait for thread to finish accessing file 2 */
	k_thread_join(tid, K_FOREVER);

	/* Check existence of file 2 */
	struct fs_file_t filep2;

	fs_file_t_init(&filep2);

	TC_PRINT("Check file 2 existence\n");
	res = fs_open(&filep2, TEST_FILE2, FA_OPEN_EXISTING | FA_READ);
	zassert_ok(res, "Err: File 2 does not exist [%d]\n", res);

	/* Cleanup */
	res = fs_close(&filep2);
	zassert_ok(res, "Error closing file 2 [%d]\n", res);
	res = fs_unlink(TEST_FILE2);
	zassert_ok(res, "Error deleting file 2 [%d]\n", res);
	res = fs_close(&filep);
	zassert_ok(res, "Error closing file 1 [%d]\n", res);
	res = fs_unlink(TEST_FILE);
	zassert_ok(res, "Error deleting file 1 [%d]\n", res);

	return res;
}

void release_dirty_mutex(void)
{
	ff_mutex_give(fat_fs.ldrv);
}

int request_dirty_mutex(void)
{
	return ff_mutex_take(fat_fs.ldrv);
}

void tlock_mutex(void *p1, void *p2, void *p3)
{
	TC_PRINT("Mutex thread: Started, locking fs\n");
	request_dirty_mutex();
	TC_PRINT("Mutex thread: Lock acquired, yield to switch back to try to open file\n");
	k_sem_give(&run_non_thread_sem);
	k_yield();

	TC_PRINT("Mutex thread: Got back to thread, release mutex now and give semaphore to check "
		 "if file opened\n");
	k_sem_give(&mutex_unlocked_sem);
	release_dirty_mutex();

	TC_PRINT("Mutex thread: Lock released, thread terminating\n");
}

void tfile2_access(void *p1, void *p2, void *p3)
{
	int res;
	ssize_t brw;
	struct fs_file_t filep2;

	TC_PRINT("File 2 access thread started\n");

	/* Init fp for 2nd File for parallel access test */
	fs_file_t_init(&filep2);

	/* open 2nd file */
	TC_PRINT("Open file 2\n");
	res = fs_open(&filep2, TEST_FILE2, FS_O_CREATE | FS_O_RDWR);
	zassert_ok(res, "Err: File 2 could not be opened [%d]\n", res);
	TC_PRINT("File 2 opened 2\n");

	/* Verify fs_write() not locked */
	brw = fs_write(&filep2, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%zd]\n", brw);
		fs_close(&filep2);
		return;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%zd]\n", brw);
		fs_close(&filep2);
		return;
	}

	/* Close file and switch back to test context*/
	res = fs_close(&filep2);
	zassert_ok(res, "Error closing file [%d]\n", res);

	TC_PRINT("File 2 access thread successfully wrote to file 2\n");
}

void test_fat_file_reentrant(void)
{
	zassert_true(test_reentrant_access() == TC_PASS);
	zassert_true(test_reentrant_parallel_file_access() == TC_PASS);
}
#endif  /* CONFIG_FS_FATFS_REENTRANT */
