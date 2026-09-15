# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Tests for the SoC lookup logic in scripts/list_hardware.py: the resolution of SoCs declared with
'base', and the selection of the soc.yml trees a lookup reports.
"""

import sys
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).parents[3]
sys.path.insert(0, str(ZEPHYR_BASE / 'scripts'))

from list_hardware import Systems  # noqa: E402

VENDOR_A = '''
family:
  - name: family_a
    series:
      - name: series_a
        socs:
          - name: soc_a
            cpuclusters:
              - name: core0
          - name: soc_a2
'''


def load(*yamls):
    systems = Systems()
    for folder, soc_yaml in yamls:
        systems.extend(Systems(folder, soc_yaml))
    return systems


def names(socs):
    return [s.name for s in socs]


def test_loaded_trees_report_the_whole_tree():
    # A tree is loaded whole, so a lookup of soc_a must also report its siblings, otherwise a
    # SoC whose CONFIG_SOC resolves to soc_a2 would have no SOC_<name>_DIR.
    systems = load(('vendor_a', VENDOR_A))
    socs, series, families = systems.get_loaded_trees(['soc_a'])
    assert names(socs) == ['soc_a', 'soc_a2']
    assert names(series) == ['series_a']
    assert names(families) == ['family_a']


BASED = '''
socs:
  - name: sip_b
    base: soc_a
'''


def test_based_soc_resolves_to_its_base():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', BASED))
    assert systems.resolve_soc_name('sip_b') == 'soc_a'
    assert systems.resolve_soc_name('soc_a') == 'soc_a'


def test_based_soc_keeps_its_name_but_takes_the_base_tree():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', BASED))
    soc = systems.get_soc('sip_b')
    assert soc.name == 'sip_b'
    assert soc.folder == ['vendor_a']


def test_based_soc_loads_only_the_base_tree():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', BASED))
    socs, _, _ = systems.get_loaded_trees(['sip_b'])
    assert {f for s in socs for f in s.folder} == {'vendor_a'}


def test_based_soc_is_not_reported_as_a_soc():
    # It owns no configuration, so it must not appear as a SoC in its own right.
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', BASED))
    assert 'sip_b' not in names(systems.get_socs())


def test_soc_based_on_a_based_soc_is_rejected():
    chained = 'socs:\n  - name: chained\n    base: sip_b\n'
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', BASED), ('vendor_c', chained))
    with pytest.raises(SystemExit) as exc:
        systems.resolve_soc_name('chained')
    assert 'itself based on another SoC' in str(exc.value)


def test_base_naming_an_unknown_soc_is_rejected():
    systems = load(('vendor_b', BASED))
    with pytest.raises(SystemExit) as exc:
        systems.resolve_soc_name('sip_b')
    assert 'is not found' in str(exc.value)


def test_conflicting_base_declarations_are_rejected():
    other = 'socs:\n  - name: sip_b\n    base: soc_a2\n'
    with pytest.raises(SystemExit) as exc:
        load(('vendor_a', VENDOR_A), ('vendor_b', BASED), ('vendor_x', other))
    assert 'declared twice' in str(exc.value)


def test_based_soc_cannot_be_extended():
    # It owns nothing to extend, so extending it must fail rather than quietly do nothing.
    extension = "socs:\n  - extend: sip_b\n    cpuclusters:\n      - name: extra\n"
    with pytest.raises(SystemExit) as exc:
        load(('vendor_a', VENDOR_A), ('vendor_b', BASED), ('vendor_x', extension))
    assert 'cannot be extended' in str(exc.value)


def test_based_soc_picks_up_an_extension_of_its_base():
    extension = "socs:\n  - extend: soc_a\n    cpuclusters:\n      - name: core1\n"
    for order in (
        (('vendor_a', VENDOR_A), ('vendor_b', BASED), ('vendor_e', extension)),
        (('vendor_a', VENDOR_A), ('vendor_e', extension), ('vendor_b', BASED)),
    ):
        systems = load(*order)
        soc = systems.get_soc('sip_b')
        assert soc.cpuclusters == ['core0', 'core1']
        # the extending root's tree is loaded too, since the base is described by both
        socs, _, _ = systems.get_loaded_trees(['sip_b'])
        assert {f for s in socs for f in s.folder} == {'vendor_a', 'vendor_e'}


NESTED_BASED = """
family:
  - name: family_n
    series:
      - name: series_n
        socs:
          - name: die_n
          - name: sip_n
            base: die_n
"""


def test_base_works_on_a_soc_nested_under_a_series():
    # Most vendors declare their SoCs under a family/series rather than at the top level.
    systems = load(("vendor_n", NESTED_BASED))
    assert systems.resolve_soc_name("sip_n") == "die_n"
    assert "sip_n" not in names(systems.get_socs())
    assert names(systems.get_socs()) == ["die_n"]
    soc = systems.get_soc("sip_n")
    assert soc.name == "sip_n"
    assert soc.series == "series_n" and soc.family == "family_n"


DIE_KCONFIG = """config SOC_A_ODDLY_NAMED
	bool

config SOC
	default "soc_a" if SOC_A_ODDLY_NAMED
"""


def test_base_symbol_is_discovered_from_the_base_tree(tmp_path):
    # The symbol is found by scanning for the 'config SOC' default naming the base, so it works
    # even where the symbol does not follow the SOC_<NAME> convention.
    (tmp_path / "Kconfig.soc").write_text(DIE_KCONFIG)
    vendor_a = "socs:\n  - name: soc_a\n"
    systems = load((str(tmp_path), vendor_a), ("vendor_b", BASED))
    assert systems.get_base_symbol("sip_b") == "SOC_A_ODDLY_NAMED"


def test_base_symbol_missing_from_the_base_tree_is_an_error(tmp_path):
    (tmp_path / "Kconfig.soc").write_text("config SOC_A\n\tbool\n")
    vendor_a = "socs:\n  - name: soc_a\n"
    systems = load((str(tmp_path), vendor_a), ("vendor_b", BASED))
    with pytest.raises(SystemExit) as exc:
        systems.get_base_symbol("sip_b")
    assert "cannot determine the Kconfig symbol" in str(exc.value)
