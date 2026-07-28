#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

"""
Style checker for Zephyr Kconfig files.

Checks the given files against the "Basic Formatting Rules" documented in
doc/contribute/style/kconfig.rst:

  1. Line length: keep lines to 100 columns or fewer. Lines containing a
     Kconfig macro expansion '$(...)' are exempt, as such calls cannot be
     wrapped across lines.
  2. Indentation: use a flat layout with tabs - entries at column 0, properties
     at a single tab, and 'help' entry text at one tab plus two extra spaces.
     Continuation lines may use any number of tabs but must not mix in spaces.
  3. Spacing: leave a single empty line between option declarations.
  4. Comments: format comments as '# Comment' rather than '#Comment'.
  5. Conditional blocks: insert an empty line before/after each top-level 'if'
     and 'endif' statement.
  6. End of file: end the file with exactly one newline.

Any style issues are printed as '<file>:<line>:<col>: [<rule>] <message>'. The
exit status distinguishes "found issues" from "the checker failed":

  0 (EXIT_OK)     - no style issues found
  1 (EXIT_ISSUES) - style issues were found and printed to stdout
  2 (EXIT_ERROR)  - the checker itself failed (e.g. a file could not be read);
                    a message is printed to stderr

Keeping these distinct lets callers (such as the CI compliance check) tell a
clean "issues found" run apart from a crash, instead of guessing from stderr.
"""

import argparse
import re
import sys
import traceback
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path

EXIT_OK = 0
EXIT_ISSUES = 1
EXIT_ERROR = 2

# A tab is rendered as 8 columns when measuring line length, matching the rest
# of the Zephyr coding style.
TAB_WIDTH = 8
MAX_COLUMNS = 100

# Help body text is indented with one tab plus two extra spaces.
HELP_PREFIX = "\t  "

# Line-leading keyword matchers. These cover deliberately different keyword sets:
#   ENTRY_RE    - declarations that need a blank line before them (rule 3)
#   BLOCK_RE    - entries and block delimiters that live at column 0 (rule 2),
#                 matched against a line with its leading tabs removed
#   IF_ENDIF_RE - top-level 'if'/'endif' that need surrounding blanks (rule 5)
#   OPENERS_RE  - block openers after which a declaration needs no blank line
ENTRY_RE = re.compile(r"(menuconfig|config|choice|menu|comment|mainmenu)\b")
BLOCK_RE = re.compile(
    r"(menuconfig|config|choice|endchoice|menu|endmenu|if|endif|comment|mainmenu"
    r"|source|rsource|osource|orsource)\b"
)
IF_ENDIF_RE = re.compile(r"(if|endif)\b")
OPENERS_RE = re.compile(r"(if|menu|choice|mainmenu)\b")

# 'help' (or the legacy '---help---') keyword line, possibly indented with tabs.
HELP_RE = re.compile(r"^\t*(help|---help---)\s*$")


@dataclass
class Issue:
    """A single style violation, used for reporting."""

    line: int  # 1-based
    col: int  # 1-based
    rule: str
    message: str


def _leading_ws(line: str) -> str:
    """Return the leading whitespace (tabs and spaces) of 'line'."""
    return line[: len(line) - len(line.lstrip(" \t"))]


def _columns(line: str) -> int:
    """Return the display width of 'line', expanding tabs."""
    return len(line.expandtabs(TAB_WIDTH))


def _is_continuation(lines: list[str], i: int) -> bool:
    """True if line 'i' continues the previous line via a trailing backslash."""
    return i > 0 and lines[i - 1].rstrip().endswith("\\")


def _comment_start(line: str) -> int:
    """
    Return the index of the '#' that starts a comment on 'line', or -1 if the
    line has no comment. Ignores '#' characters inside double-quoted strings so
    that e.g. 'default "#fff"' is not mistaken for a comment.
    """
    if "#" not in line:
        return -1

    in_quote = False
    escaped = False
    for i, ch in enumerate(line):
        if escaped:
            escaped = False
            continue
        if ch == "\\":
            escaped = True
        elif ch == '"':
            in_quote = not in_quote
        elif ch == "#" and not in_quote:
            return i
    return -1


