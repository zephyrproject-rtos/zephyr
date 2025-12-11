# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmp11x.c)
zephyr_library_sources_ifdef(CONFIG_TMP11X_TRIGGER tmp11x_trigger.c)
