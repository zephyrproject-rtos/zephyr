.. _bluetooth-contrib:

Contributing to the Bluetooth subsystem
#######################################

Overview
********

This page describes how one would contribute to the Zephyr Bluetooth stack.
It is a work in progress.

The first step is familiarizing yourself with the :ref:`Stack architecture <bluetooth-arch>`.

.. _bluetooth-coding-style:

Coding guidelines
*****************

The following describes a few of the coding conventions we use in the Bluetooth
subsystem. See also the :ref:`Zephyr Coding Guidelines <coding_guidelines>` and
the :ref:`Zephyr Coding Style <coding_style>`.

The guidelines in this document should not take precedence on rules from the
Zephyr Coding Guidelines and Coding Style mentioned above.

Unless there is a convention listed here that disagrees, the general "rule" is
to try to follow the style of the code surrounding the change.

The Bluetooth contribution guidelines are not set in stone, and this is a living
document. It will be updated if enough people agree on amending or changing the
rules.

Tools
=====

Install and use editorconfig_. This should ensure your editor uses the proper
settings, e.g. indentation, maximum line length, etc..

Before submitting a pull request, it is a good idea to:

- Run ``clang-format -i`` on your patch. See also the VScode_ or SublimeText_ plugins.
- Run ``./scripts/ci/check_compliance.py -c upstream/main..**`` and resolve any reported issues

Git `Commit hooks`_ can be used for running the compliance script automatically.

Whitespace
==========

- Tabs in the beginning of a line is the only place where tabs should ever be found.
- Add an empty line after every variable definition block.

.. _`Commit hooks`: https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks
.. _SublimeText: https://packagecontrol.io/packages/Clang%20Format
.. _VScode: https://marketplace.visualstudio.com/items?itemName=xaver.clang-format
.. _editorconfig: https://editorconfig.org/#download
