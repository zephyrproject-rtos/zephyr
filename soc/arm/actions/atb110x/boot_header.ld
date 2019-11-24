/*
 * Copyright (c) 2019 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = 0xc0;
LONG(0x00425441); /* '0', 'B', 'T', 'A' */
LONG(_image_rom_start);
LONG(_image_rom_end + (__data_ram_end - __data_ram_start));
LONG(z_arm_reset + 1);
LONG(0x0);
. = 0xe0;
