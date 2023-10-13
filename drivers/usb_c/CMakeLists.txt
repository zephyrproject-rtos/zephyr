# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_USBC_TCPC_DRIVER tcpc)
add_subdirectory_ifdef(CONFIG_USBC_VBUS_DRIVER vbus)
add_subdirectory_ifdef(CONFIG_USBC_PPC_DRIVER ppc)
