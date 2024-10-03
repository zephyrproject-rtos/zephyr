/*
 * LICENSE - WIP
 *
 *
 */


#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_XSTDVTABLES_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_XSTDVTABLES_H_

#include <zephyr/sys/fdtable.h>

extern const struct fd_op_vtable stdin_fd_op_vtable; 
extern const struct fd_op_vtable stdout_fd_op_vtable;
extern const struct fd_op_vtable stderr_fd_op_vtable; 

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_XSTDVTABLES_H_ */
