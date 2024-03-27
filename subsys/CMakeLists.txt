# SPDX-License-Identifier: Apache-2.0

# FIXME: SHADOW_VARS: move this before adding shadow variable warning below.
# This is because, in some build configurations, the external lorawan module
# is pulled in as though the source files are in main repo. This results in
# shadow variable warnings being active on these files. Until the module has
# fixed those shadow variables, keep this here before add_compile_options()
# below.
add_subdirectory_ifdef(CONFIG_LORAWAN lorawan)

# FIXME: SHADOW_VARS: Remove this once we have enabled -Wshadow globally.
add_compile_options($<TARGET_PROPERTY:compiler,warning_shadow_variables>)

# zephyr-keep-sorted-start
add_subdirectory(canbus)
add_subdirectory(debug)
add_subdirectory(fb)
add_subdirectory(fs)
add_subdirectory(ipc)
add_subdirectory(logging)
add_subdirectory(mem_mgmt)
add_subdirectory(mgmt)
add_subdirectory(modbus)
add_subdirectory(pm)
add_subdirectory(portability)
add_subdirectory(random)
add_subdirectory(rtio)
add_subdirectory(sd)
add_subdirectory(stats)
add_subdirectory(storage)
add_subdirectory(task_wdt)
add_subdirectory(testsuite)
add_subdirectory(tracing)
add_subdirectory(usb)

add_subdirectory_ifdef(CONFIG_ARM_SIP_SVC_SUBSYS sip_svc)
add_subdirectory_ifdef(CONFIG_BINDESC bindesc)
add_subdirectory_ifdef(CONFIG_BT bluetooth)
add_subdirectory_ifdef(CONFIG_CONSOLE_SUBSYS console)
add_subdirectory_ifdef(CONFIG_DAP dap)
add_subdirectory_ifdef(CONFIG_DEMAND_PAGING demand_paging)
add_subdirectory_ifdef(CONFIG_DISK_ACCESS disk)
add_subdirectory_ifdef(CONFIG_DSP dsp)
add_subdirectory_ifdef(CONFIG_EMUL emul)
add_subdirectory_ifdef(CONFIG_IMG_MANAGER dfu)
add_subdirectory_ifdef(CONFIG_INPUT input)
add_subdirectory_ifdef(CONFIG_JWT jwt)
add_subdirectory_ifdef(CONFIG_LLEXT llext)
add_subdirectory_ifdef(CONFIG_MODEM_MODULES modem)
add_subdirectory_ifdef(CONFIG_NET_BUF net)
add_subdirectory_ifdef(CONFIG_RETENTION retention)
add_subdirectory_ifdef(CONFIG_SENSING sensing)
add_subdirectory_ifdef(CONFIG_SETTINGS settings)
add_subdirectory_ifdef(CONFIG_SHELL shell)
add_subdirectory_ifdef(CONFIG_TIMING_FUNCTIONS timing)
add_subdirectory_ifdef(CONFIG_ZBUS zbus)
# zephyr-keep-sorted-stop
