# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_USERSPACE w1_handlers.c)
zephyr_library_sources(w1_common.c)

# drivers implementing link functions (read, write, reset)
# none implemented yet

# network functions:
if(CONFIG_W1_NET)
# network functions should be callable from user as well as supervisor mode:
remove_definitions(-D__ZEPHYR_SUPERVISOR__)
zephyr_library_sources(w1_net.c)
endif() #CONFIG_W1_NET
