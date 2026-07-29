# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0


def did_connect(board):
    if board.session.options.get("connect_mode") == "under-reset":
        board.target.reset_and_halt()
