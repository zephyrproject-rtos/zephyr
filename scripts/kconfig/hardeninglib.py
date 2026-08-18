# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Loader for the Zephyr security hardening database.

The database is distributed: scripts/kconfig/hardening.yaml defines the
hardening profiles (and rules for symbols defined in the top-level Kconfig
files), while per-Kconfig-symbol rules live in 'hardening.yaml' fragments
next to the subsystem, driver, architecture or SoC Kconfig files they
relate to. All files are validated against
scripts/schemas/hardening-schema.yaml. Each rule recommends either an
exact value or an integer min/max constraint, together with a rationale.

This module is shared between the 'hardenconfig' build target
(scripts/kconfig/hardenconfig.py) and the compliance check
(scripts/ci/check_compliance.py) so that both always agree on which files
are loaded and how they are interpreted.
"""

from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any, Protocol, TypedDict

import jsonschema
import kconfiglib
import yaml
from jsonschema.exceptions import best_match

try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    from yaml import CSafeLoader as _BaseLoader
except ImportError:
    from yaml import SafeLoader as _BaseLoader  # type: ignore


class _Loader(_BaseLoader):
    """Loader that rejects duplicate mapping keys. YAML permits them and
    the stock loaders keep the last occurrence, so a rule (or profile)
    defined twice in one file would be dropped before schema validation
    and the cross-file duplicate check ever run."""

    def construct_mapping(self, node, deep=False):
        self.flatten_mapping(node)
        seen = set()
        for key_node, _ in node.value:
            key = self.construct_object(key_node, deep=deep)
            if key in seen:
                raise yaml.constructor.ConstructorError(
                    None, None, f"duplicate key '{key}'", key_node.start_mark
                )
            seen.add(key)
        return super().construct_mapping(node, deep)


DEFAULT_DATABASE_PATH = Path(__file__).parent / 'hardening.yaml'

ZEPHYR_BASE = Path(__file__).parents[2]

# Roots searched for hardening database fragments. Notably excludes
# samples/ and tests/.
FRAGMENT_ROOTS = ('arch', 'boards', 'drivers', 'kernel', 'lib', 'modules', 'share', 'soc', 'subsys')
FRAGMENT_NAME = 'hardening.yaml'

SCHEMA_PATH = Path(__file__).parents[1] / 'schemas' / 'hardening-schema.yaml'
with open(SCHEMA_PATH, 'rb') as f:
    HARDENING_SCHEMA = yaml.load(f.read(), Loader=_Loader)

VALIDATOR_CLASS = jsonschema.validators.validator_for(HARDENING_SCHEMA)
VALIDATOR_CLASS.check_schema(HARDENING_SCHEMA)
VALIDATOR = VALIDATOR_CLASS(HARDENING_SCHEMA)

# Profile every rule belongs to when it does not list any explicitly.
DEFAULT_PROFILE = 'base'

# The base each numeric Kconfig type spells its values in. Symbol.str_value
# keeps the spelling used in the Kconfig files, where the '0x' prefix of a
# hex value is optional, so the base has to come from the symbol type.
_TYPE_TO_BASE = {kconfiglib.INT: 10, kconfiglib.HEX: 16}


class Rule(TypedDict):
    """A normalized hardening rule, as produced by load_database()."""

    value: str | None
    min: int | None
    max: int | None
    rationale: str
    references: list[str]
    profiles: list[str]
    source: str


class Database(TypedDict):
    """The merged hardening database, as produced by load_database()."""

    profiles: dict[str, dict[str, str]]
    rules: dict[str, Rule]


class _SymbolLike(Protocol):
    """The part of kconfiglib's Symbol interface evaluate_rule() needs."""

    @property
    def str_value(self) -> str: ...

    @property
    def orig_type(self) -> int: ...


class HardeningDatabaseError(Exception):
    """Raised when the hardening database is malformed."""


