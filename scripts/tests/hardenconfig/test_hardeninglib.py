#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the hardening database loader (scripts/kconfig/hardeninglib.py)."""

import os
import sys
import textwrap

import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "kconfig"))

import hardeninglib  # noqa: E402
import kconfiglib  # noqa: E402

VALID_DB = """
profiles:
  base:
    description: Baseline hardening.
  strict:
    extends: base
    description: Baseline plus debug feature removal.

rules:
  BOOL_OFF_RULE:
    value: n
    rationale: Feature discloses information.
    references: [CWE-200]
  BOOL_ON_RULE:
    value: y
    rationale: Protective feature.
  INT_MIN_RULE:
    min: 100
    rationale: Zero disables the mitigation.
  STRICT_ONLY_RULE:
    value: n
    profiles: [strict]
    rationale: Debug feature.
"""


def write_db(tmp_path, content, name="hardening.yaml"):
    path = tmp_path / name
    path.write_text(textwrap.dedent(content))
    return path


def test_load_valid_database(tmp_path):
    db = hardeninglib.load_database([write_db(tmp_path, VALID_DB)])

    assert set(db["profiles"]) == {"base", "strict"}
    assert db["profiles"]["strict"]["extends"] == "base"

    rule = db["rules"]["BOOL_OFF_RULE"]
    assert rule["value"] == "n"
    assert rule["min"] is None
    assert rule["references"] == ["CWE-200"]
    assert rule["profiles"] == ["base"]
    assert rule["rationale"] == "Feature discloses information."
    assert rule["source"].endswith("hardening.yaml")

    assert db["rules"]["INT_MIN_RULE"]["value"] is None
    assert db["rules"]["INT_MIN_RULE"]["min"] == 100
    assert db["rules"]["STRICT_ONLY_RULE"]["profiles"] == ["strict"]


@pytest.mark.parametrize(
    "content",
    [
        # missing rationale
        """
        profiles:
          base: {description: d}
        rules:
          FOO:
            value: n
        """,
        # both value and min
        """
        profiles:
          base: {description: d}
        rules:
          FOO:
            value: 1
            min: 1
            rationale: r
        """,
        # neither value nor min/max
        """
        profiles:
          base: {description: d}
        rules:
          FOO:
            rationale: r
        """,
        # lowercase symbol name
        """
        profiles:
          base: {description: d}
        rules:
          foo:
            value: n
            rationale: r
        """,
        # unknown rule property
        """
        profiles:
          base: {description: d}
        rules:
          FOO:
            value: n
            rationale: r
            severity: high
        """,
        # blank rationale
        """
        profiles:
          base: {description: d}
        rules:
          FOO:
            value: n
            rationale: "   "
        """,
        # profile without description
        """
        profiles:
          base: {}
        rules: {}
        """,
        # neither profiles nor rules
        """
        {}
        """,
    ],
)
def test_schema_rejects(tmp_path, content):
    with pytest.raises(hardeninglib.HardeningDatabaseError):
        hardeninglib.load_database([write_db(tmp_path, content)])


def test_unsatisfiable_constraint_rejected(tmp_path):
    content = """
    profiles:
      base: {description: d}
    rules:
      FOO:
        min: 100
        max: 5
        rationale: r
    """
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="greater than max"):
        hardeninglib.load_database([write_db(tmp_path, content)])


def test_load_missing_file(tmp_path):
    with pytest.raises(hardeninglib.HardeningDatabaseError):
        hardeninglib.load_database([tmp_path / "nonexistent.yaml"])


@pytest.mark.parametrize(
    "value,expected",
    [
        (True, "y"),
        (False, "n"),
        ("y", "y"),
        ("n", "n"),
        (100, "100"),
        ("100", "100"),
    ],
)
def test_value_normalization(tmp_path, value, expected):
    yaml_value = {True: "true", False: "false"}.get(value, value)
    db = hardeninglib.load_database(
        [
            write_db(
                tmp_path,
                f"""
                profiles:
                  base: {{description: d}}
                rules:
                  FOO:
                    value: {yaml_value}
                    rationale: r
                """,
            )
        ]
    )
    assert db["rules"]["FOO"]["value"] == expected


def test_profile_closure_and_filtering(tmp_path):
    db = hardeninglib.load_database([write_db(tmp_path, VALID_DB)])

    assert hardeninglib.profile_closure(db["profiles"], "base") == {"base"}
    assert hardeninglib.profile_closure(db["profiles"], "strict") == {
        "base",
        "strict",
    }

    base_rules = hardeninglib.rules_for_profile(db, "base")
    assert "STRICT_ONLY_RULE" not in base_rules
    assert "BOOL_OFF_RULE" in base_rules

    strict_rules = hardeninglib.rules_for_profile(db, "strict")
    assert set(strict_rules) == set(db["rules"])


