# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc_config.c
  )

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${SOC_DIR}/${ARCH}/${SOC_PATH}/cec1702-image.py
  --img0-image ${PROJECT_BINARY_DIR}/zephyr.bin
  --image-out  ${PROJECT_BINARY_DIR}/zephyr_spi_image.bin
)
