# SPDX-License-Identifier: Apache-2.0

# The settings-based NVM implementation is loramac-node specific
set(ZEPHYR_CURRENT_LIBRARY loramac-node)
zephyr_library_sources_ifdef(CONFIG_LORAWAN_NVM_SETTINGS lorawan_nvm_settings.c)
