#!/usr/bin/env bash

# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu
dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

cd "${dotslash}"

(
	cd connecter/
	./_build.sh
)

(
	cd connectable/
	./_build.sh
)

(
	cd dut/
	./_build.sh
)
