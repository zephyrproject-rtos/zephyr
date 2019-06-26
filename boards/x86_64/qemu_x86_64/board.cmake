# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set_target_properties(${ZEPHYR_TARGET} PROPERTIES QEMU_FLAGS_${ARCH} -nographic)
