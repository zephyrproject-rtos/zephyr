# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
A Sphinx extension for documenting devicetree content. It adds a role and a
directive with the same name, 'dtcompatible'.

:dtcompatible:`vnd,foo`

  This role can be used inline to make a reference to the generated
  documentation for a devicetree
  compatible given as argument.

  For example, :dtcompatible:`vnd,foo` would create a reference to the
  generated documentation page for the devicetree binding handling
  compatible "vnd,foo".

  There may be more than one page for a single compatible. For example,
  that happens if a binding behaves differently depending on the bus the
  node is on. If that occurs, the reference points at a "disambiguation"
  page which links out to all the possibilities, similarly to how Wikipedia
  disambiguation pages work.

  The Zephyr documentation uses the standard :option: role to refer
  to Kconfig options. The :dtcompatible: option is like that, except
  using its own role to avoid using one that already has a standard meaning.
  (The :option: role is meant for documenting options to command-line programs,
  not Kconfig symbols.)

.. dtcompatible:: vnd,foo

  This directive marks the page where the :dtcompatible: role link goes.
  Do not use it directly. The generated bindings documentation puts these
  in the right places.
"""

def setup(app):
    app.add_crossref_type('dtcompatible', 'dtcompatible')

    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
