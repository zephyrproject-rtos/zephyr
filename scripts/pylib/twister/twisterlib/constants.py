#!/usr/bin/env python3
#
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

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
