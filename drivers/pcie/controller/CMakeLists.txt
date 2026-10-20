# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_PCIE_BRCMSTB pcie_brcmstb.c)
zephyr_library_sources_ifdef(CONFIG_PCIE_ECAM pcie_ecam.c)
# zephyr-keep-sorted-stop
