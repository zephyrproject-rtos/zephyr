# Copyright (c) 2020 Erik Larson
# Copyright (c) 2022 Jason Kridner, BeagleBoard.org Foundation
#
# SPDX-License-Identifier: Apache-2.0

# Download cc2538-bsl.py from https://git.beagleboard.org/beagleconnect/zephyr/cc2538-bsl/-/tags/2.1-bcf

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher $ENV{ZEPHYR_BASE}/boards/arm/beagle_bcf/cc2538-bsl.py -w)
