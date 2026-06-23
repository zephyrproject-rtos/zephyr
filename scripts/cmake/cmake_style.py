#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

"""
Style checker for Zephyr CMake files (CMakeLists.txt and *.cmake).

Checks a subset of the "CMake Style Guidelines" documented in
doc/contribute/style/cmake.rst:

  1. Indentation: use 2 spaces per block level, never tabs.
  2. Commands: always use lowercase command names.
  3. No space between a command and its opening parenthesis ('if(' not 'if (').
  4. Cache/option variables (option(...) and set(... CACHE ...)) use UPPERCASE names.
  5. Boolean values are not quoted, in the positions where a boolean is expected:
     the value of option(...) and the value of set(... CACHE BOOL ...).

Line length is intentionally not enforced: CMake's lack of line continuations
makes many long lines (long paths, generator expressions, message text) awkward
or impossible to wrap, so the column limit is left as a documented guideline.

The structural rules are derived from tree-sitter-cmake's concrete syntax tree
rather than from regexes over raw text (which trip over parentheses inside
strings, comments and 'if()' expressions): a command is any '*_command' node,
its name is the first named child, and indentation depth is simply the number of
enclosing 'body' nodes (so else/elseif/endif naturally get the outer depth).

Any style issues are printed as '<file>:<line>:<col>: [<rule>] <message>'. The
exit status distinguishes "found issues" from "the checker failed":

  0 (EXIT_OK)     - no style issues found
  1 (EXIT_ISSUES) - style issues were found and printed to stdout
  2 (EXIT_ERROR)  - the checker itself failed (e.g. a file could not be read);
                    a message is printed to stderr

Keeping these distinct lets callers (such as the CI compliance check) tell a
clean "issues found" run apart from a crash, instead of guessing from stderr.

Requires: pip install tree-sitter tree-sitter-cmake
"""

import argparse
import sys
import traceback
from dataclasses import dataclass
from pathlib import Path

import tree_sitter_cmake
from tree_sitter import Language, Node, Parser

INDENT = 2

# CMake boolean constants that should not be quoted (rule 5).
CMAKE_BOOLEANS = {"ON", "OFF", "TRUE", "FALSE"}

# Commands that are an exception to the lowercase rule (rule 2): CMake module
# commands, and Zephyr/sysbuild commands that extend a mixed-case command, use a
# mixed-case 'Module_Action' convention. For these the canonical mixed-case
# spelling is the only accepted form, so both an all-lowercase and any other
# casing are flagged and corrected to the literal below. Built-in CMake commands
# are not listed here, so an uppercase 'FILE(' or 'SET(' is still caught by the
# lowercase rule.
MIXED_CASE_COMMANDS = {
    # ExternalProject module (https://cmake.org/cmake/help/latest/module/ExternalProject.html).
    "ExternalProject_Add",
    "ExternalProject_Add_Step",
    "ExternalProject_Add_StepTargets",
    "ExternalProject_Add_StepDependencies",
    "ExternalProject_Get_Property",
    # FetchContent module (https://cmake.org/cmake/help/latest/module/FetchContent.html).
    "FetchContent_Declare",
    "FetchContent_MakeAvailable",
    "FetchContent_Populate",
    "FetchContent_GetProperties",
    "FetchContent_SetPopulated",
    # Zephyr sysbuild extensions, modeled after ExternalProject_Add.
    "ExternalZephyrProject_Add",
    "ExternalZephyrVariantProject_Add",
    "ExternalZephyrProject_Cmake",
}

# Lookup from the lowercased command name to its canonical mixed-case spelling.
_MIXED_CASE_BY_LOWER = {name.lower(): name for name in MIXED_CASE_COMMANDS}

EXIT_OK = 0
EXIT_ISSUES = 1
EXIT_ERROR = 2

_PARSER = Parser(Language(tree_sitter_cmake.language()))


@dataclass
class Issue:
    """A single style violation, used for reporting."""

    line: int  # 1-based
    col: int  # 1-based
    rule: str
    message: str


