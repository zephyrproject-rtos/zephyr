# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Vanes: board-owned test metadata realizing a named test setup.

A test setup is defined once, under tests/vanes/<setup>.yml. A board or a
SoC realizes it with the devicetree overlays and Kconfig fragments in the
vanes/<setup>/ directory under its own, named like the boards/ and socs/
configuration files of an application, and with an optional vane.yml
saying what only the board knows, such as the fixture a run needs on it.

This module resolves both sides for the build system, twister and the
list_vanes.py tool. See doc/develop/test/vanes.rst.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

import jsonschema
import yaml

ZEPHYR_BASE = Path(__file__).resolve().parents[1]
SETUP_ROOTS = [ZEPHYR_BASE / 'tests' / 'vanes']
REALIZATION_FILE = 'vane.yml'
NO_FIXTURE = 'none'  # as the value of a fixture mapping: the scenario's is waived
SCHEMAS = ZEPHYR_BASE / 'scripts' / 'schemas'


def _load(path: Path, schema: str) -> dict:
    with open(path, encoding='utf-8') as f:
        data = yaml.safe_load(f) or {}
    with open(SCHEMAS / schema, encoding='utf-8') as f:
        jsonschema.validate(data, yaml.safe_load(f))
    return data


@dataclass
class Setup:
    """A test setup, as defined in tests/vanes/<name>.yml."""

    name: str
    description: str
    path: Path
    # What the devicetree shows when the setup is provided, at most one of:
    # an enabled node with the compatible, the chosen node, or the node at
    # the path, with the property if one is named. None of them: only a
    # vane provides the setup.
    compatible: str | None = None
    chosen: str | None = None
    node: str | None = None
    node_property: str | None = None

    @staticmethod
    def load(path: Path) -> Setup:
        data = _load(path, 'vane-setup-schema.yaml')
        name = path.stem
        if data.get('name', name) != name:
            raise ValueError(f'{path}: name {data["name"]!r} does not match the file name')
        if sum(key in data for key in ('compatible', 'chosen', 'node')) > 1:
            raise ValueError(f'{path}: name one of compatible, chosen and node')
        if 'property' in data and 'node' not in data:
            raise ValueError(f'{path}: property needs the node it belongs to')
        return Setup(
            name=name,
            description=data.get('description', ''),
            path=path,
            compatible=data.get('compatible'),
            chosen=data.get('chosen'),
            node=data.get('node'),
            node_property=data.get('property'),
        )

    @property
    def evidence(self) -> str | None:
        """What the devicetree shows when the setup is provided, as text,
        or None for a setup only a vane provides."""
        if self.compatible:
            return f'compatible {self.compatible}'
        if self.chosen:
            return f'chosen {self.chosen}'
        if self.node and self.node_property:
            return f'node {self.node} with {self.node_property}'
        if self.node:
            return f'node {self.node}'
        return None


def find_setups(roots: list[Path] | None = None) -> dict[str, Setup]:
    """All test setups defined under the given roots, by name."""
    setups: dict[str, Setup] = {}
    for root in roots or SETUP_ROOTS:
        for path in sorted(Path(root).glob('*.yml')):
            if path.name == REALIZATION_FILE:
                continue
            setup = Setup.load(path)
            setups.setdefault(setup.name, setup)
    return setups


