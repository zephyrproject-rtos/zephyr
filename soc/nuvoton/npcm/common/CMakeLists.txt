#
# Copyright (c) 2024 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

set(NPCM_BIN_NAME ${CONFIG_KERNEL_BIN_NAME}.npcm.bin)
string(TOUPPER "${SOC_NAME}" soc_name_upper)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${SOC_${soc_name_upper}_DIR}/common/esiost/esiost.py
  -i ${KERNEL_BIN_NAME}
  -o ${NPCM_BIN_NAME}
  -v
  )
