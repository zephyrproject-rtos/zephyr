# Copyright (c) 2022-2023, Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(sip_svc_subsys.c)
zephyr_library_sources(sip_svc_id_mgr.c)

zephyr_linker_sources(DATA_SECTIONS sip_svc_data.ld)

if(CONFIG_ARM_SIP_SVC_SUBSYS_SHELL)
  zephyr_library_sources(sip_svc_shell.c)
  zephyr_library_sources_ifdef(CONFIG_ARM_SIP_SVC_HAS_INTEL_SDM_MAILBOX_FIFO sip_svc_agilex_mailbox_shell.c)
endif()
