# SPDX-License-Identifier: Apache-2.0

# TODO: remove, moved to sign.py
board_runner_args(intel_adsp "--default-key=${RIMAGE_SIGN_KEY}")

board_finalize_runner_args(intel_adsp)
