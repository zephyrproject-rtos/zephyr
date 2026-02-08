# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ARM_SCMI scmi)
add_subdirectory_ifdef(CONFIG_TISCI tisci)
add_subdirectory_ifdef(CONFIG_QEMU_FWCFG fwcfg)
# zephyr-keep-sorted-stop
