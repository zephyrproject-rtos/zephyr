# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS tsim)

find_program(TSIM tsim-leon3)

set(TSIM_SYS -freq  20 -gr716)
