#!/usr/bin/env bash
# Copyright 2026 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test verifies that a BLOB Client broadcast neither waits for nor counts
# responses from a target marked as skipped. A skipped target is not
# addressed by the step, but it can still receive a group-addressed message
# and answer it. Its status message must be dropped by the client's model
# handler: it must not acknowledge the target, change tx.pending or reach
# the application callbacks.
#
# Node layout:
#   Device 0: BLOB Client (BLOB_CLI_ADDR).
#   Device 1: BLOB Transfer Server (BLOB_CLI_ADDR + 1), subscribed to
#             BLOB_GROUP_ADDR. Marked as skipped on the client.
#   No device has the active target's address (BLOB_CLI_ADDR + 2); its
#   response is injected with blob_cli_broadcast_rsp().
#
# Test procedure:
# 1. The client adds the skipped target (BLOB_CLI_ADDR + 1, skip = 1) and
#    the active target (BLOB_CLI_ADDR + 2).
# 2. The client starts the transfer progress procedure, which sends BLOB
#    Transfer Get to BLOB_GROUP_ADDR, and asserts tx.pending == 1: only the
#    active target is expected to respond.
# 3. The server on device 1 receives the group-addressed message and
#    answers with BLOB Transfer Status.
# 4. After 2 seconds, before the first retry, the client asserts that
#    tx.pending is still 1, the skipped target is not acknowledged and the
#    xfer_progress callback has not been called.
# 5. A response from the active target is injected. The client asserts that
#    the broadcast completes immediately (xfer_progress_complete is called
#    without waiting for the retry interval), the active target is
#    acknowledged, the skipped target is neither acknowledged, timed out nor
#    dropped, and xfer_progress was never called.
RunTest blob_broadcast_skip \
	blob_cli_broadcast_skip blob_srv_caps_standard
