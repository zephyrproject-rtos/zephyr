# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for kconfig_style.py.
"""

import kconfig_style


def _rules(text):
    """The set of rule ids reported for 'text' (line-based rules)."""
    lines = text[:-1].split("\n") if text.endswith("\n") else text.split("\n")
    return {issue.rule for issue in kconfig_style.lint(lines)}


def _file_rules(tmp_path, text):
    """The set of rule ids reported for 'text' written to a real file."""
    path = tmp_path / "Kconfig"
    path.write_text(text)
    return {issue.rule for issue in kconfig_style.check_file(path)}


# Rule 1: line length, with the $(...) macro exemption.
def test_line_too_long_flagged():
    assert _rules("config " + "A" * 110 + "\n") == {"line-too-long"}


def test_line_too_long_macro_exempt():
    assert not _rules('config X\n\tdefault "$(' + "y" * 110 + ')"\n')


# Rule 2: indentation (tabs, flat layout, help body, continuations).
def test_space_indentation_flagged():
    assert _rules("""\
config X
    default y
""") == {"tab-indent"}


def test_property_single_tab_clean():
    assert not _rules("""\
config X
\tdefault y
""")


def test_property_over_indent_flagged():
    assert _rules("""\
config X
\t\tdefault y
""") == {"over-indent"}


def test_help_body_indent_clean():
    assert not _rules("""\
config X
\thelp
\t  Help text.
""")


def test_help_body_indent_flagged():
    # Help body must be one tab plus two spaces; two tabs is wrong.
    assert _rules("""\
config X
\thelp
\t\tHelp text.
""") == {"help-indent"}


def test_continuation_tabs_clean():
    assert not _rules("""\
config X
\tdepends on A \\
\t\t|| B
""")


def test_continuation_spaces_flagged():
    assert _rules("""\
config X
\tdepends on A \\
\t  || B
""") == {"cont-indent"}


# Rule 3: single blank line between declarations, no consecutive blanks.
def test_consecutive_blank_lines_flagged():
    assert _rules("""\
config A
\tbool


config B
\tbool
""") == {"blank-lines"}


def test_decl_blank_missing_flagged():
    assert _rules("""\
config A
\tbool
config B
\tbool
""") == {"decl-blank"}


def test_decl_blank_present_clean():
    assert not _rules("""\
config A
\tbool

config B
\tbool
""")


def test_decl_after_multiline_opener_no_decl_blank():
    # A declaration right after a backslash-continued opener needs no decl-blank;
    # the 'if' still legitimately needs a blank line after it (if-blank).
    assert _rules("""\
if B \\
\t|| C
config D
\tbool

endif
""") == {"if-blank"}


# Rule 4: comment spacing.
def test_comment_space_flagged():
    assert _rules("#comment\n") == {"comment-space"}


def test_comment_space_clean():
    assert not _rules("# comment\n")


# Rule 5: blank lines around top-level if/endif, incl. multi-line conditions.
def test_if_blank_missing_flagged():
    assert _rules("""\
config A
\tbool

if B
config C
\tbool

endif
""") == {"if-blank"}


def test_if_blank_multiline_condition_clean():
    assert not _rules("""\
config A
\tbool

if B \\
\t|| C

config D
\tbool

endif
""")


# Rule 6: exactly one trailing newline (whole-file rule).
def test_final_newline_missing_flagged(tmp_path):
    assert _file_rules(tmp_path, "config A\n\tbool") == {"final-newline"}


def test_final_newline_extra_flagged(tmp_path):
    assert _file_rules(tmp_path, "config A\n\tbool\n\n") == {"final-newline"}


def test_final_newline_single_clean(tmp_path):
    assert not _file_rules(tmp_path, "config A\n\tbool\n")
