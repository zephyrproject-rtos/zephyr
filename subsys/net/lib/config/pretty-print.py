#!/usr/bin/env python3
# Copyright(c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

import yaml, sys, pprint
pprint.pprint(yaml.safe_load(sys.stdin))
