#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

dotslash="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"
bin_dir="${BSIM_OUT_PATH}/bin"
BOARD="${BOARD:-nrf52_bsim}"

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

cd "${dotslash}"

compile_name="bs_${BOARD}_"
compile_name+="$(realpath --relative-to "$(west topdir)"/zephyr prj.conf | tr /. _)"

compile_path="${bin_dir}/${compile_name}"
simulation_id="${compile_name}_$(basename "${BASH_SOURCE[0]}" | tr /. _)"

args_all=(-s="${simulation_id}" -D=2)
args_dev=(-v=2 -RealEncryption=1 -testid=the_test)
sim_seconds=200

echo "Simulation time: $sim_seconds seconds"

EXECUTE_TIMEOUT=60

# bs_2G4_phy_v1 requires pwd to at its location
cd "${BSIM_OUT_PATH}/bin"

Execute "${compile_path}" "${args_all[@]}" "${args_dev[@]}" -d=1
Execute ./bs_2G4_phy_v1 "${args_all[@]}" -v=6 -sim_length=$((sim_seconds * 10 ** 6))
Execute "${compile_path}" "${args_all[@]}" "${args_dev[@]}" -d=0

wait_for_background_jobs
