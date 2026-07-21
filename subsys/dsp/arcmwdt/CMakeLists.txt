# Copyright (c) 2022 Synopsys
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(public)

zephyr_include_directories_ifdef(CONFIG_DSP_BACKEND_ARCMWDT
  ${ARCMWDT_TOOLCHAIN_PATH}/MetaWare/arc/lib/src/dsp/include/
)
