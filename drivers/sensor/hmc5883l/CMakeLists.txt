# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(hmc5883l.c)
zephyr_library_sources_ifdef(CONFIG_HMC5883L_TRIGGER hmc5883l_trigger.c)
