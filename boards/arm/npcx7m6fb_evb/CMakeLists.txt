#
# Copyright (c) 2020 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

# find ECST tool for generating npcx header used by ROM code

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${BOARD_DIR}/support/ecst.py
  -i ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  -o ${TARGET_IMAGE_FILE} -nohcrc -nofcrc
  -chip npcx7m6 -flashsize 8 -spimaxclk 50 -spireadmode dual
)
