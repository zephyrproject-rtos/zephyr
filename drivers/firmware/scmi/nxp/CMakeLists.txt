# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ARM_SCMI_NXP_VENDOR_EXTENSIONS shmem.c)
zephyr_library_sources_ifdef(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS cpu.c)
zephyr_library_sources_ifdef(CONFIG_NXP_SCMI_BBM_HELPERS bbm.c)
