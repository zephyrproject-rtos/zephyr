#ifndef __ALT_DEV_H__
#define __ALT_DEV_H__

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

#include "system.h"
#include "sys/alt_llist.h"
#include "priv/alt_dev_llist.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The value ALT_IRQ_NOT_CONNECTED is used to represent an unconnected 
 * interrupt line. It cannot evaluate to a valid interrupt number.
 */

#define ALT_IRQ_NOT_CONNECTED (-1)

typedef struct alt_dev_s alt_dev;

struct stat;

/*
 * The file descriptor structure definition.
 */

typedef struct alt_fd_s
{
  alt_dev* dev;
  alt_u8*  priv;
  int      fd_flags;
} alt_fd;

/* 
 * The device structure definition. 
 */
 
struct alt_dev_s {
  alt_llist    llist;     /* for internal use */
  const char*  name; 
  int (*open)  (alt_fd* fd, const char* name, int flags, int mode);
  int (*close) (alt_fd* fd);
  int (*read)  (alt_fd* fd, char* ptr, int len);
  int (*write) (alt_fd* fd, const char* ptr, int len); 
  int (*lseek) (alt_fd* fd, int ptr, int dir);
  int (*fstat) (alt_fd* fd, struct stat* buf);
  int (*ioctl) (alt_fd* fd, int req, void* arg);
};

/*
 * Functions used to register device for access through the C standard 
 * library.
 *
 * The only difference between alt_dev_reg() and alt_fs_reg() is the 
 * interpretation that open() places on the device name. In the case of
 * alt_dev_reg the device is assumed to be a particular character device,
 * and so there must be an exact match in the name for open to succeed. 
 * In the case of alt_fs_reg() the name of the device is treated as the
 * mount point for a directory, and so any call to open() where the name 
 * is the root of the device filename will succeed. 
 */

extern int alt_fs_reg  (alt_dev* dev); 

static ALT_INLINE int alt_dev_reg (alt_dev* dev)
{
  extern alt_llist alt_dev_list;

  return alt_dev_llist_insert ((alt_dev_llist*) dev, &alt_dev_list);
}

#ifdef __cplusplus
}
#endif
 
#endif /* __ALT_DEV_H__ */
