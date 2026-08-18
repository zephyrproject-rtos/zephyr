#!/usr/bin/env python3

# Copyright (c) 2019-2024 Intel Corporation
# Copyright (c) 2026 The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check the current configuration against Zephyr's security hardening
database (scripts/kconfig/hardening.yaml) and report deviations, with the
rationale for each recommendation.

Environment variables (typically set through CMake cache variables of the
same name, e.g. 'west build -t hardenconfig -- -DHARDENCONFIG_PROFILE=base'):

- HARDENCONFIG_PROFILE: hardening profile to check against (default: strict)
- HARDENCONFIG_SHOW_ALL: also show passing and non-applicable options
- HARDENCONFIG_STRICT: exit with an error code if any check fails
- HARDENCONFIG_JSON: path to additionally write results to, as JSON
- HARDENCONFIG_EXTRA_SOURCES: semicolon-separated list of additional
  hardening database YAML files; later files may add profiles and rules,
  and override same-named rules, but not redefine existing profiles
"""

import json
import os
import sys
import textwrap
from dataclasses import asdict, dataclass, field
from typing import Any

import hardeninglib
from kconfiglib import Kconfig, Symbol, expr_value, standard_kconfig
from tabulate import tabulate

DEFAULT_PROFILE = 'strict'

# Rationale for options flagged through Kconfig marker symbols rather than
# through the hardening database.
MARKER_RATIONALE: dict[str, str] = {
    'EXPERIMENTAL': 'Selects EXPERIMENTAL: the implementation is at an experimental stage.',
    'DEPRECATED': 'Selects DEPRECATED: the feature is deprecated.',
    'NOT_SECURE': 'Selects NOT_SECURE: the feature is inherently not secure.',
}


# How CMake and shells spell a false boolean; anything else that is set
# enables the flag.
FALSE_VALUES = frozenset({'', '0', 'n', 'no', 'off', 'false', 'ignore', 'notfound'})


def env_flag(name: str) -> bool:
    return os.environ.get(name, '').strip().lower() not in FALSE_VALUES


@dataclass
class Option:
    """One evaluated recommendation. Carries no live kconfiglib state so
    that reports are serializable and renderer-agnostic."""

    name: str
    recommended: str
    rationale: str
    result: str  # 'PASS', 'FAIL' or 'NA'
    current: str | None = None
    visible: bool = False
    origin: str = 'database'  # 'database' or 'marker'
    references: list[str] = field(default_factory=list)
    # database file (or Kconfig file, for marker options) the rule comes from
    source: str | None = None


@dataclass
class HardeningReport:
    """The outcome of checking a configuration against a hardening
    profile. This structure (through to_dict()) is the stable interface
    for consumers such as the JSON output."""

    profile: str
    options: list[Option]

    @property
    def failures(self) -> list[Option]:
        """Failing options. Options that do not apply to the configuration
        evaluate to 'NA', so every failure is one the user can act on."""
        return [opt for opt in self.options if opt.result == 'FAIL']

    def to_dict(self) -> dict[str, Any]:
        return {
            'profile': self.profile,
            'fail_count': len(self.failures),
            'options': [asdict(opt) for opt in self.options],
        }


def build_report(
    kconf: Kconfig, rules: dict[str, hardeninglib.Rule], profile: str
) -> HardeningReport:
    options: list[Option] = []

    for name, rule in rules.items():
        symbol = kconf.syms.get(name)
        # A recommendation the target cannot follow (no such symbol, or no
        # visible prompt to change it) is not applicable, not a failure.
        visible = symbol is not None and symbol.visibility != 0
        options.append(
            Option(
                name=name,
                recommended=hardeninglib.recommended_str(rule),
                rationale=rule['rationale'],
                result=hardeninglib.evaluate_rule(rule, symbol if visible else None),
                current=symbol.str_value if symbol is not None else None,
                visible=visible,
                references=list(rule['references']),
                source=rule['source'],
            )
        )

    # Independently of the database, flag options marked in Kconfig itself
    # as experimental, deprecated or not secure. Only 'value'/'min'/'max'
    # matter for evaluation; the rest fills out the Rule shape.
    off_rule: hardeninglib.Rule = {
        'value': 'n',
        'min': None,
        'max': None,
        'rationale': '',
        'references': [],
        'profiles': [],
        'source': '',
    }
    markers = {marker: kconf.syms[marker] for marker in MARKER_RATIONALE}
    seen = set(rules)
    for node in kconf.node_iter():
        if not isinstance(node.item, Symbol) or node.item.name in seen:
            continue
        for target, cond, _ in node.selects:
            for marker, marker_sym in markers.items():
                # An inactive 'select ... if ...' does not mark the symbol.
                if target is marker_sym and expr_value(cond):
                    seen.add(node.item.name)
                    visible = node.item.visibility != 0
                    options.append(
                        Option(
                            name=node.item.name,
                            recommended='n',
                            rationale=MARKER_RATIONALE[marker],
                            result=hardeninglib.evaluate_rule(
                                off_rule, node.item if visible else None
                            ),
                            current=node.item.str_value,
                            visible=visible,
                            origin='marker',
                            source=node.filename,
                        )
                    )
                    break
            if node.item.name in seen:
                break

    return HardeningReport(profile=profile, options=options)


def write_json(report: HardeningReport, path: str) -> None:
    with open(path, 'w') as out:
        json.dump(report.to_dict(), out, indent=2)
        out.write('\n')


def render_text(report: HardeningReport, show_all: bool) -> str:
    headers = ['Name', 'Current', 'Recommended', 'Check result', 'Rationale']

    table_data = [
        [
            f'CONFIG_{opt.name}',
            opt.current,
            opt.recommended,
            opt.result,
            textwrap.fill(opt.rationale, width=50),
        ]
        for opt in report.options
        if show_all or opt.result == 'FAIL'
    ]

    lines = [f'Hardening report for profile: {report.profile}']
    if table_data:
        lines.append(tabulate(table_data, headers=headers, tablefmt='grid'))
    n_fail = len(report.failures)
    if n_fail:
        lines.append(f'{n_fail} option(s) deviate from the hardening recommendations.')
    else:
        lines.append('No deviations from the hardening recommendations.')
    return '\n'.join(lines)


def hardenconfig(kconf: Kconfig) -> None:
    kconf.load_config()

    extra_sources = os.environ.get('HARDENCONFIG_EXTRA_SOURCES', '')
    extra_paths = [p for p in extra_sources.split(';') if p]

    profile = os.environ.get('HARDENCONFIG_PROFILE', '') or DEFAULT_PROFILE

    try:
        database = hardeninglib.load_database(extra_paths=extra_paths)
        errors = hardeninglib.check_profile_integrity(database)
        if errors:
            raise hardeninglib.HardeningDatabaseError('\n'.join(errors))
        rules = hardeninglib.rules_for_profile(database, profile)
    except hardeninglib.HardeningDatabaseError as e:
        sys.exit(f'hardenconfig: invalid hardening database: {e}')

    report = build_report(kconf, rules, profile)

    json_path = os.environ.get('HARDENCONFIG_JSON', '')
    if json_path:
        write_json(report, json_path)

    print(render_text(report, show_all=env_flag('HARDENCONFIG_SHOW_ALL')))
    print()

    if env_flag('HARDENCONFIG_STRICT') and report.failures:
        sys.exit(1)


def main() -> None:
    hardenconfig(standard_kconfig(__doc__))


if __name__ == '__main__':
    main()
