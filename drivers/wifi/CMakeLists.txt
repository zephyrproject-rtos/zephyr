# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_WIFI_WINC1500	winc1500)
add_subdirectory_ifdef(CONFIG_WIFI_SIMPLELINK	simplelink)
add_subdirectory_ifdef(CONFIG_WIFI_ESWIFI	eswifi)
