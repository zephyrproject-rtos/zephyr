"""
Kconfig role and directive
##########################

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

A Sphinx extension that provides a role and directive for Kconfig content. The
``:kconfig:`` role can be used inline to generate references to the page
containing documentation for any Kconfig option. The ``.. kconfig::`` directive
is automatically added to the Kconfig generated pages.

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""


def setup(app):
    app.add_crossref_type("kconfig", "kconfig")

    return {
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