def _iter_help_state(lines: list[str]) -> Iterator[tuple[int, str, bool]]:
    """
    Yield (index, line, in_help) for each line, where 'in_help' is True for
    lines that are part of a 'help' entry's body text.
    """
    in_help = False
    help_kw_cols = 0
    for i, line in enumerate(lines):
        if in_help:
            if line.strip() == "":
                # Blank lines stay inside the help block.
                yield i, line, True
                continue
            if _columns(_leading_ws(line)) > help_kw_cols:
                yield i, line, True
                continue
            # Indentation dropped back: the help block has ended.
            in_help = False

        if HELP_RE.match(line):
            in_help = True
            help_kw_cols = _columns(_leading_ws(line))
            yield i, line, False
            continue

        yield i, line, False


def _indent_issue(lines: list[str], i: int, line: str, in_help: bool) -> Issue | None:
    """
    Rule 2: check the indentation of a single line. Zephyr uses a flat layout:
    entries at column 0, properties at a single tab, and help text at one tab
    plus two extra spaces.
    """
    if not line.strip():
        return None

    lineno = i + 1
    if in_help:
        if not line.startswith(HELP_PREFIX) or "\t" in line[len(HELP_PREFIX) :].lstrip(" "):
            return Issue(
                lineno,
                1,
                "help-indent",
                "help text must be indented with one tab plus two extra spaces",
            )
        return None

    if line.startswith(" "):
        # Indentation must start with a tab.
        return Issue(lineno, 1, "tab-indent", "use tabs for indentation, not spaces")

    if _is_continuation(lines, i):
        # Continuation lines may use any number of tabs to align with the
        # statement they continue, but must not mix in spaces (a space after a
        # tab is injected verbatim into the joined line by kconfiglib).
        if " " in _leading_ws(line):
            return Issue(
                lineno, 1, "cont-indent", "use tabs only on continuation lines, not spaces"
            )
        return None

    content = line.lstrip("\t")
    tabs = len(line) - len(content)
    if BLOCK_RE.match(content):
        if tabs:
            return Issue(lineno, 1, "over-indent", "Kconfig entries must start at column 0")
    elif tabs > 1:
        return Issue(lineno, 1, "over-indent", "use a single tab to indent properties")
    return None


def lint(lines: list[str]) -> list[Issue]:
    """Return a list of Issue objects for the given list of lines."""
    issues = []
    help_body = set()

    for i, line, in_help in _iter_help_state(lines):
        lineno = i + 1
        if in_help:
            help_body.add(i)

        # Rule 1: line length. Lines containing a Kconfig macro expansion
        # '$(...)' are exempt: such calls cannot be wrapped, as kconfiglib
        # joins backslash continuations by raw concatenation and the
        # continuation's leading whitespace would be injected into the macro
        # arguments, corrupting them.
        cols = _columns(line)
        if cols > MAX_COLUMNS and "$(" not in line:
            issues.append(
                Issue(
                    lineno,
                    MAX_COLUMNS + 1,
                    "line-too-long",
                    f"line is {cols} columns, over the {MAX_COLUMNS} column limit",
                )
            )

        # Rule 2: indentation.
        if issue := _indent_issue(lines, i, line, in_help):
            issues.append(issue)

        # Rule 4: comment spacing (skip help body). A tab counts as separation
        # too, so commented-out code such as '#<tab>help' is not flagged.
        if not in_help:
            c = _comment_start(line)
            if c != -1:
                rest = line[c + 1 :]
                if rest and rest[0] not in (" ", "\t", "#", "\n"):
                    issues.append(
                        Issue(
                            lineno,
                            c + 1,
                            "comment-space",
                            "add a space after '#' in comments",
                        )
                    )

    issues.extend(_lint_blank_lines(lines, help_body))

    issues.sort(key=lambda issue: (issue.line, issue.col))
    return issues


def _is_entry(line: str) -> bool:
    return ENTRY_RE.match(line) is not None


def _if_endif_kw(line: str) -> str | None:
    """Return 'if'/'endif' if 'line' is such a column-0 statement, else None."""
    m = IF_ENDIF_RE.match(line)
    return m.group(1) if m else None


def _is_full_comment(line: str) -> bool:
    """True if 'line' is a comment line (only whitespace before the '#')."""
    c = _comment_start(line)
    return c != -1 and line[:c].strip() == ""


