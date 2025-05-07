# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ARM_SCMI scmi)
add_subdirectory_ifdef(CONFIG_NRF_IRONSIDE nrf_ironside)
# zephyr-keep-sorted-stop
