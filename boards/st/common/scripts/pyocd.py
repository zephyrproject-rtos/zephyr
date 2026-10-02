# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

from pyocd.core.target import Target


def did_connect(board):
    if board.session.options.get("connect_mode") == "under-reset":
        board.target.reset_and_halt()

    def reset_for_flash(_notification):
        board.target.reset_and_halt()

    board.session.subscribe(
        reset_for_flash,
        [Target.Event.PRE_FLASH_ERASE, Target.Event.POST_FLASH_PROGRAM],
    )
