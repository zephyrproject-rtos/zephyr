# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for shields_list_contains() whitespace handling."""

from types import SimpleNamespace

import kconfigfunctions


class _Kconf(SimpleNamespace):
    filename = "Kconfig.shield"
    linenr = 5


def test_shields_list_contains_exact_match(monkeypatch):
    monkeypatch.setenv("SHIELD_AS_LIST", "myshield;other")
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, "myshield") == "y"
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, "missing") == "n"


def test_shields_list_contains_missing_env(monkeypatch):
    monkeypatch.delenv("SHIELD_AS_LIST", raising=False)
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, "myshield") == "n"


def test_shields_list_contains_strips_leading_space(monkeypatch, capsys):
    monkeypatch.setenv("SHIELD_AS_LIST", "myshield")
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, " myshield") == "y"
    err = capsys.readouterr().out
    assert 'searching for shield " myshield", did you mean "myshield" (without a space)' in err


def test_shields_list_contains_strips_trailing_space(monkeypatch, capsys):
    monkeypatch.setenv("SHIELD_AS_LIST", "myshield")
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, "myshield ") == "y"
    err = capsys.readouterr().out
    assert 'did you mean "myshield"' in err


def test_shields_list_contains_internal_whitespace(monkeypatch, capsys):
    monkeypatch.setenv("SHIELD_AS_LIST", "my shield")
    assert kconfigfunctions.shields_list_contains(_Kconf(), None, "my shield") == "n"
    err = capsys.readouterr().out
    assert 'contains whitespace' in err
