/**
 * Copyright (c) 2023-2024 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRIVERS_NET_NSOS_ERRNO_H__
#define __DRIVERS_NET_NSOS_ERRNO_H__

#include <errno.h>

#define NSOS_MID_EPERM 1         /**< Not owner */
#define NSOS_MID_ENOENT 2        /**< No such file or directory */
#define NSOS_MID_ESRCH 3         /**< No such context */
#define NSOS_MID_EINTR 4         /**< Interrupted system call */
#define NSOS_MID_EIO 5           /**< I/O error */
#define NSOS_MID_ENXIO 6         /**< No such device or address */
#define NSOS_MID_E2BIG 7         /**< Arg list too long */
#define NSOS_MID_ENOEXEC 8       /**< Exec format error */
#define NSOS_MID_EBADF 9         /**< Bad file number */
#define NSOS_MID_ECHILD 10       /**< No children */
#define NSOS_MID_EAGAIN 11       /**< No more contexts */
#define NSOS_MID_ENOMEM 12       /**< Not enough core */
#define NSOS_MID_EACCES 13       /**< Permission denied */
#define NSOS_MID_EFAULT 14       /**< Bad address */
#define NSOS_MID_ENOTBLK 15      /**< Block device required */
#define NSOS_MID_EBUSY 16        /**< Mount device busy */
#define NSOS_MID_EEXIST 17       /**< File exists */
#define NSOS_MID_EXDEV 18        /**< Cross-device link */
#define NSOS_MID_ENODEV 19       /**< No such device */
#define NSOS_MID_ENOTDIR 20      /**< Not a directory */
#define NSOS_MID_EISDIR 21       /**< Is a directory */
#define NSOS_MID_EINVAL 22       /**< Invalid argument */
#define NSOS_MID_ENFILE 23       /**< File table overflow */
#define NSOS_MID_EMFILE 24       /**< Too many open files */
#define NSOS_MID_ENOTTY 25       /**< Not a typewriter */
#define NSOS_MID_ETXTBSY 26      /**< Text file busy */
#define NSOS_MID_EFBIG 27        /**< File too large */
#define NSOS_MID_ENOSPC 28       /**< No space left on device */
#define NSOS_MID_ESPIPE 29       /**< Illegal seek */
#define NSOS_MID_EROFS 30        /**< Read-only file system */
#define NSOS_MID_EMLINK 31       /**< Too many links */
#define NSOS_MID_EPIPE 32        /**< Broken pipe */
#define NSOS_MID_EDOM 33         /**< Argument too large */
#define NSOS_MID_ERANGE 34       /**< Result too large */
#define NSOS_MID_ENOMSG 35       /**< Unexpected message type */
#define NSOS_MID_EDEADLK 45      /**< Resource deadlock avoided */
#define NSOS_MID_ENOLCK 46       /**< No locks available */
#define NSOS_MID_ENOSTR 60       /**< STREAMS device required */
#define NSOS_MID_ENODATA 61      /**< Missing expected message data */
#define NSOS_MID_ETIME 62        /**< STREAMS timeout occurred */
#define NSOS_MID_ENOSR 63        /**< Insufficient memory */
#define NSOS_MID_EPROTO 71       /**< Generic STREAMS error */
#define NSOS_MID_EBADMSG 77      /**< Invalid STREAMS message */
#define NSOS_MID_ENOSYS 88       /**< Function not implemented */
#define NSOS_MID_ENOTEMPTY 90    /**< Directory not empty */
#define NSOS_MID_ENAMETOOLONG 91 /**< File name too long */
#define NSOS_MID_ELOOP 92        /**< Too many levels of symbolic links */
#define NSOS_MID_EOPNOTSUPP 95   /**< Operation not supported on socket */
#define NSOS_MID_EPFNOSUPPORT 96 /**< Protocol family not supported */
#define NSOS_MID_ECONNRESET 104   /**< Connection reset by peer */
#define NSOS_MID_ENOBUFS 105      /**< No buffer space available */
#define NSOS_MID_EAFNOSUPPORT 106 /**< Addr family not supported */
#define NSOS_MID_EPROTOTYPE 107   /**< Protocol wrong type for socket */
#define NSOS_MID_ENOTSOCK 108     /**< Socket operation on non-socket */
#define NSOS_MID_ENOPROTOOPT 109  /**< Protocol not available */
#define NSOS_MID_ESHUTDOWN 110    /**< Can't send after socket shutdown */
#define NSOS_MID_ECONNREFUSED 111 /**< Connection refused */
#define NSOS_MID_EADDRINUSE 112   /**< Address already in use */
#define NSOS_MID_ECONNABORTED 113 /**< Software caused connection abort */
#define NSOS_MID_ENETUNREACH 114  /**< Network is unreachable */
#define NSOS_MID_ENETDOWN 115     /**< Network is down */
#define NSOS_MID_ETIMEDOUT 116    /**< Connection timed out */
#define NSOS_MID_EHOSTDOWN 117    /**< Host is down */
#define NSOS_MID_EHOSTUNREACH 118 /**< No route to host */
#define NSOS_MID_EINPROGRESS 119  /**< Operation now in progress */
#define NSOS_MID_EALREADY 120     /**< Operation already in progress */
#define NSOS_MID_EDESTADDRREQ 121 /**< Destination address required */
#define NSOS_MID_EMSGSIZE 122        /**< Message size */
#define NSOS_MID_EPROTONOSUPPORT 123 /**< Protocol not supported */
#define NSOS_MID_ESOCKTNOSUPPORT 124 /**< Socket type not supported */
#define NSOS_MID_EADDRNOTAVAIL 125   /**< Can't assign requested address */
#define NSOS_MID_ENETRESET 126       /**< Network dropped connection on reset */
#define NSOS_MID_EISCONN 127         /**< Socket is already connected */
#define NSOS_MID_ENOTCONN 128        /**< Socket is not connected */
#define NSOS_MID_ETOOMANYREFS 129    /**< Too many references: can't splice */
#define NSOS_MID_ENOTSUP 134         /**< Unsupported value */
#define NSOS_MID_EILSEQ 138          /**< Illegal byte sequence */
#define NSOS_MID_EOVERFLOW 139       /**< Value overflow */
#define NSOS_MID_ECANCELED 140       /**< Operation canceled */

int errno_to_nsos_mid(int err);
int errno_from_nsos_mid(int err);

#endif /* __DRIVERS_NET_NSOS_ERRNO_H__ */