def _decl_unit_start(lines: list[str], i: int) -> int:
    """
    Return the first line index of the declaration "unit" ending at entry line
    'i', i.e. walk upwards over any comment lines attached directly above the
    entry (they document it and belong with it).
    """
    j = i
    while j > 0 and _is_full_comment(lines[j - 1]):
        j -= 1
    return j


def _blank_requirements(
    lines: list[str], help_body: set[int]
) -> tuple[dict[int, tuple[str, str]], dict[int, tuple[str, str]]]:
    """
    Return (before, after): maps of line index -> ('rule', 'message') for indices
    that must be preceded / followed by a blank line (rule 3 declarations and
    rule 5 top-level if/endif). Computed in a single pass over 'lines'.
    """
    n = len(lines)
    before: dict[int, tuple[str, str]] = {}
    after: dict[int, tuple[str, str]] = {}
    for i, line in enumerate(lines):
        kw = _if_endif_kw(line)
        if kw:
            # A comment block directly above an 'if' documents the conditional
            # block and belongs with it, so the required blank line goes before
            # the comment rather than between the comment and the 'if'.
            start = _decl_unit_start(lines, i) if kw == "if" else i
            if start > 0:
                before[start] = ("if-blank", f"add a blank line before '{kw}'")
            if i < n - 1:
                after[i] = ("if-blank", f"add a blank line after '{kw}'")
        elif i not in help_body and _is_entry(line):
            start = _decl_unit_start(lines, i)
            if start > 0 and not OPENERS_RE.match(lines[start - 1]):
                before.setdefault(start, ("decl-blank", "add a blank line before this declaration"))
    return before, after


def _lint_blank_lines(lines: list[str], help_body: set[int]) -> list[Issue]:
    """Issues for rules 3 (single blank between declarations) and 5 (if/endif)."""
    issues = []
    n = len(lines)

    def blank(idx: int) -> bool:
        return 0 <= idx < n and lines[idx].strip() == ""

    # Rule 3: collapse runs of blank lines. Blanks inside help text are not
    # declaration separators, so they are excluded via 'help_body'.
    consecutive = 0
    for i, line in enumerate(lines):
        if line.strip() == "" and i not in help_body:
            consecutive += 1
            if consecutive > 1:
                issues.append(Issue(i + 1, 1, "blank-lines", "remove consecutive blank lines"))
        else:
            consecutive = 0

    # Rules 3 and 5: required blank lines before/after.
    before, after = _blank_requirements(lines, help_body)
    for i, (rule, msg) in before.items():
        if not blank(i - 1):
            issues.append(Issue(i + 1, 1, rule, msg))
    for i, (rule, msg) in after.items():
        if not blank(i + 1):
            issues.append(Issue(i + 1, 1, rule, msg))

    return issues


def _final_newline_issues(raw: str) -> list[Issue]:
    """Rule 6: a file must end with exactly one newline."""
    if not raw:
        return []
    if not raw.endswith("\n"):
        last = raw.rsplit("\n", 1)[-1]
        return [
            Issue(
                raw.count("\n") + 1,
                len(last) + 1,
                "final-newline",
                "add a newline at the end of the file",
            )
        ]
    if raw.endswith("\n\n"):
        return [
            Issue(
                len(raw.rstrip("\n").split("\n")) + 1,
                1,
                "final-newline",
                "remove blank line(s) at the end of the file",
            )
        ]
    return []


def check_file(path: Path) -> list[Issue]:
    """Return the list of style issues for a single file."""
    raw = path.read_text(encoding="utf-8")
    lines = raw[:-1].split("\n") if raw.endswith("\n") else raw.split("\n")
    issues = lint(lines) + _final_newline_issues(raw)
    issues.sort(key=lambda issue: (issue.line, issue.col))
    return issues


def _is_kconfig_file(name: str) -> bool:
    return "Kconfig" in name


def expand_paths(paths: list[str]) -> list[Path]:
    """
    Expand the given paths into a de-duplicated list of files. Files are used
    as-is; directories are searched recursively for Kconfig files (sorted).
    """
    files = []
    for path in paths:
        p = Path(path)
        if p.is_dir():
            files.extend(
                sorted(f for f in p.rglob("*") if f.is_file() and _is_kconfig_file(f.name))
            )
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
        help="Kconfig files to check. Directories are searched recursively for Kconfig files.",
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
