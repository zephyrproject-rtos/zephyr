/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

Not all platforms have an SoC defined pstate node in their device tree.
For those that wish to use the pstate driver on such platforms, a stub overlay
name 'pstate-sub.overlay' is provided for convenience. To use this overlay,
enable CONFIG_CPU_FREQ_PSTATE_SET_STUB in the prj.conf and use the following
command to build your project.

west build -b <board> -- -DDTC_OVERLAY_FILE=pstate-stub.overlay
