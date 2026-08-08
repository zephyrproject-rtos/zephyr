# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for cmake_style.py.
"""

import pytest

# cmake_style needs tree-sitter; skip the whole module if it is not installed.
cmake_style = pytest.importorskip("cmake_style")


def _rules(text):
    """The set of rule ids reported for 'text'."""
    return {issue.rule for issue in cmake_style.check_text(text)}


# Rule 1: indentation (2 spaces per block level, no tabs).
def test_indent_wrong_depth_flagged():
    assert _rules("""\
if(A)
foo()
endif()
""") == {"indent"}


def test_indent_correct_depth_clean():
    assert not _rules("""\
if(A)
  foo()
endif()
""")


def test_indent_else_endif_use_outer_depth():
    assert not _rules("""\
if(A)
  a()
else()
  b()
endif()
""")


def test_tab_indent_flagged():
    assert _rules("""\
if(A)
\tfoo()
endif()
""") == {"tab-indent"}


# Rule 2: lowercase commands, with the mixed-case module exception.
def test_command_case_uppercase_builtin_flagged():
    assert _rules("FILE(GLOB x)\n") == {"command-case"}


def test_command_case_lowercase_clean():
    assert not _rules("file(GLOB x)\n")


def test_command_case_module_canonical_clean():
    assert not _rules("ExternalProject_Add(foo)\n")


def test_command_case_module_non_canonical_flagged():
    # A module command must use its canonical mixed case, not all-lowercase.
    assert _rules("externalproject_add(foo)\n") == {"command-case"}


# Rule 3: no space before the opening parenthesis.
def test_paren_space_flagged():
    assert _rules("""\
if (A)
endif()
""") == {"paren-space"}


def test_paren_space_clean():
    assert not _rules("""\
if(A)
endif()
""")


# Rule 4: cache/option variables use UPPERCASE names.
def test_cache_var_lowercase_flagged():
    assert _rules('set(foo 1 CACHE STRING "d")\n') == {"cache-var-case"}


def test_cache_var_uppercase_clean():
    assert not _rules('set(FOO 1 CACHE STRING "d")\n')


def test_cache_var_option_flagged():
    assert _rules('option(bar "desc" ON)\n') == {"cache-var-case"}


def test_cache_var_mixed_case_clean():
    # Mixed-case names (e.g. Python3_EXECUTABLE) are dictated by convention.
    assert not _rules('set(Foo_BAR 1 CACHE STRING "d")\n')


def test_cache_var_local_set_ignored():
    assert not _rules('set(output_dir "x")\n')


# Rule 5: booleans are not quoted in option()/set(... CACHE BOOL ...) values.
def test_quoted_bool_option_flagged():
    assert _rules('option(X "desc" "OFF")\n') == {"quoted-bool"}


def test_quoted_bool_cache_bool_flagged():
    assert _rules('set(X "ON" CACHE BOOL "d")\n') == {"quoted-bool"}


def test_quoted_bool_string_operand_clean():
    # A quoted boolean elsewhere is a legitimate string, not a boolean value.
    assert not _rules('string(REPLACE "ON" "off" out "${in}")\n')


def test_quoted_bool_cache_string_clean():
    assert not _rules('set(X "ON" CACHE STRING "d")\n')


def test_quoted_bool_plain_string_clean():
    assert not _rules('set(log_level "OFF")\n')


# Line length is intentionally not enforced for CMake.
def test_line_length_not_enforced():
    assert not _rules("set(x " + "a" * 200 + ")\n")


def test_clean_file_has_no_issues():
    assert not _rules("""\
if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()
""")
