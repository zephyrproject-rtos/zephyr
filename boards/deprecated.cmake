# SPDX-License-Identifier: Apache-2.0

# This file contains boards in Zephyr which has been replaced with a new board
# name.
# This allows the system to automatically change the board while at the same
# time prints a warning to the user, that the board name is deprecated.
#
# To add a board rename, add a line in following format:
# set(<old_board_name>_DEPRECATED <new_board_name>)

set(nrf51_pca10028_DEPRECATED nrf51dk_nrf51422)
set(nrf52840_pca10056_DEPRECATED nrf52840dk_nrf52840)
