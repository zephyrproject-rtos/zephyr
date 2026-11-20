#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
set -eu

export FANOUT_CONFIG=audio
for scenario in small_large large_small all_small all_large; do
  bash "$(dirname "$0")/../${scenario}.sh" "$@"
done
