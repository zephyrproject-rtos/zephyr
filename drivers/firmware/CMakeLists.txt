# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory(nrf_ironside)
add_subdirectory_ifdef(CONFIG_ARM_SCMI scmi)
# zephyr-keep-sorted-stop
