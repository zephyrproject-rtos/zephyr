set_ifndef(BOARD_FLASH_RUNNER pyocd)
set_ifndef(BOARD_DEBUG_RUNNER pyocd)
board_finalize_runner_args(pyocd) # No default arguments to provide.
