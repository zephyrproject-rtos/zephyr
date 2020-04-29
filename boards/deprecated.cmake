# SPDX-License-Identifier: Apache-2.0

# This file contains boards in Zephyr which has been replaced with a new board
# name.
# This allows the system to automatically change the board while at the same
# time prints a warning to the user, that the board name is deprecated.
#
# To add a board rename, add a line in following format:
# set(<old_board_name>_DEPRECATED <new_board_name>)

set(nrf51_pca10028_DEPRECATED nrf51dk_nrf51422)
set(nrf51_pca10031_DEPRECATED nrf51dongle_nrf51422)
set(nrf52_pca10040_DEPRECATED nrf52dk_nrf52832)
set(nrf52_pca20020_DEPRECATED thingy52_nrf52832)
set(nrf52810_pca10040_DEPRECATED nrf52dk_nrf52810)
set(nrf52833_pca10100_DEPRECATED nrf52833dk_nrf52833)
set(nrf52840_pca10056_DEPRECATED nrf52840dk_nrf52840)
set(nrf52840_pca10059_DEPRECATED nrf52840dongle_nrf52840)
set(nrf52811_pca10056_DEPRECATED nrf52840dk_nrf52811)
set(nrf5340_dk_nrf5340_cpuapp_DEPRECATED nrf5340pdk_nrf5340_cpuapp)
set(nrf5340_dk_nrf5340_cpuappns_DEPRECATED nrf5340pdk_nrf5340_cpuappns)
set(nrf5340_dk_nrf5340_cpunet_DEPRECATED nrf5340pdk_nrf5340_cpunet)
set(nrf9160_pca10090_DEPRECATED nrf9160dk_nrf9160)
set(nrf9160_pca10090ns_DEPRECATED nrf9160dk_nrf9160ns)
set(nrf52840_pca10090_DEPRECATED nrf9160dk_nrf52840)
set(efr32_slwstk6061a_DEPRECATED efr32_radio_brd4250b)
