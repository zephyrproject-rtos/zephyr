# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from pylint.checkers import BaseChecker

import astroid
from astroid import nodes

class ZephyrArgParseChecker(BaseChecker):
    """Class implementing function checker for Zephyr."""

    # The name defines a custom section of the config for this checker.
    name = "zephyr-arg-parse"

    # Register messages emitted by the checker.
    msgs = {
        "E9901": (
            "Argument parser with abbreviations is disallowed",
            "argument-parser-with-abbreviations",
            "An ArgumentParser object must set `allow_abbrev=false` to disable "
            "abbreviations and prevent issues with these being used by projects"
            " and/or scripts."
        )
    }

    # Function that looks at evert function call for ArgumentParser invocation
    def visit_call(self, node: nodes.Call) -> None:
        if isinstance(node.func, astroid.nodes.node_classes.Attribute) and \
           node.func.attrname == "ArgumentParser":
            abbrev_disabled = False

            # Check that allow_abbrev is set and that the value is False
            for keyword in node.keywords:
                if keyword.arg == "allow_abbrev":
                    if not isinstance(keyword.value, astroid.nodes.node_classes.Const):
                        continue
                    if keyword.value.pytype() != "builtins.bool":
                        continue
                    if keyword.value.value is False:
                        abbrev_disabled = True

            if abbrev_disabled is False:
                self.add_message(
                    "argument-parser-with-abbreviations", node=node
                )

        return ()

# This is called from pylint, hence PyLinter not being declared in this file
# pylint: disable=undefined-variable
def register(linter: PyLinter) -> None:
    linter.register_checker(ZephyrArgParseChecker(linter))
