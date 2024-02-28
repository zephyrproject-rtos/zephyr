#!/usr/bin/env python3
# Copyright(c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

import sys
from ruamel.yaml import YAML
yaml=YAML()
conf=yaml.load(sys.stdin)
yaml.dump(conf, sys.stdout)
