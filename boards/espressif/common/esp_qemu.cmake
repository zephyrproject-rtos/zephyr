# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Opt the board into the Espressif QEMU emu platform. Included from board.cmake
# when CONFIG_ESPRESSIF_QEMU_TARGET=y and not CONFIG_MCUBOOT. Run/debugserver/
# flash-image logic lives in
# cmake/emu/espressif_qemu.cmake.

set(SUPPORTED_EMU_PLATFORMS espressif_qemu)
