# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ARM_SIP_SVC_HAS_INTEL_SDM_MAILBOX_FIFO sip_smc_intel_socfpga.c)
