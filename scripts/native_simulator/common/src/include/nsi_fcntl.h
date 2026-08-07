/**
 * Copyright (c) 2023-2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_FCNTL_H
#define NSI_COMMON_SRC_NSI_FCNTL_H

#define NSI_FCNTL_MID_O_RDONLY		00
#define NSI_FCNTL_MID_O_WRONLY		01
#define NSI_FCNTL_MID_O_RDWR		02

#define NSI_FCNTL_MID_O_CREAT		0x0100
#define NSI_FCNTL_MID_O_TRUNC		0x0200
#define NSI_FCNTL_MID_O_APPEND		0x0400
#define NSI_FCNTL_MID_O_EXCL		0x0800
#define NSI_FCNTL_MID_O_NONBLOCK	0x4000

int nsi_fcntl_to_mid(int flags);
int nsi_fcntl_to_mid_strict(int flags);
int nsi_fcntl_from_mid(int flags);

#endif /* NSI_COMMON_SRC_NSI_FCNTL_H */
