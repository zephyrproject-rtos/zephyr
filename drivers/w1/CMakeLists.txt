# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_USERSPACE w1_handlers.c)
zephyr_library_sources_ifdef(CONFIG_W1_SHELL        w1_shell.c)
zephyr_library_sources(w1_common.c)

# drivers implementing link functions (read, write, reset)
zephyr_library_sources_ifdef(CONFIG_W1_DS2484         w1_ds2484.c)
zephyr_library_sources_ifdef(CONFIG_W1_DS2485           w1_ds2485.c)
zephyr_library_sources_ifdef(CONFIG_W1_DS2477_85_COMMON w1_ds2477_85_common.c)
zephyr_library_sources_ifdef(CONFIG_W1_TEST           w1_test.c)
zephyr_library_sources_ifdef(CONFIG_W1_ZEPHYR_SERIAL  w1_zephyr_serial.c)

# network functions:
if(CONFIG_W1_NET)
# network functions should be callable from user as well as supervisor mode:
remove_definitions(-D__ZEPHYR_SUPERVISOR__)
zephyr_library_sources(w1_net.c)
endif() #CONFIG_W1_NET
