/*
 * Copyright (c) 2025 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Assembly-safe errno definitions
 *
 * This header provides errno constants that can be safely included
 * in assembly (.S) files. Unlike the standard errno.h, this file
 * contains only preprocessor macros without any C-specific syntax
 * such as function declarations, typedefs, or extern declarations.
 */

#ifndef ZEPHYR_INCLUDE_SYS_ERRNO_ASM_H_
#define ZEPHYR_INCLUDE_SYS_ERRNO_ASM_H_

#if defined(_ASMLANGUAGE)

/*
 * POSIX Error codes - positive values
 * Based on IEEE Std 1003.1-2017
 */

#define EPERM            1   /**< Not owner */
#define ENOENT           2   /**< No such file or directory */
#define ESRCH            3   /**< No such context */
#define EINTR            4   /**< Interrupted system call */
#define EIO              5   /**< I/O error */
#define ENXIO            6   /**< No such device or address */
#define E2BIG            7   /**< Arg list too long */
#define ENOEXEC          8   /**< Exec format error */
#define EBADF            9   /**< Bad file number */
#define ECHILD           10  /**< No children */
#define EAGAIN           11  /**< No more contexts */
#define ENOMEM           12  /**< Not enough core */
#define EACCES           13  /**< Permission denied */
#define EFAULT           14  /**< Bad address */
#define ENOTBLK          15  /**< Block device required */
#define EBUSY            16  /**< Mount device busy */
#define EEXIST           17  /**< File exists */
#define EXDEV            18  /**< Cross-device link */
#define ENODEV           19  /**< No such device */
#define ENOTDIR          20  /**< Not a directory */
#define EISDIR           21  /**< Is a directory */
#define EINVAL           22  /**< Invalid argument */
#define ENFILE           23  /**< File table overflow */
#define EMFILE           24  /**< Too many open files */
#define ENOTTY           25  /**< Not a typewriter */
#define ETXTBSY          26  /**< Text file busy */
#define EFBIG            27  /**< File too large */
#define ENOSPC           28  /**< No space left on device */
#define ESPIPE           29  /**< Illegal seek */
#define EROFS            30  /**< Read-only file system */
#define EMLINK           31  /**< Too many links */
#define EPIPE            32  /**< Broken pipe */
#define EDOM             33  /**< Argument too large */
#define ERANGE           34  /**< Result too large */
#define ENOMSG           35  /**< Unexpected message type */
#define EDEADLK          45  /**< Resource deadlock avoided */
#define ENOLCK           46  /**< No locks available */
#define ENOSTR           60  /**< STREAMS device required */
#define ENODATA          61  /**< Missing expected message data */
#define ETIME            62  /**< STREAMS timeout occurred */
#define ENOSR            63  /**< Insufficient memory */
#define EPROTO           71  /**< Generic STREAMS error */
#define EBADMSG          77  /**< Invalid STREAMS message */
#define ENOSYS           88  /**< Function not implemented */
#define ENOTEMPTY        90  /**< Directory not empty */
#define ENAMETOOLONG     91  /**< File name too long */
#define ELOOP            92  /**< Too many levels of symbolic links */
#define EOPNOTSUPP       95  /**< Operation not supported on socket */
#define EPFNOSUPPORT     96  /**< Protocol family not supported */
#define ECONNRESET       104 /**< Connection reset by peer */
#define ENOBUFS          105 /**< No buffer space available */
#define EAFNOSUPPORT     106 /**< Addr family not supported */
#define EPROTOTYPE       107 /**< Protocol wrong type for socket */
#define ENOTSOCK         108 /**< Socket operation on non-socket */
#define ENOPROTOOPT      109 /**< Protocol not available */
#define ESHUTDOWN        110 /**< Can't send after socket shutdown */
#define ECONNREFUSED     111 /**< Connection refused */
#define EADDRINUSE       112 /**< Address already in use */
#define ECONNABORTED     113 /**< Software caused connection abort */
#define ENETUNREACH      114 /**< Network is unreachable */
#define ENETDOWN         115 /**< Network is down */
#define ETIMEDOUT        116 /**< Connection timed out */
#define EHOSTDOWN        117 /**< Host is down */
#define EHOSTUNREACH     118 /**< No route to host */
#define EINPROGRESS      119 /**< Operation now in progress */
#define EALREADY         120 /**< Operation already in progress */
#define EDESTADDRREQ     121 /**< Destination address required */
#define EMSGSIZE         122 /**< Message size */
#define EPROTONOSUPPORT  123 /**< Protocol not supported */
#define ESOCKTNOSUPPORT  124 /**< Socket type not supported */
#define EADDRNOTAVAIL    125 /**< Can't assign requested address */
#define ENETRESET        126 /**< Network dropped connection on reset */
#define EISCONN          127 /**< Socket is already connected */
#define ENOTCONN         128 /**< Socket is not connected */
#define ETOOMANYREFS     129 /**< Too many references: can't splice */
#define ENOTSUP          134 /**< Unsupported value */
#define EILSEQ           138 /**< Illegal byte sequence */
#define EOVERFLOW        139 /**< Value overflow */
#define ECANCELED        140 /**< Operation canceled */

