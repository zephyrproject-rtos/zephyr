# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Base class and registry for twister sidecars.

See :mod:`twisterlib.sidecars` for what a sidecar is and how the package
discovers the implementations that live next to this module.
"""

from __future__ import annotations

import dataclasses
import logging
import types
import typing

from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


def _json_schema_for_type(tp) -> dict:
    """Map a Python type annotation to a JSON-schema fragment.

    Only the shapes a sidecar ``Config`` realistically uses are handled
    (``str``/``int``/``float``/``bool``, ``list[...]`` and ``X | None``);
    anything else falls back to an unconstrained ``{}`` rather than guessing.
    """
    origin = typing.get_origin(tp)
    if origin in (typing.Union, types.UnionType):
        # Treat ``X | None`` as ``X``; a genuine multi-type union we cannot
        # express is left unconstrained.
        args = [a for a in typing.get_args(tp) if a is not types.NoneType]
        return _json_schema_for_type(args[0]) if len(args) == 1 else {}
    if origin in (list, tuple):
        item_args = typing.get_args(tp)
        return {'type': 'array', 'items': _json_schema_for_type(item_args[0]) if item_args else {}}
    if tp is bool:
        return {'type': 'boolean'}
    if tp is int:
        return {'type': 'integer'}
    if tp is float:
        return {'type': 'number'}
    if tp is str:
        return {'type': 'string'}
    return {}


class Sidecar:
    """Base class for host-side resources provisioned around a test run.

    A subclass sets :attr:`NAME` to the value that selects it (the testsuite's
    ``sidecar:`` field, which is also the key of its block under
    ``sidecar_config:``) and is registered automatically by defining that name,
    so adding a sidecar needs no change to any common code. When it also defines
    a :attr:`Config` dataclass, that block is coerced into it and exposed as
    :attr:`config`; otherwise ``config`` is the raw mapping. Namespacing the
    configuration by sidecar name keeps each sidecar's keys from colliding, so
    ``sidecar_config`` can describe more than one sidecar without ambiguity even
    though only the active one is consumed.
    """

    #: Value of the ``sidecar:`` field that selects this sidecar; also its key
    #: under ``sidecar_config:``. Left ``None`` on the abstract base.
    NAME: str | None = None

    #: Optional dataclass describing this sidecar's ``sidecar_config`` block.
    Config: type | None = None

    #: Registry of ``NAME`` to sidecar class, populated as subclasses are
    #: defined (see :meth:`__init_subclass__`), so no central list has to be
    #: edited when a sidecar is added.
    _registry: dict[str, type[Sidecar]] = {}

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        if cls.NAME is not None:
            Sidecar._registry[cls.NAME] = cls

    def __init__(self):
        self.instance: TestInstance | None = None
        self.config = None

    def configure(self, instance: TestInstance):
        self.instance = instance
        raw = (getattr(instance.testsuite, 'sidecar_config', None) or {}).get(self.NAME, {})
        self.config = self.Config(**raw) if self.Config is not None else raw

    @classmethod
    def config_schema(cls) -> dict:
        """JSON-schema fragment validating this sidecar's ``sidecar_config`` block.

        Derived from the :attr:`Config` dataclass so the configuration is
        described in exactly one place; the twister schema assembles these
        fragments at load time (see :func:`twisterlib.sidecars.sidecar_config_schema`)
        rather than hard-coding each sidecar's keys. A sidecar with unusual
        needs may override this.
        """
        if cls.Config is None:
            return {'type': 'object', 'additionalProperties': False}
        hints = typing.get_type_hints(cls.Config)
        properties = {
            f.name: _json_schema_for_type(hints[f.name]) for f in dataclasses.fields(cls.Config)
        }
        return {'type': 'object', 'properties': properties, 'additionalProperties': False}

    def cmake_env(self, build_dir: str) -> dict[str, str]:
        """Environment overrides for the CMake configure step.

        Returns environment variables the build needs for this sidecar (for
        example QEMU flags that reference a per-build socket). Called on a
        configured sidecar before CMake runs; the default adds nothing. Keeping
        this on the sidecar lets the runner stay agnostic of any specific one.
        """
        return {}

    def cmake_args(self, build_dir: str) -> list[str]:
        """Extra CMake arguments for the configure step.

        Returns additional arguments (typically ``-DCONFIG_...`` Kconfig values)
        the build needs for this sidecar -- for example pinning the guest to the
        host interface the sidecar created. Called on a configured sidecar before
        CMake runs; the default adds nothing. Like :meth:`cmake_env`, this keeps
        sidecar specifics out of the runner.
        """
        return []

    def setup(self) -> bool:
        """Provision the resource before the test is executed.

        Called before the test runs, whichever path executes it: a harness that
        runs the test itself, or the handler. Returns True when execution should
        proceed, False to skip it (for example when a required host tool is
        missing). It need not undo its own partial work on failure, because
        :meth:`teardown` runs either way.
        """
        return True

    def teardown(self) -> None:
        """Release the resource after the test has been executed.

        Called in a ``finally`` block once the sidecar has been configured, so
        it must be safe to call even if the test failed or timed out, and even
        if :meth:`setup` failed or never provisioned anything.
        """


class SidecarImporter:
    @staticmethod
    def get_sidecar(sidecar_name):
        if not sidecar_name:
            return None
        sidecar_class = Sidecar._registry.get(sidecar_name)
        if sidecar_class is None:
            # A name that does not resolve is almost always a typo in the
            # testsuite's `sidecar:`; fail loudly rather than silently running
            # the test without the host resource it asked for.
            supported = ', '.join(sorted(Sidecar._registry))
            raise ValueError(f"unknown sidecar '{sidecar_name}' (supported: {supported})")
        return sidecar_class()
