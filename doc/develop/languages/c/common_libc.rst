.. _c_library_common:

Common C library code
#####################

Zephyr provides some C library functions that are designed to be used in
conjunction with multiple C libraries. These either provide functions not
available in multiple C libraries or are designed to replace functionality
in the C library with code better suited for use in the Zephyr environment

Time function
*************

This provides an implementation of the standard C function, :c:func:`time`,
relying on the Zephyr function, :c:func:`clock_gettime`. This function can
be enabled by selecting :kconfig:option:`COMMON_LIBC_TIME`.
