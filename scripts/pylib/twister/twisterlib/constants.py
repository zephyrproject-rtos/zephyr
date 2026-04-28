#!/usr/bin/env python3
#
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

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
]
SUPPORTED_SIMS_IN_PYTEST = ['native', 'qemu']
SUPPORTED_SIMS_WITH_EXEC = ['nsim', 'mdb-nsim', 'renode', 'tsim', 'native', 'simics', 'custom']
