# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(rmt.c rmt_common.c rmt_encoder_bytes.c rmt_encoder_copy.c rmt_encoder_simple.c rmt_encoder.c rmt_rx.c rmt_tx.c)
