/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_ERRNO_H
#define NSI_COMMON_SRC_NSI_ERRNO_H

/*
 * Optional utility module to convert errno values from one libC to another,
 * where one libC would normally be the embedded libC used to build the embedded code and the other
 * the host libC.
 *
 * It works by converting the errno values from/to either libC into an intermediate set of values
 * (NSI_ERRNO_MID_E*).
 * When two components would like to exchange errors, they should include nsi_errno.h and build and
 * link to nsi_errno.c in their context. And convert to/from these intermediate error values
 * before passing them to/after receiving them from the other side by using
 * nsi_errno_to_mid() / nsi_errno_from_mid().
 */

#include <errno.h>

#define NSI_ERRNO_MID_EPERM             1 /**< Not owner */
#define NSI_ERRNO_MID_ENOENT            2 /**< No such file or directory */
#define NSI_ERRNO_MID_ESRCH             3 /**< No such context */
#define NSI_ERRNO_MID_EINTR             4 /**< Interrupted system call */
#define NSI_ERRNO_MID_EIO               5 /**< I/O error */
#define NSI_ERRNO_MID_ENXIO             6 /**< No such device or address */
#define NSI_ERRNO_MID_E2BIG             7 /**< Arg list too long */
#define NSI_ERRNO_MID_ENOEXEC           8 /**< Exec format error */
#define NSI_ERRNO_MID_EBADF             9 /**< Bad file number */
#define NSI_ERRNO_MID_ECHILD           10 /**< No children */
#define NSI_ERRNO_MID_EAGAIN           11 /**< No more contexts */
#define NSI_ERRNO_MID_ENOMEM           12 /**< Not enough core */
#define NSI_ERRNO_MID_EACCES           13 /**< Permission denied */
#define NSI_ERRNO_MID_EFAULT           14 /**< Bad address */
#define NSI_ERRNO_MID_ENOTBLK          15 /**< Block device required */
#define NSI_ERRNO_MID_EBUSY            16 /**< Mount device busy */
#define NSI_ERRNO_MID_EEXIST           17 /**< File exists */
#define NSI_ERRNO_MID_EXDEV            18 /**< Cross-device link */
#define NSI_ERRNO_MID_ENODEV           19 /**< No such device */
#define NSI_ERRNO_MID_ENOTDIR          20 /**< Not a directory */
#define NSI_ERRNO_MID_EISDIR           21 /**< Is a directory */
#define NSI_ERRNO_MID_EINVAL           22 /**< Invalid argument */
#define NSI_ERRNO_MID_ENFILE           23 /**< File table overflow */
#define NSI_ERRNO_MID_EMFILE           24 /**< Too many open files */
#define NSI_ERRNO_MID_ENOTTY           25 /**< Not a typewriter */
#define NSI_ERRNO_MID_ETXTBSY          26 /**< Text file busy */
#define NSI_ERRNO_MID_EFBIG            27 /**< File too large */
#define NSI_ERRNO_MID_ENOSPC           28 /**< No space left on device */
#define NSI_ERRNO_MID_ESPIPE           29 /**< Illegal seek */
#define NSI_ERRNO_MID_EROFS            30 /**< Read-only file system */
#define NSI_ERRNO_MID_EMLINK           31 /**< Too many links */
#define NSI_ERRNO_MID_EPIPE            32 /**< Broken pipe */
#define NSI_ERRNO_MID_EDOM             33 /**< Argument too large */
#define NSI_ERRNO_MID_ERANGE           34 /**< Result too large */
#define NSI_ERRNO_MID_ENOMSG           35 /**< Unexpected message type */
#define NSI_ERRNO_MID_EDEADLK          45 /**< Resource deadlock avoided */
#define NSI_ERRNO_MID_ENOLCK           46 /**< No locks available */
#define NSI_ERRNO_MID_ENOSTR           60 /**< STREAMS device required */
#define NSI_ERRNO_MID_ENODATA          61 /**< Missing expected message data */
#define NSI_ERRNO_MID_ETIME            62 /**< STREAMS timeout occurred */
#define NSI_ERRNO_MID_ENOSR            63 /**< Insufficient memory */
#define NSI_ERRNO_MID_EPROTO           71 /**< Generic STREAMS error */
#define NSI_ERRNO_MID_EBADMSG          77 /**< Invalid STREAMS message */
#define NSI_ERRNO_MID_ENOSYS           88 /**< Function not implemented */
#define NSI_ERRNO_MID_ENOTEMPTY        90 /**< Directory not empty */
#define NSI_ERRNO_MID_ENAMETOOLONG     91 /**< File name too long */
#define NSI_ERRNO_MID_ELOOP            92 /**< Too many levels of symbolic links */
#define NSI_ERRNO_MID_EOPNOTSUPP       95 /**< Operation not supported on socket */
#define NSI_ERRNO_MID_EPFNOSUPPORT     96 /**< Protocol family not supported */
#define NSI_ERRNO_MID_ECONNRESET      104 /**< Connection reset by peer */
#define NSI_ERRNO_MID_ENOBUFS         105 /**< No buffer space available */
#define NSI_ERRNO_MID_EAFNOSUPPORT    106 /**< Addr family not supported */
#define NSI_ERRNO_MID_EPROTOTYPE      107 /**< Protocol wrong type for socket */
#define NSI_ERRNO_MID_ENOTSOCK        108 /**< Socket operation on non-socket */
#define NSI_ERRNO_MID_ENOPROTOOPT     109 /**< Protocol not available */
#define NSI_ERRNO_MID_ESHUTDOWN       110 /**< Can't send after socket shutdown */
#define NSI_ERRNO_MID_ECONNREFUSED    111 /**< Connection refused */
#define NSI_ERRNO_MID_EADDRINUSE      112 /**< Address already in use */
#define NSI_ERRNO_MID_ECONNABORTED    113 /**< Software caused connection abort */
#define NSI_ERRNO_MID_ENETUNREACH     114 /**< Network is unreachable */
#define NSI_ERRNO_MID_ENETDOWN        115 /**< Network is down */
#define NSI_ERRNO_MID_ETIMEDOUT       116 /**< Connection timed out */
#define NSI_ERRNO_MID_EHOSTDOWN       117 /**< Host is down */
#define NSI_ERRNO_MID_EHOSTUNREACH    118 /**< No route to host */
#define NSI_ERRNO_MID_EINPROGRESS     119 /**< Operation now in progress */
#define NSI_ERRNO_MID_EALREADY        120 /**< Operation already in progress */
#define NSI_ERRNO_MID_EDESTADDRREQ    121 /**< Destination address required */
#define NSI_ERRNO_MID_EMSGSIZE        122 /**< Message size */
#define NSI_ERRNO_MID_EPROTONOSUPPORT 123 /**< Protocol not supported */
#define NSI_ERRNO_MID_ESOCKTNOSUPPORT 124 /**< Socket type not supported */
#define NSI_ERRNO_MID_EADDRNOTAVAIL   125 /**< Can't assign requested address */
#define NSI_ERRNO_MID_ENETRESET       126 /**< Network dropped connection on reset */
#define NSI_ERRNO_MID_EISCONN         127 /**< Socket is already connected */
#define NSI_ERRNO_MID_ENOTCONN        128 /**< Socket is not connected */
#define NSI_ERRNO_MID_ETOOMANYREFS    129 /**< Too many references: can't splice */
#define NSI_ERRNO_MID_ENOTSUP         134 /**< Unsupported value */
#define NSI_ERRNO_MID_EILSEQ          138 /**< Illegal byte sequence */
#define NSI_ERRNO_MID_EOVERFLOW       139 /**< Value overflow */
#define NSI_ERRNO_MID_ECANCELED       140 /**< Operation canceled */

/* Convert a errno value to the intermediate represetation to pass it to the other side */
int nsi_errno_to_mid(int err);
/* Convert a errno value from the intermediate representation into the local libC value */
int nsi_errno_from_mid(int err);
/* Return the middleground representation of the current host libC errno */
int nsi_get_errno_in_mid(void);
/* Return the local representation of the current host libC errno */
int nsi_host_get_errno(void);

#endif /* NSI_COMMON_SRC_NSI_ERRNO_H */
