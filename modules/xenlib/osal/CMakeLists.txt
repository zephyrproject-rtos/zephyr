# Copyright (c) 2026 EPAM Systems
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/osal .)

# Provide Xen headers to Xenlib through an OS-independent include path.
# Xenlib can include them as <xen/*.h> instead of <zephyr/xen/*.h>.
zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)

zephyr_library_sources(xenlib_osal_atomic.c
                       xenlib_osal_barrier.c
                       xenlib_osal_list.c
                       xenlib_osal_memory.c
                       xenlib_osal_mutex.c
                       xenlib_osal_semaphore.c
                       xenlib_osal_shell.c
                       xenlib_osal_thread.c
                       xenlib_osal_timepoint.c)