def test_unknown_profile(tmp_path):
    db = hardeninglib.load_database([write_db(tmp_path, VALID_DB)])
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="unknown profile"):
        hardeninglib.rules_for_profile(db, "nonexistent")


def test_extends_cycle(tmp_path):
    db = hardeninglib.load_database(
        [
            write_db(
                tmp_path,
                """
                profiles:
                  a:
                    extends: b
                    description: d
                  b:
                    extends: a
                    description: d
                rules: {}
                """,
            )
        ]
    )
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="cycle"):
        hardeninglib.profile_closure(db["profiles"], "a")

    errors = hardeninglib.check_profile_integrity(db)
    assert any("cycle" in e for e in errors)


def test_profile_integrity(tmp_path):
    db = hardeninglib.load_database(
        [
            write_db(
                tmp_path,
                """
                profiles:
                  base: {description: d}
                rules:
                  FOO:
                    value: n
                    profiles: [undefined-profile]
                    rationale: r
                """,
            )
        ]
    )
    errors = hardeninglib.check_profile_integrity(db)
    assert len(errors) == 1
    assert "undefined-profile" in errors[0]

    good = hardeninglib.load_database([write_db(tmp_path, VALID_DB)])
    assert hardeninglib.check_profile_integrity(good) == []


def test_extra_sources_merge_and_override(tmp_path):
    extra = write_db(
        tmp_path,
        """
        profiles:
          custom:
            extends: strict
            description: Product profile.
        rules:
          BOOL_OFF_RULE:
            value: y
            rationale: Overridden downstream.
          EXTRA_RULE:
            value: n
            profiles: [custom]
            rationale: Downstream rule.
        """,
        name="extra.yaml",
    )
    db = hardeninglib.load_database([write_db(tmp_path, VALID_DB)], extra_paths=[extra])

    assert set(db["profiles"]) == {"base", "strict", "custom"}
    # extra sources override same-named rules entirely
    assert db["rules"]["BOOL_OFF_RULE"]["value"] == "y"
    assert db["rules"]["BOOL_OFF_RULE"]["references"] == []
    assert "EXTRA_RULE" in db["rules"]

    custom_rules = hardeninglib.rules_for_profile(db, "custom")
    assert "EXTRA_RULE" in custom_rules
    assert "STRICT_ONLY_RULE" in custom_rules


def test_extra_sources_cannot_redefine_profile(tmp_path):
    extra = write_db(
        tmp_path,
        """
        profiles:
          strict:
            description: Redefined downstream, dropping its inheritance.
        """,
        name="extra.yaml",
    )
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="already defined"):
        hardeninglib.load_database([write_db(tmp_path, VALID_DB)], extra_paths=[extra])


FRAGMENT = """
rules:
  FRAGMENT_RULE:
    value: n
    rationale: Fragment rule.
"""


def test_fragments_merge_with_source(tmp_path):
    central = write_db(tmp_path, VALID_DB)
    fragment = write_db(tmp_path, FRAGMENT, name="fragment.yaml")
    db = hardeninglib.load_database([central, fragment])

    assert "FRAGMENT_RULE" in db["rules"]
    assert db["rules"]["FRAGMENT_RULE"]["source"].endswith("fragment.yaml")
    assert db["rules"]["BOOL_OFF_RULE"]["source"].endswith("hardening.yaml")


def test_duplicate_rule_across_files_rejected(tmp_path):
    central = write_db(tmp_path, VALID_DB)
    dup = write_db(
        tmp_path,
        """
        rules:
          BOOL_OFF_RULE:
            value: y
            rationale: Conflicting duplicate.
        """,
        name="duplicate.yaml",
    )
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="already defined"):
        hardeninglib.load_database([central, dup])


@pytest.mark.parametrize(
    "content",
    [
        """
        rules:
          BOOL_OFF_RULE:
            value: n
            rationale: First definition.
          BOOL_OFF_RULE:
            value: y
            rationale: Second definition.
        """,
        """
        profiles:
          base:
            description: First definition.
          base:
            description: Second definition.
        """,
    ],
    ids=["rule", "profile"],
)
def test_duplicate_key_in_one_file_rejected(tmp_path, content):
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="duplicate key"):
        hardeninglib.load_database([write_db(tmp_path, content)])


