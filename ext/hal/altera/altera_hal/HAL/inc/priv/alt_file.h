#ifndef __ALT_FILE_H__
#define __ALT_FILE_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A LIBRARY READ-ONLY SOURCE FILE. DO NOT EDIT.                       *
*                                                                             *
******************************************************************************/

#include "sys/alt_dev.h"
#include "sys/alt_llist.h"
#include "os/alt_sem.h"

#include "alt_types.h"

/*
 * This header provides the internal defenitions required to control file 
 * access. These variables and functions are not guaranteed to exist in 
 * future implementations of the HAL.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The function alt_find_dev() is used to search the device list "list" to
 * locate a device named "name". If a match is found, then a pointer to the
 * device is returned, otherwise NULL is returned. 
 */
 
extern alt_dev* alt_find_dev (const char* name, alt_llist* list);

/*
 * alt_find_file() is used to search the list of registered file systems to
 * find the filesystem that the file named "name" belongs to. If a match is
 * found, then a pointer to the filesystems alt_dev structure is returned,
 * otherwise NULL is returned.
 *
 * Note that a match does not indicate that the file exists, only that a 
 * filesystem exists that is registered for a partition that could contain
 * the file. The filesystems open() function would need to be called in order
 * to determine if the file exists.
 */

extern alt_dev* alt_find_file (const char* name);

/*
 * alt_get_fd() is used to allocate a file descriptor for the device or 
 * filesystem "dev". A negative return value indicates an error, otherwise the
 * return value is the index of the file descriptor within the file descriptor
 * pool.
 */

extern int alt_get_fd (alt_dev* dev);

/*
 * alt_release_fd() is called to free the file descriptor with index "fd".
 */

extern void alt_release_fd (int fd);

/*
 * alt_fd_lock() is called by ioctl() to mark the file descriptor "fd" as 
 * being open for exclusive access. Subsequent calls to open() for the device 
 * associated with "fd" will fail. A device is unlocked by either calling 
 * close() for "fd", or by an alternate call to ioctl() (see ioctl.c for 
 * details).
 */

extern int alt_fd_lock (alt_fd* fd);

/*
 * alt_fd_unlock() is called by ioctl() to unlock a descriptor previously 
 * locked by a call to alt_fd_lock().
 */

extern int alt_fd_unlock (alt_fd* fd);

/*
 * "alt_fd_list" is the pool of file descriptors.
 */

extern alt_fd alt_fd_list[];

/*
 * flags used by alt_fd. 
 *
 * ALT_FD_EXCL is used to mark a file descriptor as locked for exclusive
 * access, i.e. further calls to open() for the associated device should
 * fail.
 *
 * ALT_FD_DEV marks a dile descriptor as belonging to a device as oposed to a
 * filesystem. 
 */

#define ALT_FD_EXCL 0x80000000
#define ALT_FD_DEV  0x40000000

#define ALT_FD_FLAGS_MASK (ALT_FD_EXCL | ALT_FD_DEV)

/*
 * "alt_dev_list" is the head of the linked list of registered devices.
 */

extern alt_llist alt_dev_list;

/*
 * "alt_fs_list" is the head of the linked list of registered filesystems.
 */

extern alt_llist alt_fs_list;

/*
 * "alt_fd_list_lock" is a semaphore used to ensure that access to the pool
 * of file descriptors is thread safe.
 */

ALT_EXTERN_SEM(alt_fd_list_lock)

/*
 * "alt_max_fd" is a 'high water mark'. It indicates the highest file 
 * descriptor allocated. Use of this can save searching the entire pool
 * for active file descriptors, which helps avoid contention on access 
 * to the file descriptor pool.
 */
 
extern alt_32 alt_max_fd;

/*
 * alt_io_redirect() is called at startup to redirect stdout, stdin, and 
 * stderr to the devices named in the input arguments. By default these streams
 * are directed at /dev/null, and are then redirected using this function once
 * all of the devices have been registered within the system. 
 */

extern void alt_io_redirect(const char* stdout_dev, 
                            const char* stdin_dev, 
                            const char* stderr_dev);


#ifdef __cplusplus
}
#endif

#endif /* __ALT_FILE_H__ */
