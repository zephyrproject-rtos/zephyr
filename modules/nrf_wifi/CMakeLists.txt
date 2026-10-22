#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_WIFI_NRF70 os)
add_subdirectory_ifdef(CONFIG_NRF70_BUSLIB bus)
