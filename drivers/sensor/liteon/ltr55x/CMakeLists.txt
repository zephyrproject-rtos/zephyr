# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ltr55x.c)
zephyr_library_sources_ifdef(CONFIG_LTR55X_TRIGGER ltr55x_trigger.c)