def _text(node: Node) -> str:
    """Decoded source text of a node ('' if the node carries no text)."""
    return node.text.decode("utf-8") if node.text is not None else ""


def _tab_indent_issues(lines: list[str]) -> list[Issue]:
    """Rule 1 (part): indentation must use spaces, not tabs."""
    issues: list[Issue] = []
    for i, line in enumerate(lines, start=1):
        indent = line[: len(line) - len(line.lstrip())]
        if "\t" in indent:
            issues.append(
                Issue(i, indent.index("\t") + 1, "tab-indent", "use spaces, not tabs, to indent")
            )
    return issues


def _check_command(node: Node, depth: int, lines: list[str], issues: list[Issue]) -> None:
    name_node = node.named_children[0] if node.named_children else None
    if name_node is None:
        return
    name = _text(name_node)
    name_line = name_node.start_point.row + 1
    name_col = name_node.start_point.column

    # Rule 2: lowercase command, except module/sysbuild commands whose canonical
    # names are mixed-case by convention. For those the canonical spelling is the
    # only accepted form; everything else must be lowercase.
    canonical = _MIXED_CASE_BY_LOWER.get(name.lower())
    if canonical is not None:
        if name != canonical:
            issues.append(Issue(name_line, name_col + 1, "command-case", f"use '{canonical}'"))
    elif name != name.lower():
        issues.append(
            Issue(
                name_line,
                name_col + 1,
                "command-case",
                f"use lowercase command '{name.lower()}'",
            )
        )

    # Rule 3: no space before the opening parenthesis. CMake only allows spaces
    # and tabs (not newlines) between a command name and its '(', so any gap here
    # is whitespace on the same line.
    lparen = next((c for c in node.children if c.type == "("), None)
    if lparen is not None and lparen.start_byte > name_node.end_byte:
        issues.append(
            Issue(
                name_line,
                name_col + len(name) + 1,
                "paren-space",
                f"remove the space before '(' in '{name}'",
            )
        )

    # Rule 1 (depth): a statement-leading command sits at depth * INDENT spaces.
    line, col = node.start_point.row + 1, node.start_point.column
    prefix = lines[line - 1][:col]
    if prefix.strip() == "" and "\t" not in prefix and col != depth * INDENT:
        issues.append(
            Issue(
                line,
                col + 1,
                "indent",
                f"expected {depth * INDENT} spaces of indentation, found {col}",
            )
        )

    # Rules 4 and 5: cache/option variable casing and quoted boolean values.
    if name.lower() in ("set", "option"):
        _check_cache_var(node, name, issues)
        _check_quoted_bool(node, name, issues)


def _command_args(node: Node) -> list[Node]:
    """The positional 'argument' nodes of a command, in order."""
    arg_list = next((c for c in node.children if c.type == "argument_list"), None)
    if arg_list is None:
        return []
    return [c for c in arg_list.children if c.type == "argument"]


def _quoted_bool(arg: Node) -> str | None:
    """If 'arg' is a quoted CMake boolean, return its (unquoted) text, else None."""
    quoted = next((c for c in arg.children if c.type == "quoted_argument"), None)
    if quoted is None:
        return None
    content = _text(quoted)[1:-1]
    return content if content.upper() in CMAKE_BOOLEANS else None


def _check_cache_var(node: Node, command: str, issues: list[Issue]) -> None:
    args = _command_args(node)
    if not args:
        return
    # option() always defines a cache variable; set() only with a CACHE keyword.
    if command.lower() != "option" and not any(a.text == b"CACHE" for a in args):
        return
    name_arg = args[0]
    var = _text(name_arg)
    # Skip computed (${...}) or quoted names.
    if "$" in var or var.startswith(('"', "'")):
        return
    # Only flag all-lowercase names. Mixed-case names are typically dictated by
    # CMake convention (e.g. Python3_EXECUTABLE, <Pkg>_FOUND) and can't be
    # renamed, so flagging them would be a false positive.
    if var == var.lower() and var != var.upper():
        issues.append(
            Issue(
                name_arg.start_point.row + 1,
                name_arg.start_point.column + 1,
                "cache-var-case",
                f"use an UPPERCASE name for cache variable '{var}'",
            )
        )


