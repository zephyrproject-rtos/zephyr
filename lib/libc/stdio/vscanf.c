/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdarg.h>

FILE *vscanf(const char *format, va_list args)
{
    return vfscanf(stdin, format, args);
}
<<<<<<< HEAD

=======
>>>>>>> c64262e9bea (libc: implement vscanf())