@dataclass
class Target:
    """A board target: board name, revision and qualifiers."""

    board: str
    qualifiers: str
    revision: str | None = None
    single_soc: bool = True

    @staticmethod
    def parse(name: str, single_soc: bool = True) -> Target:
        """Parse 'board[@revision]/qualifiers'."""
        m = re.fullmatch(r'([^@/]+)(?:@([^/]+))?(?:/(.*))?', name)
        if m is None:
            raise ValueError(f'not a board target: {name!r}')
        return Target(m.group(1), m.group(3) or '', m.group(2), single_soc)

    def board_stems(self) -> list[str]:
        """File name stems a board directory may use for this target.

        The same names as the boards/ directory of an application: the board
        name followed by the qualifiers, or by the qualifiers without the
        SoC when the board has one SoC, each with and without the revision.
        """
        segments = self.qualifiers.split('/') if self.qualifiers else []
        names = ['_'.join([self.board, *segments])]
        if self.single_soc:
            names.append('_'.join([self.board, *segments[1:]]))
        stems = []
        for name in names:
            if self.revision:
                stems.append(name + '_' + self.revision.replace('.', '_'))
            stems.append(name)
        return stems

    def soc_stem(self) -> str:
        """The file name stem a SoC directory uses for this target's qualifiers."""
        return self.qualifiers.replace('/', '_')

    @property
    def soc(self) -> str:
        return self.qualifiers.split('/')[0] if self.qualifiers else ''


@dataclass
class Vane:
    """A board's or SoC's realization of a test setup for one target."""

    setup: str
    files: list[Path] = field(default_factory=list)
    # What the realization says about the fixture a run needs: a name the
    # scenario does not name, or a mapping from fixtures the scenario names to
    # their replacement on this hardware, NO_FIXTURE for none.
    fixture: str | dict[str, str] | None = None

    @property
    def overlays(self) -> list[Path]:
        return [f for f in self.files if f.suffix == '.overlay']

    @property
    def confs(self) -> list[Path]:
        return [f for f in self.files if f.suffix == '.conf']


def _realization(vane_dir: Path) -> dict:
    path = vane_dir / REALIZATION_FILE
    if not path.is_file():
        return {}
    return _load(path, 'vane-schema.yaml')


def find_vane(
    setup: str, target: Target, board_dirs: list[Path], soc_dirs: list[Path] | None = None
) -> Vane | None:
    """The realization of a setup for a target, or None when there is none.

    SoC files come first, then the board's, as the build applies them. A
    vane.yml counts once the setup is realized by the files found so far,
    so a board may state what only it knows about a SoC's realization; the
    last one found, the board's, wins.
    """
    vane = Vane(setup)
    for soc_dir in soc_dirs or []:
        _collect(vane, Path(soc_dir) / 'vanes' / setup, [target.soc_stem()])
    for board_dir in board_dirs:
        _collect(vane, Path(board_dir) / 'vanes' / setup, target.board_stems())
    return vane if vane.files else None


def _collect(vane: Vane, vane_dir: Path, stems: list[str]) -> None:
    if not vane_dir.is_dir():
        return
    for stem in stems:
        for suffix in ('.overlay', '.conf'):
            path = vane_dir / (stem + suffix)
            if path.is_file() and path not in vane.files:
                vane.files.append(path)
    if vane.files:
        fixture = _realization(vane_dir).get('fixture')
        if fixture == NO_FIXTURE:
            raise ValueError(
                f'{vane_dir / REALIZATION_FILE}: name the fixture the scenario names '
                f'to waive it: {{<fixture>: {NO_FIXTURE}}}'
            )
        if fixture is not None:
            vane.fixture = fixture


def fixtures_for(scenario: list[str] | None, found: list[Vane | None]) -> list[str] | None:
    """The fixtures a run needs on the target.

    The scenario's, each replaced or waived by a vane that names it in its
    fixture mapping, plus the fixtures the vanes name on their own. A vane
    owns only the fixtures it names, so the vanes of several setups compose.
    None when the vanes leave the scenario's alone.
    """
    fixtures = list(scenario or [])
    changed = False
    for vane in found:
        if vane is None or vane.fixture is None:
            continue
        if isinstance(vane.fixture, str):
            if vane.fixture not in fixtures:
                fixtures.append(vane.fixture)
                changed = True
            continue
        for named, replacement in vane.fixture.items():
            if named not in fixtures:
                continue
            fixtures.remove(named)
            if replacement != NO_FIXTURE and replacement not in fixtures:
                fixtures.append(replacement)
            changed = True
    return fixtures if changed else None
