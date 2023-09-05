# Copyright (c) 2024 Analog Devices, Inc.
# Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_WSEN_HIDS_2525020210002 wsen_hids_2525020210002)
add_subdirectory_ifdef(CONFIG_WSEN_PADS_2511020213301 wsen_pads_2511020213301)
add_subdirectory_ifdef(CONFIG_WSEN_PDUS_25131308XXXXX wsen_pdus_25131308XXXXX)
add_subdirectory_ifdef(CONFIG_WSEN_TIDS_2521020222501 wsen_tids_2521020222501)
# zephyr-keep-sorted-stop
