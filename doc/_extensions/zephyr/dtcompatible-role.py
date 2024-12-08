# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
A Sphinx extension for documenting devicetree content. It adds a role and a
directive with the same name, 'dtcompatible'.

The directive marks the page where the :dtcompatible: role link goes.
Do not use it directly. The generated bindings documentation puts these
in the right places.
"""

def setup(app):
    app.add_crossref_type('dtcompatible', 'dtcompatible')

    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