def _normalize_value(value: bool | int | str) -> str:
    """Normalize a rule 'value' to its .config string representation.

    YAML resolves 'true'/'false' (and 'yes'/'no', 'on'/'off') to booleans
    while 'y'/'n' stay strings, and integers may be written bare. Normalizing
    here makes all those spellings compare correctly against
    kconfiglib's Symbol.str_value.
    """
    if isinstance(value, bool):
        return 'y' if value else 'n'
    return str(value)


def _normalize_rule(name: str, rule: Mapping[str, Any], source: str) -> Rule:
    minimum, maximum = rule.get('min'), rule.get('max')
    if minimum is not None and maximum is not None and minimum > maximum:
        raise HardeningDatabaseError(
            f'{source}: rule {name} has min {minimum} greater than max {maximum}, '
            'which no configuration can satisfy'
        )

    return {
        'value': _normalize_value(rule['value']) if 'value' in rule else None,
        'min': minimum,
        'max': maximum,
        'rationale': rule['rationale'].strip(),
        'references': rule.get('references', []),
        'profiles': rule.get('profiles', [DEFAULT_PROFILE]),
        'source': source,
    }


def discover_fragments(zephyr_base: Path | str | None = None) -> list[Path]:
    """Return the sorted list of hardening database fragments found under
    the fragment roots of the Zephyr tree."""
    base = Path(zephyr_base) if zephyr_base is not None else ZEPHYR_BASE
    fragments: list[Path] = []
    for root in FRAGMENT_ROOTS:
        root_dir = base / root
        if root_dir.is_dir():
            fragments.extend(root_dir.rglob(FRAGMENT_NAME))
    return sorted(fragments)


def _load_file(path: Path | str) -> dict[str, Any]:
    try:
        with open(path, 'rb') as db_file:
            data = yaml.load(db_file.read(), Loader=_Loader)
    except OSError as e:
        raise HardeningDatabaseError(f'{path}: {e}') from e
    except yaml.YAMLError as e:
        raise HardeningDatabaseError(f'{path}: invalid YAML: {e}') from e

    errors = list(VALIDATOR.iter_errors(data))
    if errors:
        err = best_match(errors)
        raise HardeningDatabaseError(f'{path}: {err.message} in {err.json_path}')
    return data


def _source_name(path: Path | str) -> str:
    try:
        return Path(path).resolve().relative_to(ZEPHYR_BASE).as_posix()
    except ValueError:
        return str(path)


def load_database(
    paths: Sequence[Path | str] | None = None, extra_paths: Sequence[Path | str] = ()
) -> Database:
    """Load, schema-validate and merge the hardening database.

    ``paths`` is the in-tree file set: profiles may only be defined in the
    first file, and defining the same symbol in two files is an error.
    When None, it defaults to scripts/kconfig/hardening.yaml plus all
    discovered fragments.

    ``extra_paths`` are additional (out-of-tree) database files; they may
    add profiles and rules, and override same-named rules. Redefining an
    already defined profile is an error: it would silently change the
    meaning of a profile other rules are tagged with.

    Returns a dict with 'profiles' (as in the YAML) and 'rules' (mapping
    symbol name to a normalized rule dict with keys 'value', 'min', 'max',
    'rationale', 'references', 'profiles' and 'source').

    Raises HardeningDatabaseError on unreadable, unparseable or
    schema-invalid input.
    """
    if paths is None:
        paths = [DEFAULT_DATABASE_PATH, *discover_fragments()]

    profiles: dict[str, dict[str, str]] = {}
    rules: dict[str, Rule] = {}
    for i, path in enumerate(paths):
        data = _load_file(path)
        source = _source_name(path)

        if 'profiles' in data:
            if i > 0:
                raise HardeningDatabaseError(
                    f'{path}: profiles may only be defined in '
                    f'{_source_name(paths[0])}, fragments define rules only'
                )
            profiles.update(data['profiles'])

        for name, rule in data.get('rules', {}).items():
            if name in rules:
                raise HardeningDatabaseError(
                    f"{path}: rule {name} is already defined in {rules[name]['source']}"
                )
            rules[name] = _normalize_rule(name, rule, source)

    for path in extra_paths:
        data = _load_file(path)
        source = _source_name(path)
        for name, profile in data.get('profiles', {}).items():
            if name in profiles:
                raise HardeningDatabaseError(
                    f"{path}: profile '{name}' is already defined; extra "
                    'sources may add profiles, not redefine them'
                )
            profiles[name] = profile
        for name, rule in data.get('rules', {}).items():
            rules[name] = _normalize_rule(name, rule, source)

    return {'profiles': profiles, 'rules': rules}


