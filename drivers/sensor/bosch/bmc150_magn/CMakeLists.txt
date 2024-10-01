# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmc150_magn.c)
zephyr_library_sources_ifdef(CONFIG_BMC150_MAGN_TRIGGER bmc150_magn_trigger.c)
