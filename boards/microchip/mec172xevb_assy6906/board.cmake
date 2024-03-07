# SPDX-License-Identifier: Apache-2.0

set(SPI_IMAGE_NAME spi_image.bin)

board_set_flasher_ifnset(dediprog)

# --vcc=0 - use 3.5V to flash
board_finalize_runner_args(dediprog
  "--spi-image=${PROJECT_BINARY_DIR}/${SPI_IMAGE_NAME}"
  "--vcc=0"
)

# This allows a custom script to be used for flashing the SPI chip.
include(${ZEPHYR_BASE}/boards/common/misc.board.cmake)
