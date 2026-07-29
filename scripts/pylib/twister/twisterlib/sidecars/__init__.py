# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Sidecars: host-side resources provisioned around a test run.

A sidecar is selected with the ``sidecar:`` testsuite field and is orthogonal
to the ``harness:`` field: the harness processes the guest's output while the
sidecar provisions a host-side resource (a ``virtiofsd`` daemon, an ivshmem
shared memory region, ...) around the run. The runner calls
:meth:`~twisterlib.sidecars.base.Sidecar.setup` before the handler executes the
test and :meth:`~twisterlib.sidecars.base.Sidecar.teardown` afterwards.

This keeps output processing and host provisioning independent, so any harness
(``console`` for a sample, ``ztest`` for a test) can be paired with any sidecar,
and twister can also attach a sidecar itself (for example to capture coverage
over ivshmem) without the test having to opt in.

Each sidecar lives in its own module in this package and registers itself simply
by subclassing :class:`~twisterlib.sidecars.base.Sidecar` with a ``NAME``.
Importing this package imports every module in it, so adding a sidecar is just
adding a file here: no central registry, no runner change, and no other common
code has to be touched.
"""

from __future__ import annotations

import importlib
import pkgutil

from twisterlib.sidecars.base import Sidecar, SidecarImporter

# Import every sidecar module (everything in this package but ``base``) so that
# each Sidecar subclass registers itself. This is what lets a new sidecar be
# added by dropping a single module in here.
for _module_info in pkgutil.iter_modules(__path__):
    if _module_info.name != 'base':
        importlib.import_module(f'{__name__}.{_module_info.name}')


def sidecar_config_schema() -> dict:
    """Assemble the ``sidecar_config`` schema from the registered sidecars.

    Each sidecar contributes its own block, keyed by ``NAME`` (see
    :meth:`Sidecar.config_schema`), so the twister schema file does not have to
    change when a sidecar is added -- the runner injects this at load time.
    """
    return {
        'type': 'object',
        'properties': {
            name: cls.config_schema() for name, cls in sorted(Sidecar._registry.items())
        },
        'additionalProperties': False,
    }


__all__ = ['Sidecar', 'SidecarImporter', 'sidecar_config_schema']
