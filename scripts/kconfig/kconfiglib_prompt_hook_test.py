# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the prompt hook of the user-defined functions module."""

import itertools
import textwrap

import kconfiglib
import pytest

KCONFIG = textwrap.dedent(
    """
    mainmenu "Top menu"

    config FLAG
    \tbool

    config DEP
    \tbool "Dependency"
    \tdefault y

    config PLAIN
    \tbool "unchanged"

    config SEL
    \tbool "Selects flag" if DEP
    \tselect FLAG if DEP

    config UP
    \tstring "upper me"
    \tdefault "x"

    choice
    \tprompt "A choice"

    config CH_A
    \tbool "Choice a"

    endchoice

    menu "A menu"

    comment "A comment"

    endmenu
    """
)

HOOK_MODULE = textwrap.dedent(
    """
    functions = {}
    calls = []

    def prompt_hook(kconf, node, prompt):
        calls.append((node.item, prompt))
        flag = kconf.syms.get("FLAG")
        if flag is not None and any(
            target is flag for target, _, _ in getattr(node.item, "selects", ())
        ):
            prompt += " [FLAG]"
        if prompt.startswith("upper "):
            prompt = prompt.upper()
        return prompt
    """
)

_module_ids = itertools.count()


@pytest.fixture
def load(tmp_path, monkeypatch):
    """Returns a function that parses a Kconfig with a given functions module."""

    def _load(kconfig=KCONFIG, module=HOOK_MODULE):
        (tmp_path / "Kconfig").write_text(kconfig)
        # Unique module name per parse, so that importlib does not return a
        # module from an earlier test
        name = f"prompt_hook_test_mod_{next(_module_ids)}"
        if module is not None:
            (tmp_path / f"{name}.py").write_text(module)
        monkeypatch.syspath_prepend(str(tmp_path))
        monkeypatch.setenv("KCONFIG_FUNCTIONS", name)
        monkeypatch.setenv("srctree", str(tmp_path))
        return kconfiglib.Kconfig("Kconfig", warn_to_stderr=False)

    return _load


def _node(kconf, name):
    return kconf.syms[name].nodes[0]


def test_hook_called_for_every_prompt(load):
    kconf = load()
    prompts = kconf._prompt_hook.__globals__["calls"]

    assert (kconf.top_node.item, "Top menu") in prompts
    assert (kconf.syms["PLAIN"], "unchanged") in prompts
    assert (kconf.choices[0], "A choice") in prompts
    assert (kconfiglib.MENU, "A menu") in prompts
    assert (kconfiglib.COMMENT, "A comment") in prompts
    # Promptless symbols are skipped
    assert not any(item is kconf.syms["FLAG"] for item, _ in prompts)


def test_returned_text_replaces_prompt(load):
    kconf = load()
    node = _node(kconf, "SEL")

    assert node.prompt[0] == "Selects flag [FLAG]"
    # The condition of the prompt is untouched
    assert node.prompt[1] is kconf.syms["DEP"]
    assert _node(kconf, "UP").prompt[0] == "UPPER ME"


def test_orig_prompt_keeps_source_text(load):
    kconf = load()
    node = _node(kconf, "SEL")

    assert node.orig_prompt == ("Selects flag", kconf.syms["DEP"])
    assert 'bool "Selects flag" if DEP' in str(node)
    assert "[FLAG]" not in str(node)
    assert _node(kconf, "UP").orig_prompt[0] == "upper me"


def test_unchanged_prompt_is_not_copied(load):
    kconf = load()
    node = _node(kconf, "PLAIN")

    assert node._src_prompt is None
    assert node.prompt[0] == "unchanged"
    assert node.orig_prompt[0] == "unchanged"


def test_module_without_hook(load):
    kconf = load(module="functions = {}\n")

    assert kconf._prompt_hook is None
    assert _node(kconf, "SEL").prompt[0] == "Selects flag"
    assert _node(kconf, "SEL")._src_prompt is None


def test_missing_module(load):
    kconf = load(module=None)

    assert kconf._prompt_hook is None
    assert _node(kconf, "SEL").prompt[0] == "Selects flag"


def test_non_string_return_is_an_error(load):
    module = "functions = {}\ndef prompt_hook(kconf, node, prompt):\n    return None\n"

    with pytest.raises(kconfiglib.KconfigError, match="prompt_hook\\(\\) returned None"):
        load(module=module)


def test_config_output_is_unchanged(load, tmp_path):
    kconf = load(module="functions = {}\n")
    kconf.write_config(str(tmp_path / "without.config"), header="")
    kconf = load()
    kconf.write_config(str(tmp_path / "with.config"), header="")

    assert (tmp_path / "with.config").read_text() == (tmp_path / "without.config").read_text()
