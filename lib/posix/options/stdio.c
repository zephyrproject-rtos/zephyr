/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/posix/stdio.h>
#include <zephyr/kernel.h>


int fgetc(FILE *stream)
{
    ARG_UNUSED(stream);
    errno = ENOSYS;
    return -1;
}