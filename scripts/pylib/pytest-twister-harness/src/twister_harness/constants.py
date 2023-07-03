# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

QEMU_FIFO_FILE_NAME: str = 'qemu-fifo'
END_OF_DATA = object()  #: used for indicating that there will be no more data in queue
