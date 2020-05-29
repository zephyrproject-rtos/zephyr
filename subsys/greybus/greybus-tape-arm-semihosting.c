/*
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Fabien Parent <fparent@baylibre.com>
 */

#include <errno.h>
#include <arch/arm/semihosting.h>
#include <greybus/tape.h>

static ssize_t gb_tape_write(int fd, const void *data, size_t size)
{
    return semihosting_write(fd, data, size);
}

static ssize_t gb_tape_read(int fd, void *data, size_t size)
{
    return semihosting_read(fd, data, size);
}

static int gb_tape_open(const char *tape, int mode)
{
    int semihosting_mode;

    switch (mode) {
    case GB_TAPE_RDONLY:
        semihosting_mode = SEMIHOSTING_RDONLY;
        break;

    case GB_TAPE_WRONLY:
        semihosting_mode = SEMIHOSTING_WRONLY;
        break;

    default:
        return -EINVAL;
    }

    return semihosting_open(tape, semihosting_mode);
}

static void gb_tape_close(int fd)
{
    semihosting_close(fd);
}

static struct gb_tape_mechanism gb_tape_arm_semihosting = {
    .open = gb_tape_open,
    .close = gb_tape_close,
    .write = gb_tape_write,
    .read = gb_tape_read,
};

int gb_tape_arm_semihosting_register(void)
{
    return gb_tape_register_mechanism(&gb_tape_arm_semihosting);
}
