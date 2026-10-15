# SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

zephyr_link_libraries($<TARGET_PROPERTY:linker,no_warn_rwx_segments>)