def _check_quoted_bool(node: Node, command: str, issues: list[Issue]) -> None:
    # Rule 5: do not quote a boolean value where a boolean is expected. Only the
    # value of option() and the value of set(... CACHE BOOL ...) are checked;
    # elsewhere a quoted ON/OFF/TRUE/FALSE is a legitimate string (e.g. a
    # string(REPLACE ...) operand or a CACHE STRING value).
    args = _command_args(node)
    if command.lower() == "option":
        # option(<variable> "<help_text>" [<value>])
        values = args[2:3]
    else:
        # set(<variable> <value>... CACHE BOOL "<help>" [FORCE])
        cache = next((i for i, a in enumerate(args) if a.text == b"CACHE"), None)
        if cache is None or args[cache + 1 : cache + 2] == [] or args[cache + 1].text != b"BOOL":
            return
        values = args[1:cache]

    for arg in values:
        content = _quoted_bool(arg)
        if content is not None:
            issues.append(
                Issue(
                    arg.start_point.row + 1,
                    arg.start_point.column + 1,
                    "quoted-bool",
                    f"do not quote the boolean value {content}",
                )
            )


def _tree_issues(root: Node, lines: list[str]) -> list[Issue]:
    """Rules 1 (depth), 2, 3, 4 and 5, derived from the tree-sitter-cmake tree."""
    issues: list[Issue] = []

    def walk(node: Node, depth: int) -> None:
        node_type = node.type
        if node_type.endswith("_command"):
            _check_command(node, depth, lines, issues)
        child_depth = depth + 1 if node_type == "body" else depth
        for child in node.children:
            walk(child, child_depth)

    walk(root, 0)
    return issues


def check_text(text: str) -> list[Issue]:
    """Return the list of style issues for the given CMake file contents."""
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines = lines[:-1]
    # Drop a trailing CR so the tab/column checks don't trip over it on CRLF
    # files (the parser works on the original bytes; a line-terminal CR shifts no
    # token columns).
    lines = [line[:-1] if line.endswith("\r") else line for line in lines]
    root = _PARSER.parse(text.encode("utf-8")).root_node
    issues = _tab_indent_issues(lines) + _tree_issues(root, lines)
    issues.sort(key=lambda issue: (issue.line, issue.col))
    return issues


def check_file(path: Path) -> list[Issue]:
    """Return the list of style issues for a single file."""
    return check_text(path.read_text(encoding="utf-8"))


def _is_cmake_file(name: str) -> bool:
    return name == "CMakeLists.txt" or name.endswith(".cmake")


def expand_paths(paths: list[str]) -> list[Path]:
    """
    Expand the given paths into a de-duplicated list of files. Files are used
    as-is; directories are searched recursively for CMake files (sorted).
    """
    files: list[Path] = []
    for path in paths:
        p = Path(path)
        if p.is_dir():
            files.extend(sorted(f for f in p.rglob("*") if f.is_file() and _is_cmake_file(f.name)))
        else:
            files.append(p)
    # Preserve order while dropping duplicates.
    return list(dict.fromkeys(files))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__,
        allow_abbrev=False,
    )
    parser.add_argument(
        "paths",
        metavar="PATH",
        nargs="+",
        help="CMake files to check. Directories are searched recursively for "
        "CMakeLists.txt and *.cmake files.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    total = 0
    failed = False
    for path in expand_paths(args.paths):
        try:
            issues = check_file(path)
        except Exception:
            # Never let a checker failure masquerade as "no issues" (exit 0) or
            # "issues found" (exit 1): report it and exit with EXIT_ERROR.
            print(f"error: failed to check '{path}':", file=sys.stderr)
            traceback.print_exc()
            failed = True
            continue
        for issue in issues:
            print(f"{path}:{issue.line}:{issue.col}: [{issue.rule}] {issue.message}")
            total += 1

    if failed:
        return EXIT_ERROR
    return EXIT_ISSUES if total else EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
