set_ifndef(BOARD_FLASH_RUNNER nios2)
set_ifndef(BOARD_DEBUG_RUNNER nios2)

board_finalize_runner_args(nios2
  # TODO: merge this script into nios2.py
  "--quartus-flash=$ENV{ZEPHYR_BASE}/scripts/support/quartus-flash.py"
  )
