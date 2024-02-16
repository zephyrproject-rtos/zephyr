# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(intel_cyclonev)
board_set_debugger_ifnset(intel_cyclonev)

if(OPENOCD_USE_LOAD_IMAGE)
  set_ifndef(OPENOCD_FLASH load_image)
else()
  set_ifndef(OPENOCD_FLASH "flash write_image erase")
endif()

set(OPENOCD_CMD_LOAD_DEFAULT "${OPENOCD_FLASH}")
set(OPENOCD_CMD_VERIFY_DEFAULT "verify_image")

board_finalize_runner_args(intel_cyclonev
  --cmd-load "${OPENOCD_CMD_LOAD_DEFAULT}"
  --cmd-verify "${OPENOCD_CMD_VERIFY_DEFAULT}"
  )
