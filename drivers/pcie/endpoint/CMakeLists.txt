# SPDX-License-Identifier: Apache-2.0

zephyr_sources(pcie_ep_common.c)

zephyr_sources_ifdef(CONFIG_PCIE_EP_BCM_IPROC pcie_ep_bcm_iproc.c)
