.. _error_code_conventions:

Return Codes
************

Zephyr uses the standard codes in :file:`errno.h` for all APIs.

As a general rule, ``0`` indicates success; a negative errno.h code indicates
an error condition. The table below shows the error code conventions based on
device driver use cases, but they can also be applied to other kernel
components.

+-----------------+------------------------------------------------+
| Code            | Meaning                                        |
+=================+================================================+
| 0               | Success.                                       |
+-----------------+------------------------------------------------+
| -EIO            | General failure.                               |
+-----------------+------------------------------------------------+
| -ENOTSUP        | Operation is not supported or operation is     |
|                 | invalid.                                       |
+-----------------+------------------------------------------------+
| -EINVAL         | Device configuration is not valid or function  |
|                 | argument is not valid.                         |
+-----------------+------------------------------------------------+
| -EBUSY          | Device controller is busy.                     |
+-----------------+------------------------------------------------+
| -EACCES         | Device controller is not accessible.           |
+-----------------+------------------------------------------------+
| -ENODEV         | Device type is not supported.                  |
+-----------------+------------------------------------------------+
| -EPERM          | Device is not configured or operation is not   |
|                 | permitted.                                     |
+-----------------+------------------------------------------------+
| -ENOSYS         | Function is not implemented.                   |
+-----------------+------------------------------------------------+
