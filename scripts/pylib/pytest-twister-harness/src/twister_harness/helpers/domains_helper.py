# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import os
import sys
import logging

from pathlib import Path

ZEPHYR_BASE = os.environ['ZEPHYR_BASE']
sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts', 'pylib', 'build_helpers'))

from domains import Domains

logger = logging.getLogger(__name__)
logging.getLogger('pykwalify').setLevel(logging.ERROR)


def get_default_domain_name(domains_file: Path | str) -> int:
    """
    Get the default domain name from the domains.yaml file
    """
    domains = Domains.from_file(domains_file)
    logger.debug("Loaded sysbuild domain data from %s" % domains_file)
    return domains.get_default_domain().name
