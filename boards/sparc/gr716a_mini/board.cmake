# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM tsim)

find_program(TSIM tsim-leon3)

set(TSIM_SYS -freq  20 -gr716)
