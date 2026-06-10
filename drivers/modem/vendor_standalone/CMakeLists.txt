# SPDX-License-Identifier: Apache-2.0

add_subdirectory(simcom)
add_subdirectory_ifdef(CONFIG_MODEM_HL78XX hl78xx)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_MODEM_HL7800 hl7800.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_QUECTEL_BG9X quectel-bg9x.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_ST87MXX st87mxx.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_UBLOX_SARA ublox-sara-r4.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_WNCM14A2A wncm14a2a.c)
# zephyr-keep-sorted-stop