#define EWOULDBLOCK      EAGAIN /**< Operation would block */

/*
 * Negative errno values for assembly use
 * These replace the need for global variables like _k_neg_eagain
 */

#define NEG_EPERM            (-1)
#define NEG_ENOENT           (-2)
#define NEG_ESRCH            (-3)
#define NEG_EINTR            (-4)
#define NEG_EIO              (-5)
#define NEG_ENXIO            (-6)
#define NEG_E2BIG            (-7)
#define NEG_ENOEXEC          (-8)
#define NEG_EBADF            (-9)
#define NEG_ECHILD           (-10)
#define NEG_EAGAIN           (-11)
#define NEG_ENOMEM           (-12)
#define NEG_EACCES           (-13)
#define NEG_EFAULT           (-14)
#define NEG_ENOTBLK          (-15)
#define NEG_EBUSY            (-16)
#define NEG_EEXIST           (-17)
#define NEG_EXDEV            (-18)
#define NEG_ENODEV           (-19)
#define NEG_ENOTDIR          (-20)
#define NEG_EISDIR           (-21)
#define NEG_EINVAL           (-22)
#define NEG_ENFILE           (-23)
#define NEG_EMFILE           (-24)
#define NEG_ENOTTY           (-25)
#define NEG_ETXTBSY          (-26)
#define NEG_EFBIG            (-27)
#define NEG_ENOSPC           (-28)
#define NEG_ESPIPE           (-29)
#define NEG_EROFS            (-30)
#define NEG_EMLINK           (-31)
#define NEG_EPIPE            (-32)
#define NEG_EDOM             (-33)
#define NEG_ERANGE           (-34)
#define NEG_ENOMSG           (-35)
#define NEG_EDEADLK          (-45)
#define NEG_ENOLCK           (-46)
#define NEG_ENOSTR           (-60)
#define NEG_ENODATA          (-61)
#define NEG_ETIME            (-62)
#define NEG_ENOSR            (-63)
#define NEG_EPROTO           (-71)
#define NEG_EBADMSG          (-77)
#define NEG_ENOSYS           (-88)
#define NEG_ENOTEMPTY        (-90)
#define NEG_ENAMETOOLONG     (-91)
#define NEG_ELOOP            (-92)
#define NEG_EOPNOTSUPP       (-95)
#define NEG_EPFNOSUPPORT     (-96)
#define NEG_ECONNRESET       (-104)
#define NEG_ENOBUFS          (-105)
#define NEG_EAFNOSUPPORT     (-106)
#define NEG_EPROTOTYPE       (-107)
#define NEG_ENOTSOCK         (-108)
#define NEG_ENOPROTOOPT      (-109)
#define NEG_ESHUTDOWN        (-110)
#define NEG_ECONNREFUSED     (-111)
#define NEG_EADDRINUSE       (-112)
#define NEG_ECONNABORTED     (-113)
#define NEG_ENETUNREACH      (-114)
#define NEG_ENETDOWN         (-115)
#define NEG_ETIMEDOUT        (-116)
#define NEG_EHOSTDOWN        (-117)
#define NEG_EHOSTUNREACH     (-118)
#define NEG_EINPROGRESS      (-119)
#define NEG_EALREADY         (-120)
#define NEG_EDESTADDRREQ     (-121)
#define NEG_EMSGSIZE         (-122)
#define NEG_EPROTONOSUPPORT  (-123)
#define NEG_ESOCKTNOSUPPORT  (-124)
#define NEG_EADDRNOTAVAIL    (-125)
#define NEG_ENETRESET        (-126)
#define NEG_EISCONN          (-127)
#define NEG_ENOTCONN         (-128)
#define NEG_ETOOMANYREFS     (-129)
#define NEG_ENOTSUP          (-134)
#define NEG_EILSEQ           (-138)
#define NEG_EOVERFLOW        (-139)
#define NEG_ECANCELED        (-140)

#define NEG_EWOULDBLOCK      NEG_EAGAIN

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_SYS_ERRNO_ASM_H_ */
