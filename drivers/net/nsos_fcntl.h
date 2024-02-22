/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRIVERS_NET_NSOS_FCNTL_H__
#define __DRIVERS_NET_NSOS_FCNTL_H__

#define NSOS_MID_O_RDONLY	00
#define NSOS_MID_O_WRONLY	01
#define NSOS_MID_O_RDWR		02

#define NSOS_MID_O_APPEND	0x0400
#define NSOS_MID_O_EXCL		0x0800
#define NSOS_MID_O_NONBLOCK	0x4000

int fl_to_nsos_mid(int flags);
int fl_to_nsos_mid_strict(int flags);
int fl_from_nsos_mid(int flags);

#endif /* __DRIVERS_NET_NSOS_FCNTL_H__ */
