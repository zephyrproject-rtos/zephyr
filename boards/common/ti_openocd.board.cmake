# Copyright (c) 2026 Texas Instruments Incorporated.
#
# SPDX-License-Identifier: Apache-2.0

# Force TI's openocd
set(OPENOCD OPENOCD-NOTFOUND)
find_program(OPENOCD openocd PATHS $ENV{TI_OPENOCD_INSTALL_DIR}/bin NO_DEFAULT_PATH)
