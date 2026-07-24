# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tmp11x.c)
zephyr_library_sources_ifdef(CONFIG_TMP11X_TRIGGER tmp11x_trigger.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_TMP11X tmp11x_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_TMP11X .)