def profile_closure(profiles: Mapping[str, Mapping[str, str]], name: str) -> set[str]:
    """Return the set of profile names 'name' transitively extends,
    including 'name' itself.

    Raises HardeningDatabaseError on an unknown profile or an 'extends'
    cycle.
    """
    closure: list[str] = []
    current: str | None = name
    while current is not None:
        if current in closure:
            cycle = ' -> '.join(closure + [current])
            raise HardeningDatabaseError(f"profile 'extends' cycle: {cycle}")
        if current not in profiles:
            known = ', '.join(sorted(profiles))
            raise HardeningDatabaseError(f"unknown profile '{current}' (known profiles: {known})")
        closure.append(current)
        current = profiles[current].get('extends')
    return set(closure)


def rules_for_profile(database: Database, profile: str) -> dict[str, Rule]:
    """Return the rules active in 'profile', following 'extends'."""
    active = profile_closure(database['profiles'], profile)
    return {
        name: rule
        for name, rule in database['rules'].items()
        if active.intersection(rule['profiles'])
    }


def check_profile_integrity(database: Database) -> list[str]:
    """Return a list of error strings for profile-related inconsistencies:
    rules tagged with undefined profiles, undefined 'extends' targets, or
    'extends' cycles.
    """
    errors: list[str] = []
    profiles = database['profiles']

    for name in profiles:
        try:
            profile_closure(profiles, name)
        except HardeningDatabaseError as e:
            errors.append(str(e))

    for symbol, rule in database['rules'].items():
        for tag in rule['profiles']:
            if tag not in profiles:
                errors.append(f"rule {symbol}: unknown profile '{tag}'")

    return errors


def recommended_str(rule: Rule) -> str:
    """Human-readable recommendation for a rule."""
    if rule['value'] is not None:
        return rule['value']
    if rule['min'] is not None and rule['max'] is not None:
        return f">={rule['min']}, <={rule['max']}"
    if rule['min'] is not None:
        return f">={rule['min']}"
    return f"<={rule['max']}"


def _current_int(symbol: _SymbolLike) -> int | None:
    """Current value of an int/hex symbol as a number, or None if the symbol
    is not numeric or holds an unparseable value."""
    base = _TYPE_TO_BASE.get(symbol.orig_type)
    if base is None:
        return None
    try:
        return int(symbol.str_value, base)
    except ValueError:
        return None


def _recommended_int(value: str) -> int | None:
    """Recommended value as a number, or None if it is not numeric. YAML has
    already resolved a '0x' prefix to a decimal integer, so unlike a symbol
    value this is always read in base 0."""
    try:
        return int(value, 0)
    except ValueError:
        return None


def evaluate_rule(rule: Rule, symbol: _SymbolLike | None) -> str:
    """Evaluate a rule against a kconfiglib Symbol (or None if the symbol
    does not exist in the Kconfig tree, or does not apply to the current
    configuration). Returns 'PASS', 'FAIL' or 'NA'.
    """
    if symbol is None:
        return 'NA'

    current_int = _current_int(symbol)

    if rule['value'] is not None:
        if current_int is None:
            return 'PASS' if symbol.str_value == rule['value'] else 'FAIL'
        return 'PASS' if _recommended_int(rule['value']) == current_int else 'FAIL'

    if current_int is None:
        return 'FAIL'
    if rule['min'] is not None and current_int < rule['min']:
        return 'FAIL'
    if rule['max'] is not None and current_int > rule['max']:
        return 'FAIL'
    return 'PASS'
