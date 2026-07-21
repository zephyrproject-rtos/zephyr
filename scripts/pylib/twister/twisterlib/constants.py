#!/usr/bin/env python3
#
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os

from twisterlib import ZEPHYR_BASE

# Use this for internal comparisons; that's what canonicalization is
# for. Don't use it when invoking other components of the build system
# to avoid confusing and hard to trace inconsistencies in error messages
# and logs, generated Makefiles, etc. compared to when users invoke these
# components directly.
# Note "normalization" is different from canonicalization, see os.path.
canonical_zephyr_base = os.path.realpath(ZEPHYR_BASE)

SUPPORTED_SIMS = [
    "mdb-nsim",
    "nsim",
    "renode",
    "qemu",
    "tsim",
    "armfvp",
    "xt-sim",
    "native",
    "custom",
    "simics",
    "whisper",
]
SUPPORTED_SIMS_IN_PYTEST = ['native', 'qemu']
SUPPORTED_SIMS_WITH_EXEC = ['nsim', 'mdb-nsim', 'renode', 'tsim', 'native', 'simics', 'custom']

# Simulators whose executable is resolved at CMake configure time (via
# find_program) instead of being known during test planning. The binary that
# gets used can depend on the build configuration (e.g. Arm FVP selects a
# different model depending on the Ethos-U NPU), so these cannot be listed in
# SUPPORTED_SIMS_WITH_EXEC and checked up front. After the build, availability
# is read from the named CMakeCache variable: a missing entry or a *-NOTFOUND
# value means the simulator was not found and the test cannot be executed.
SIM_PROGRAM_CMAKE_VARS = {
    'armfvp': 'ARMFVP',
}

PYTEST_HARNESSES = ['pytest', 'shell', 'power', 'display_capture']

SUPPORTED_HARNESSES = [
    'console',
    'ztest',
    'test',
    'gtest',
    'robot',
    'ctest',
    'bsim',
    'script',
] + PYTEST_HARNESSES
