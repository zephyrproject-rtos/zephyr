# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(opt300x.c)
zephyr_library_sources_ifdef(CONFIG_OPT300X_TRIGGER opt300x_trigger.c)
