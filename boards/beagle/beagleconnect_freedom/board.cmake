# Copyright (c) 2020 Erik Larson
# Copyright (c) 2022 Jason Kridner, BeagleBoard.org Foundation
#
# SPDX-License-Identifier: Apache-2.0

# Download cc1352-flasher (https://pypi.org/project/cc1352-flasher/) using the following command:
# pip3 install cc1352-flasher

find_program(CC1352_FLASHER NAMES cc1352_flasher)
board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher ${CC1352_FLASHER} --bcf)
