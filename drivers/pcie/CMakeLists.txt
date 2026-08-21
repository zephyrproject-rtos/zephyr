# SPDX-FileCopyrightText: Copyright (c) 2019 Intel Corporation
# SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(controller)
add_subdirectory_ifdef(CONFIG_PCIE_HOST host)
add_subdirectory_ifdef(CONFIG_PCIE_ENDPOINT endpoint)
