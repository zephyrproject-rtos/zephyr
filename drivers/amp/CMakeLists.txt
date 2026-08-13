# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/amp.h)

zephyr_library()

# OpenAMP *_ops implementations
zephyr_library_sources_ifdef(CONFIG_AMP_OPENAMP_OPS_PROVIDE_REMOTEPROC amp_openamp_rproc_ops.c)
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_AMP_OPENAMP_OPS_PROVIDE_STORAGE_OPS_MEMCPY amp_openamp_storage_ops_memcpy.c)
# zephyr-keep-sorted-stop

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_USERSPACE amp_handlers.c)
# zephyr-keep-sorted-stop

# AMP driver implementations
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_AMP_TI_K3 amp_ti_k3.c)
# zephyr-keep-sorted-stop
