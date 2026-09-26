# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the Kconfig preprocessor functions and the prompt hook."""

import textwrap
from types import SimpleNamespace

import kconfigfunctions
import kconfiglib
import pytest


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


PROMPT_HOOK_KCONFIG = textwrap.dedent(
    """
    config EXPERIMENTAL
    \tbool

    config DEPRECATED
    \tbool

    config EXP
    \tbool "Exp"
    \tselect EXPERIMENTAL

    config DEP_COND
    \tbool "Dep cond"
    \tselect DEPRECATED if EXP

    config SPLIT
    \tbool "Split"

    config SPLIT
    \tbool
    \tselect EXPERIMENTAL

    config BOTH
    \tbool "Both"
    \tselect EXPERIMENTAL
    \tselect DEPRECATED

    config ALREADY
    \tbool "Already [EXPERIMENTAL]"
    \tselect EXPERIMENTAL

    config PREFIX
    \tbool "[DEPRECATED] Prefix"
    \tselect DEPRECATED

    config VARIANT
    \tbool "Variant (experimental)"
    \tselect EXPERIMENTAL

    config NUM
    \tint "Num [DEPRECATED]"
    \tdefault 1

    config NONE
    \tbool "None"

    choice
    \tprompt "Choice"

    config C1
    \tbool "C one"
    \tselect EXPERIMENTAL

    endchoice

    menu "Menu"

    comment "Comment"

    endmenu
    """
)


@pytest.fixture
def prompt_hook_kconf(tmp_path, monkeypatch):
    (tmp_path / "Kconfig").write_text(PROMPT_HOOK_KCONFIG)
    monkeypatch.delenv("KCONFIG_FUNCTIONS", raising=False)
    monkeypatch.setenv("srctree", str(tmp_path))
    return kconfiglib.Kconfig("Kconfig", warn_to_stderr=False)


def _prompt(kconf, name, node=0):
    return kconf.syms[name].nodes[node].prompt[0]


def test_prompt_hook_appends_tag(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "EXP") == "Exp [EXPERIMENTAL]"
    assert _prompt(prompt_hook_kconf, "NONE") == "None"


def test_prompt_hook_conditional_select_is_static(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "DEP_COND") == "Dep cond [DEPRECATED]"


def test_prompt_hook_select_in_other_definition_location(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "SPLIT") == "Split [EXPERIMENTAL]"


def test_prompt_hook_both_tags_in_order(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "BOTH") == "Both [EXPERIMENTAL] [DEPRECATED]"


def test_prompt_hook_does_not_duplicate_tag(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "ALREADY") == "Already [EXPERIMENTAL]"
    assert _prompt(prompt_hook_kconf, "PREFIX") == "[DEPRECATED] Prefix"


def test_prompt_hook_ignores_variant_spelling(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "VARIANT") == "Variant (experimental) [EXPERIMENTAL]"


def test_prompt_hook_leaves_non_selecting_types_alone(prompt_hook_kconf):
    assert _prompt(prompt_hook_kconf, "NUM") == "Num [DEPRECATED]"


def test_prompt_hook_choice_symbols_menus_and_comments(prompt_hook_kconf):
    kconf = prompt_hook_kconf

    assert _prompt(kconf, "C1") == "C one [EXPERIMENTAL]"
    assert kconf.choices[0].nodes[0].prompt[0] == "Choice"
    assert kconf.menus[0].prompt[0] == "Menu"
    assert kconf.comments[0].prompt[0] == "Comment"


def test_prompt_hook_source_text_is_kept(prompt_hook_kconf):
    node = prompt_hook_kconf.syms["EXP"].nodes[0]

    assert node.orig_prompt[0] == "Exp"
    assert 'bool "Exp"' in str(node)
