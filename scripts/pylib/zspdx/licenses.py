# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

"""Identifiers of the SPDX license list.

Taken from the license-expression package, which tracks the published list,
rather than from a snapshot vendored here.
"""

from functools import cache

from license_expression import get_spdx_licensing


@cache
def get_license_ids() -> frozenset[str]:
    """The SPDX license list identifiers, deprecated spellings included."""
    ids = set()
    for key, symbol in get_spdx_licensing().known_symbols.items():
        # Aliases hold the deprecated spellings (GPL-2.0, LGPL-2.1, ...) still
        # found in source headers. The package's own LicenseRef-* keys and its
        # few spaced aliases are not SPDX identifiers.
        for name in (key, *symbol.aliases):
            if not name.startswith("LicenseRef-") and " " not in name:
                ids.add(name)
    return frozenset(ids)
