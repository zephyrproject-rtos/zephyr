/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* This file exists solely to include a single binary blob in a link,
 * used by the qemu kernel file architecture swap code in the cmake
 * configuration.
 */

__asm__(".incbin \"zephyr-qemu.bin\"");
