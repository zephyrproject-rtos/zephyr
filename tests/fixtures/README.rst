.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

Twister fixture registry
########################

Each ``<name>.yaml`` file here describes one fixture a test scenario may request
with ``harness_config: fixture`` (or in ``required_devices``): the hardware or
setup a board must provide for it. Twister loads this directory when it scans
the test configurations and refuses any scenario that names a fixture without
a file here, so a new fixture is added in the same change that introduces its
first user.

The files are validated against ``scripts/schemas/twister/fixtures-schema.yaml``
and rendered in the twister documentation, see ``doc/develop/twister/index.rst``
(section "Fixture registry").
