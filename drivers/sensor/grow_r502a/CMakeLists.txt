# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(grow_r502a.c)
zephyr_library_sources_ifdef(CONFIG_GROW_R502A_TRIGGER  grow_r502a_trigger.c)
