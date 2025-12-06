# Copyright (c) 2024 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_INTC_ARM_V7M_NVIC intc_arm_v7m_nvic)
add_subdirectory_ifdef(CONFIG_INTC_ARM_V8M_NVIC intc_arm_v8m_nvic)
