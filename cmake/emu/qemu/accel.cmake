# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QEMU_ACCEL_HVF)
  qemu_append_flags(-accel hvf)
endif()
