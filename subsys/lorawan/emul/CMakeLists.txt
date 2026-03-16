# SPDX-License-Identifier: Apache-2.0

# The emulator currently depends on loramac-node for some types and functions.
# Once these dependencies are removed, it can become a fully standalone backend.
set(ZEPHYR_CURRENT_LIBRARY loramac-node)
zephyr_library_sources_ifdef(CONFIG_LORAWAN_EMUL lorawan_emul.c)
