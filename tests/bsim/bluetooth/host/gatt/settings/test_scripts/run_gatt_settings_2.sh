#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set -eu

bash_source_dir="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

source "${bash_source_dir}/_env.sh"
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="${test_name}_2"
verbosity_level=2
EXECUTE_TIMEOUT=120

cd ${BSIM_OUT_PATH}/bin

# Remove the files used by the custom SETTINGS backend
TO_DELETE="${simulation_id}_server_2.log ${simulation_id}_client_2.log"
echo "remove settings files ${TO_DELETE}"
rm ${TO_DELETE} || true

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s="${simulation_id}" -D=8 -sim_length=30e6 $@

# Only one `server` device is really running at a time. This is a necessary hack
# because bsim doesn't support plugging devices in and out of a running
# simulation, but this test needs a way to power-cycle the `server` device a few
# times.
#
# Each device will wait until the previous instance (called 'test round') has
# finished executing before starting up.
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=0 -testid=server -RealEncryption=1 -argstest 0 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=1 -testid=server -RealEncryption=1 -argstest 1 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=2 -testid=server -RealEncryption=1 -argstest 2 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=3 -testid=server -RealEncryption=1 -argstest 3 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=4 -testid=server -RealEncryption=1 -argstest 4 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=5 -testid=server -RealEncryption=1 -argstest 5 6 "server_2"
Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=6 -testid=server -RealEncryption=1 -argstest 6 6 "server_2"

Execute "$test_2_exe" -v=${verbosity_level} \
    -s="${simulation_id}" -d=7 -testid=client -RealEncryption=1 -argstest 0 0 "client_2"

wait_for_background_jobs
