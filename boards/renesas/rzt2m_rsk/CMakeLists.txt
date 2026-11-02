# SPDX-FileCopyrightText: Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CODE_DATA_RELOCATION AND CONFIG_NET_L2_ETHERNET AND NOT CONFIG_NET_RAW_MODE)
  zephyr_code_relocate(LIBRARY subsys__net__l2__ethernet LOCATION ATCM_TEXT NOKEEP)
  zephyr_code_relocate(LIBRARY subsys__net__ip LOCATION ATCM_TEXT NOKEEP)
  zephyr_code_relocate(LIBRARY subsys__net LOCATION ATCM_TEXT NOKEEP)
  zephyr_code_relocate_ifdef(CONFIG_ETH_DRIVER LIBRARY drivers__ethernet LOCATION ATCM_TEXT NOKEEP)
endif()
