# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Helper functions used by gen_kconfig_rest.py and gen_devicetree_rest.py.
"""

import errno


def write_if_updated(path, s):
    """
    Writes 's' as the contents of <out_dir>/<filename>, but only if it
    differs from the current contents of the file. This avoids unnecessary
    timestamp updates, which trigger documentation rebuilds.

    Returns True if the file was updated, False otherwise.
    """

    try:
        with open(path, encoding="utf-8") as f:
            if s == f.read():
                return False
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

    with open(path, "w", encoding="utf-8") as f:
        f.write(s)
    return True
