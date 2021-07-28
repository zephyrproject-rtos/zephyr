# SPDX-License-Identifier: Apache-2.0

# This file contains boards in Zephyr which has been replaced with a new board
# name.
# This allows the system to automatically change the board while at the same
# time prints a warning to the user, that the board name is deprecated.
#
# To add a board rename, add a line in following format:
# set(<old_board_name>_DEPRECATED <new_board_name>)

set(bl5340_dvk_cpuappns_DEPRECATED bl5340_dvk_cpuapp_ns)
set(mps2_an521_nonsecure_DEPRECATED mps2_an521_ns)
set(musca_b1_nonsecure_DEPRECATED musca_b1_ns)
set(musca_s1_nonsecure_DEPRECATED musca_s1_ns)
set(nrf5340dk_nrf5340_cpuappns_DEPRECATED nrf5340dk_nrf5340_cpuapp_ns)
set(nrf9160dk_nrf9160ns_DEPRECATED nrf9160dk_nrf9160_ns)
