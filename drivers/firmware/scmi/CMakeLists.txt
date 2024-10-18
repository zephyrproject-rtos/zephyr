# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# SCMI core files
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI core.c)
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_MAILBOX_TRANSPORT mailbox.c)
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_SHMEM shmem.c)

# SCMI protocol helper files
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_CLK_HELPERS clk.c)
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_PINCTRL_HELPERS pinctrl.c)
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_POWER_DOMAIN_HELPERS power.c)