def test_profiles_only_in_first_file(tmp_path):
    central = write_db(tmp_path, VALID_DB)
    fragment = write_db(
        tmp_path,
        """
        profiles:
          rogue: {description: d}
        rules: {}
        """,
        name="rogue.yaml",
    )
    with pytest.raises(hardeninglib.HardeningDatabaseError, match="profiles may only"):
        hardeninglib.load_database([central, fragment])


def test_discover_fragments(tmp_path):
    for rel in [
        "subsys/foo/hardening.yaml",
        "drivers/bar/hardening.yaml",
        "arch/hardening.yaml",
        "samples/baz/hardening.yaml",  # excluded root
        "subsys/foo/hardening_extra.yaml",  # wrong name
    ]:
        path = tmp_path / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(FRAGMENT))

    found = hardeninglib.discover_fragments(tmp_path)
    rels = [p.relative_to(tmp_path).as_posix() for p in found]
    assert rels == [
        "arch/hardening.yaml",
        "drivers/bar/hardening.yaml",
        "subsys/foo/hardening.yaml",
    ]


def test_recommended_str():
    def rule(**kwargs):
        return {"value": None, "min": None, "max": None, **kwargs}

    assert hardeninglib.recommended_str(rule(value="y")) == "y"
    assert hardeninglib.recommended_str(rule(min=100)) == ">=100"
    assert hardeninglib.recommended_str(rule(max=5)) == "<=5"
    assert hardeninglib.recommended_str(rule(min=1, max=5)) == ">=1, <=5"


@pytest.fixture(scope="module")
def kconf(tmp_path_factory):
    kconfig = tmp_path_factory.mktemp("kconfig") / "Kconfig"
    kconfig.write_text(
        textwrap.dedent(
            """
            config BOOL_ON
                bool "bool on"
                default y

            config BOOL_OFF
                bool "bool off"

            config INT_100
                int "int"
                default 100

            config HEX_100
                hex "hex"
                default 0x64

            config HEX_UNPREFIXED
                hex "hex without the 0x prefix"
                default 64
            """
        )
    )
    return kconfiglib.Kconfig(str(kconfig))


@pytest.mark.parametrize(
    "symbol,rule,expected",
    [
        ("BOOL_ON", {"value": "y"}, "PASS"),
        ("BOOL_ON", {"value": "n"}, "FAIL"),
        ("BOOL_OFF", {"value": "n"}, "PASS"),
        ("BOOL_OFF", {"value": "y"}, "FAIL"),
        ("INT_100", {"value": "100"}, "PASS"),
        ("INT_100", {"value": 100}, "PASS"),
        ("INT_100", {"min": 100}, "PASS"),
        ("INT_100", {"min": 101}, "FAIL"),
        ("INT_100", {"max": 99}, "FAIL"),
        ("INT_100", {"min": 1, "max": 200}, "PASS"),
        # hex values are read in base 16, whether or not YAML resolved the
        # recommendation ('value: 0x64') to a decimal integer
        ("HEX_100", {"value": 100}, "PASS"),
        ("HEX_100", {"value": "0x64"}, "PASS"),
        ("HEX_100", {"value": 64}, "FAIL"),
        ("HEX_100", {"min": 100}, "PASS"),
        ("HEX_100", {"min": 101}, "FAIL"),
        # the '0x' prefix is optional in Kconfig and Symbol.str_value keeps
        # the spelling used there
        ("HEX_UNPREFIXED", {"value": 100}, "PASS"),
        ("HEX_UNPREFIXED", {"min": 100}, "PASS"),
        ("HEX_UNPREFIXED", {"min": 101}, "FAIL"),
        # non-numeric current value on a constraint rule
        ("BOOL_ON", {"min": 1}, "FAIL"),
        (None, {"value": "y"}, "NA"),
    ],
)
def test_evaluate_rule(kconf, symbol, rule, expected):
    full_rule = {"value": None, "min": None, "max": None, **rule}
    if "value" in rule:
        full_rule["value"] = hardeninglib._normalize_value(rule["value"])
    sym = kconf.syms[symbol] if symbol else None
    assert hardeninglib.evaluate_rule(full_rule, sym) == expected


@pytest.mark.skipif(
    not hardeninglib.DEFAULT_DATABASE_PATH.exists(),
    reason="in-tree hardening database not present",
)
def test_in_tree_database_is_valid():
    db = hardeninglib.load_database()
    assert hardeninglib.check_profile_integrity(db) == []
    assert {"base", "strict"}.issubset(db["profiles"])
    # the default profile every untagged rule lands in must exist
    assert hardeninglib.DEFAULT_PROFILE in db["profiles"]
