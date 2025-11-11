#!/usr/bin/env bash
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

set -eu

# Provides common functions for bsim tests.
# Mainly `Execute`, and `wait_for_background_jobs`.
source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# Helper variable. Expands to "tests_bsim_bluetooth_host_misc_sample_test".
test_name="$(guess_test_long_name)"

# Use a unique simulation id per test script. It is a necessity as the CI runner
# will run all the test scripts in parallel. If multiple simulations share the
# same ID, they will step on each other's toes in unpredictable ways.
simulation_id=${test_name}

# This controls the verbosity of the simulator. It goes up to 9 (not 11, sorry)
# and is useful for figuring out what is happening on the PHYsical layer (e.g.
# device 1 starts listening at T=100us) among other things.
# `2` is the default value if not specified.
verbosity_level=2

# This sets the watchdog timeout for the `Execute` function.
#
# Although all simulations are started with a bounded end (the `sim_length`
# option), something very wrong can still happen and this additional time-out
# will ensure all executables started by the current script are killed.
#
# It measures wall-clock time, not simulated time. E.g. a test that simulates 5
# hours might actually complete (have a runtime of) 10 seconds.
#
# The default is set in `sh_common.source`.
# Guidelines to set this value:
# - Do not set it to a value lower or equal to the default.
# - If the test takes over 5 seconds of runtime, set `EXECUTE_TIMEOUT` to at
#   least 5 times the run-time on your machine.
EXECUTE_TIMEOUT=120

# Set simulation length, in microseconds. The PHY will run for this amount of
# simulated time, unless both devices exit.
#
# If you are not early-exiting the devices (e.g. using `TEST_PASS_AND_EXIT()`),
# please run the test once and set the simulation time in the same ballpark. No
# need to simulate hours of runtime if the test finishes in 10 seconds.
#
SIM_LEN_US=$((2 * 1000 * 1000))

# This is the final path of the test executable.
#
# Some tests may have different executable images for each device in the test.
#
# In our case, both test cases are compiled in the same image, and the right one
# will be run depending on what arguments we give the executable.
test_exe="${BSIM_OUT_PATH}/bin/bs_${BOARD_TS}_${test_name}_prj_conf"

# BabbleSim will by default search for its shared libraries assuming it is
# running in the bin/ directory. Test results will also be placed in
# `${BSIM_OUT_PATH}/results` if not specified.
cd ${BSIM_OUT_PATH}/bin

# Instantiate all devices in the simulation.
# The `testid` parameter is used to run the right role or procedure (here "dut" vs "tester").
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=0 -rs=420 -testid=dut \
		-argstest log_level 3
Execute "${test_exe}" -v=${verbosity_level} -s=${simulation_id} -d=1 -rs=69  -testid=peer \
		-argstest log_level 3

# Start the PHY. Double-check the `-D` parameter: it has to match the number of
# devices started in the lines above.
#
# Also set a maximum simulation length. If the devices have not set a special
# variable indicating they have passed before the simulation runs out of time,
# the test will be reported as "in progress (not passed)".
Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} -D=2 -sim_length=${SIM_LEN_US} $@

# Wait until all executables started above have returned.
# The exit code returned will be != 0 if any of them have failed.
wait_for_background_jobs
