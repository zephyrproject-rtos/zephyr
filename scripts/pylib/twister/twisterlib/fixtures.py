# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Registry of the fixtures test scenarios may request.

A fixture names a hardware setup a board must provide for a scenario to run
(see ``harness_config: fixture:``). Every fixture name used in the tree is
described by ``tests/fixtures/<name>.yaml``; test roots outside the tree
extend the registry with ``--fixture-registry``.
"""

from __future__ import annotations

import logging
import os
import re
from collections.abc import Iterable

import scl
from twisterlib import ZEPHYR_BASE
from twisterlib.error import ConfigurationError

logger = logging.getLogger('twister')

FIXTURES_SCHEMA_PATH = os.path.join(
    ZEPHYR_BASE, "scripts", "schemas", "twister", "fixtures-schema.yaml"
)
FIXTURES_REGISTRY_PATH = os.path.join(ZEPHYR_BASE, "tests", "fixtures")
FIXTURE_NAME_RE = re.compile(r'^[a-z0-9_]+$')


class FixtureRegistry:
    """Known fixture names, loaded from one or more registry directories."""

    def __init__(self, registry_dirs: Iterable[str] | None = None):
        self.fixtures: dict[str, dict] = {}
        self.dirs: list[str] = []
        self.schema = scl.yaml_load(FIXTURES_SCHEMA_PATH)
        for path in registry_dirs if registry_dirs is not None else [FIXTURES_REGISTRY_PATH]:
            self.load(path)

    def load(self, path: str) -> None:
        """Add the fixtures described by the ``<name>.yaml`` files of one directory."""
        if not os.path.isdir(path):
            raise ConfigurationError(path, "fixture registry directory not found")
        for filename in sorted(os.listdir(path)):
            name, ext = os.path.splitext(filename)
            if ext != '.yaml':
                continue
            cfile = os.path.join(path, filename)
            if not FIXTURE_NAME_RE.match(name):
                raise ConfigurationError(
                    cfile, "fixture names consist of lowercase letters, digits and underscores"
                )
            if name in self.fixtures:
                raise ConfigurationError(cfile, f"fixture '{name}' is already registered")
            self.fixtures[name] = scl.yaml_load_verify(cfile, self.schema)
        self.dirs.append(path)

    def __contains__(self, name: str) -> bool:
        return self.base_name(name) in self.fixtures

    @staticmethod
    def base_name(fixture: str) -> str:
        """Strip the optional ``:parameter`` suffix a hardware map may append."""
        return fixture.split(':', 1)[0]

    def unknown(self, fixtures: Iterable[str]) -> list[str]:
        """Return the given fixture names that are not registered, in order."""
        return [f for f in fixtures if f not in self]

    def check_suite(self, suite, cfile: str) -> None:
        """Raise ConfigurationError if a test suite requests an unregistered fixture.

        ``cfile`` is the test configuration file named in the error.
        """
        requested = list(suite.harness_config.fixture or [])
        for req_dev in suite.harness_config.required_devices:
            requested.extend(req_dev.fixture)
        unknown = self.unknown(requested)
        if unknown:
            raise ConfigurationError(
                cfile,
                f"scenario '{suite.name}' uses fixtures not listed in the fixture registry: "
                f"{', '.join(sorted(set(unknown)))}. Describe them in "
                f"{FIXTURES_REGISTRY_PATH}/<name>.yaml or pass --fixture-registry.",
            )
