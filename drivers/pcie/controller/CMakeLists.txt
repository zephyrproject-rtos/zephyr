# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_PCIE_HOST)
  # zephyr-keep-sorted-start
  zephyr_library_sources_ifdef(CONFIG_PCIE_BRCMSTB pcie_brcmstb.c)
  zephyr_library_sources_ifdef(CONFIG_PCIE_ECAM pcie_ecam.c)
  # zephyr-keep-sorted-stop
endif()

if(CONFIG_PCIE_ENDPOINT)
  # zephyr-keep-sorted-start
  zephyr_library_sources_ifdef(CONFIG_PCIE_EP_IPROC pcie_ep_iproc.c pcie_ep_iproc_msi.c)
  # zephyr-keep-sorted-stop
endif()
