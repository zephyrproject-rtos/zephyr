# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

# The RT2660 Neutron driver blob references the MCUX SDK debug console and
# cache maintenance helpers with no Zephyr counterpart. The MCXN and RT700
# blobs define their own weak versions, so the shims are RT2660-only.
if(CONFIG_NXP_NEUTRON)
  zephyr_library_sources(middleware/neutron/neutron_shim.c)
endif()
