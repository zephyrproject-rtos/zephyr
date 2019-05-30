/*
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* From: https://fossies.org/dox/musl-1.1.22/____strerror_8h_source.html */

E(EILSEQ,       "Illegal byte sequence")
E(EDOM,         "Domain error")
E(ERANGE,       "Result not representable")

E(ENOTTY,       "Not a tty")
E(EACCES,       "Permission denied")
E(EPERM,        "Operation not permitted")
E(ENOENT,       "No such file or directory")
E(ESRCH,        "No such process")
E(EEXIST,       "File exists")

E(ENOSPC,       "No space left on device")
E(ENOMEM,       "Out of memory")

E(EBUSY,        "Resource busy")
E(EINTR,        "Interrupted system call")
E(EAGAIN,       "Resource temporarily unavailable")
E(ESPIPE,       "Invalid seek")

E(EXDEV,        "Cross-device link")
E(EROFS,        "Read-only file system")
E(ENOTEMPTY,    "Directory not empty")

E(ECONNRESET,   "Connection reset by peer")
E(ETIMEDOUT,    "Operation timed out")
E(ECONNREFUSED, "Connection refused")
E(EHOSTDOWN,    "Host is down")
E(EHOSTUNREACH, "Host is unreachable")
E(EADDRINUSE,   "Address in use")

E(EPIPE,        "Broken pipe")
E(EIO,          "I/O error")
E(ENXIO,        "No such device or address")
E(ENOTBLK,      "Block device required")
E(ENODEV,       "No such device")
E(ENOTDIR,      "Not a directory")
E(EISDIR,       "Is a directory")
E(ETXTBSY,      "Text file busy")
E(ENOEXEC,      "Exec format error")

E(EINVAL,       "Invalid argument")

E(E2BIG,        "Argument list too long")
E(ELOOP,        "Symbolic link loop")
E(ENAMETOOLONG, "Filename too long")
E(ENFILE,       "Too many open files in system")
E(EMFILE,       "No file descriptors available")
E(EBADF,        "Bad file descriptor")
E(ECHILD,       "No child process")
E(EFAULT,       "Bad address")
E(EFBIG,        "File too large")
E(EMLINK,       "Too many links")
E(ENOLCK,       "No locks available")

E(EDEADLK,      "Resource deadlock would occur")
E(ECANCELED,    "Operation canceled")
E(ENOSYS,       "Function not implemented")
E(ENOMSG,       "No message of desired type")
E(ENOSTR,       "Device not a stream")
E(ENODATA,      "No data available")
E(ETIME,        "Device timeout")
E(ENOSR,        "Out of streams resources")
E(EPROTO,       "Protocol error")
E(EBADMSG,      "Bad message")
E(ENOTSOCK,     "Not a socket")
E(EDESTADDRREQ, "Destination address required")
E(EMSGSIZE,     "Message too large")
E(EPROTOTYPE,   "Protocol wrong type for socket")
E(ENOPROTOOPT,  "Protocol not available")
E(EPROTONOSUPPORT, "Protocol not supported")
E(ESOCKTNOSUPPORT, "Socket type not supported")
E(ENOTSUP,      "Not supported")
E(EPFNOSUPPORT, "Protocol family not supported")
E(EAFNOSUPPORT, "Address family not supported by protocol")
E(EADDRNOTAVAIL, "Address not available")
E(ENETDOWN,     "Network is down")
E(ENETUNREACH,  "Network unreachable")
E(ENETRESET,    "Connection reset by network")
E(ECONNABORTED, "Connection aborted")
E(ENOBUFS,      "No buffer space available")
E(EISCONN,      "Socket is connected")
E(ENOTCONN,     "Socket not connected")
E(ESHUTDOWN,    "Cannot send after socket shutdown")
E(EALREADY,     "Operation already in progress")
E(EINPROGRESS,  "Operation in progress")

E(0,            "No error information")
