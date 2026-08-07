# Copyright (c) 2025 Nordic Semiconductor
# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

set_property(TARGET linker PROPERTY undefined --undefined=)

# `armlink` does not accept -O* flags so drop flags to avoid build error.
set_property(TARGET linker PROPERTY no_optimization "")
set_property(TARGET linker PROPERTY optimization_debug "")
set_property(TARGET linker PROPERTY optimization_speed "")
set_property(TARGET linker PROPERTY optimization_size "")
set_property(TARGET linker PROPERTY optimization_size_aggressive "")
