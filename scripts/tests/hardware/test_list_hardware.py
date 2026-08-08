# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Tests for the SoC lookup logic in scripts/list_hardware.py: the resolution of the 'requires'
property which lets a SoC pull in the SoC trees it is defined in terms of, and the selection of
the soc.yml trees a lookup reports.
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
          - name: soc_a2
'''

VENDOR_B = '''
socs:
  - name: sip_b
    requires:
      - soc_a
'''

VENDOR_C = '''
socs:
  - name: sip_c
    requires:
      - sip_b
'''


def load(*yamls):
    systems = Systems()
    for folder, soc_yaml in yamls:
        systems.extend(Systems(folder, soc_yaml))
    return systems


def names(socs):
    return [s.name for s in socs]


def test_closure_of_soc_without_requires():
    systems = load(('vendor_a', VENDOR_A))
    assert names(systems.get_soc_closure(['soc_a'])) == ['soc_a']


def test_closure_pulls_in_required_soc():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', VENDOR_B))
    closure = systems.get_soc_closure(['sip_b'])
    assert names(closure) == ['sip_b', 'soc_a']
    assert sorted(f for s in closure for f in s.folder) == ['vendor_a', 'vendor_b']


def test_closure_is_transitive():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', VENDOR_B), ('vendor_c', VENDOR_C))
    assert names(systems.get_soc_closure(['sip_c'])) == ['sip_c', 'sip_b', 'soc_a']


def test_closure_does_not_pull_in_unrelated_socs():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', VENDOR_B))
    assert 'soc_a2' not in names(systems.get_soc_closure(['sip_b']))


def test_closure_of_cyclic_requires_terminates():
    cyclic_x = 'socs:\n  - name: soc_x\n    requires:\n      - soc_y\n'
    cyclic_y = 'socs:\n  - name: soc_y\n    requires:\n      - soc_x\n'
    systems = load(('vendor_x', cyclic_x), ('vendor_y', cyclic_y))
    assert names(systems.get_soc_closure(['soc_x'])) == ['soc_x', 'soc_y']


def test_closure_of_unknown_required_soc_errors_out():
    systems = load(('vendor_b', VENDOR_B))
    with pytest.raises(SystemExit) as exc:
        systems.get_soc_closure(['sip_b'])
    assert "required by SoC 'sip_b'" in str(exc.value)


def test_requires_can_be_added_by_extending_a_soc():
    extension = 'socs:\n  - extend: soc_a\n    requires:\n      - soc_a2\n'
    systems = load(('vendor_a', VENDOR_A), ('vendor_ext', extension))
    assert names(systems.get_soc_closure(['soc_a'])) == ['soc_a', 'soc_a2']


def test_loaded_trees_report_the_whole_tree():
    # A tree is loaded whole, so a lookup of soc_a must also report its siblings, otherwise a
    # SoC whose CONFIG_SOC resolves to soc_a2 would have no SOC_<name>_DIR.
    systems = load(('vendor_a', VENDOR_A))
    socs, series, families = systems.get_loaded_trees(['soc_a'])
    assert names(socs) == ['soc_a', 'soc_a2']
    assert names(series) == ['series_a']
    assert names(families) == ['family_a']


def test_loaded_trees_follow_requires_across_trees():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', VENDOR_B), ('vendor_c', VENDOR_C))
    socs, _, families = systems.get_loaded_trees(['sip_c'])
    # sip_c requires sip_b which requires soc_a, so all three trees are reported whole.
    assert set(names(socs)) == {'sip_c', 'sip_b', 'soc_a', 'soc_a2'}
    assert names(families) == ['family_a']


def test_loaded_trees_of_an_independent_soc_pull_in_nothing_else():
    systems = load(('vendor_a', VENDOR_A), ('vendor_b', VENDOR_B))
    socs, _, families = systems.get_loaded_trees(['soc_a'])
    assert 'sip_b' not in names(socs)
    assert names(families) == ['family_a']
