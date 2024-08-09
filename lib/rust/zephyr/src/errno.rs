pub type ErrnoResult<T> = Result<T, u32>;

pub fn check_result(result: core::ffi::c_int) -> ErrnoResult<()> {
    if result >= 0 {
        Ok(())
    } else {
        Err(-result as u32)
    }
}

pub fn check_value(result: core::ffi::c_int) -> ErrnoResult<u32> {
    if result >= 0 {
        Ok(result as u32)
    } else {
        Err(-result as u32)
    }
}

pub fn check_ptr<T>(result: *const T, null_error: u32) -> ErrnoResult<*const T> {
    if result == core::ptr::null() {
        Err(null_error)
    } else {
        Ok(result)
    }
}

pub fn check_ptr_mut<T>(result: *mut T, null_error: u32) -> ErrnoResult<*mut T> {
    if result == core::ptr::null_mut() {
        Err(null_error)
    } else {
        Ok(result)
    }
}

// definitions from minimal libc
pub const EPERM: u32 = 1;
pub const ENOENT: u32 = 2;
pub const ESRCH: u32 = 3;
pub const EINTR: u32 = 4;
pub const EIO: u32 = 5;
pub const ENXIO: u32 = 6;
pub const E2BIG: u32 = 7;
pub const ENOEXEC: u32 = 8;
pub const EBADF: u32 = 9;
pub const ECHILD: u32 = 10;
pub const EAGAIN: u32 = 11;
pub const ENOMEM: u32 = 12;
pub const EACCES: u32 = 13;
pub const EFAULT: u32 = 14;
pub const ENOTBLK: u32 = 15;
pub const EBUSY: u32 = 16;
pub const EEXIST: u32 = 17;
pub const EXDEV: u32 = 18;
pub const ENODEV: u32 = 19;
pub const ENOTDIR: u32 = 20;
pub const EISDIR: u32 = 21;
pub const EINVAL: u32 = 22;
pub const ENFILE: u32 = 23;
pub const EMFILE: u32 = 24;
pub const ENOTTY: u32 = 25;
pub const ETXTBSY: u32 = 26;
pub const EFBIG: u32 = 27;
pub const ENOSPC: u32 = 28;
pub const ESPIPE: u32 = 29;
pub const EROFS: u32 = 30;
pub const EMLINK: u32 = 31;
pub const EPIPE: u32 = 32;
pub const EDOM: u32 = 33;
pub const ERANGE: u32 = 34;
pub const ENOMSG: u32 = 35;
pub const EDEADLK: u32 = 45;
pub const ENOLCK: u32 = 46;
pub const ENOSTR: u32 = 60;
pub const ENODATA: u32 = 61;
pub const ETIME: u32 = 62;
pub const ENOSR: u32 = 63;
pub const EPROTO: u32 = 71;
pub const EBADMSG: u32 = 77;
pub const ENOSYS: u32 = 88;
pub const ENOTEMPTY: u32 = 90;
pub const ENAMETOOLONG: u32 = 91;
pub const ELOOP: u32 = 92;
pub const EOPNOTSUPP: u32 = 95;
pub const EPFNOSUPPORT: u32 = 96;
pub const ECONNRESET: u32 = 104;
pub const ENOBUFS: u32 = 105;
pub const EAFNOSUPPORT: u32 = 106;
pub const EPROTOTYPE: u32 = 107;
pub const ENOTSOCK: u32 = 108;
pub const ENOPROTOOPT: u32 = 109;
pub const ESHUTDOWN: u32 = 110;
pub const ECONNREFUSED: u32 = 111;
pub const EADDRINUSE: u32 = 112;
pub const ECONNABORTED: u32 = 113;
pub const ENETUNREACH: u32 = 114;
pub const ENETDOWN: u32 = 115;
pub const ETIMEDOUT: u32 = 116;
pub const EHOSTDOWN: u32 = 117;
pub const EHOSTUNREACH: u32 = 118;
pub const EINPROGRESS: u32 = 119;
pub const EALREADY: u32 = 120;
pub const EDESTADDRREQ: u32 = 121;
pub const EMSGSIZE: u32 = 122;
pub const EPROTONOSUPPORT: u32 = 123;
pub const ESOCKTNOSUPPORT: u32 = 124;
pub const EADDRNOTAVAIL: u32 = 125;
pub const ENETRESET: u32 = 126;
pub const EISCONN: u32 = 127;
pub const ENOTCONN: u32 = 128;
pub const ETOOMANYREFS: u32 = 129;
pub const ENOTSUP: u32 = 134;
pub const EILSEQ: u32 = 138;
pub const EOVERFLOW: u32 = 139;
pub const ECANCELED: u32 = 140;

pub const EWOULDBLOCK: u32 = EAGAIN;
