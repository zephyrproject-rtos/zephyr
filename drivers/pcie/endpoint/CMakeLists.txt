# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(pcie_ep_common.c)

zephyr_library_sources_ifdef(
  CONFIG_PCIE_EP_IPROC
  pcie_ep_iproc.c
  pcie_ep_iproc_msi.c
  )
