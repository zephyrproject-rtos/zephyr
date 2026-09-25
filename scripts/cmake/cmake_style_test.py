# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for cmake_style.py.
"""

import pytest

# cmake_style needs tree-sitter; skip the whole module if it is not installed.
cmake_style = pytest.importorskip("cmake_style")

# A fixture allow-list, so the rule tests don't depend on the contents of the
# shipped cmake_style_mixed_case.txt.
MIXED_CASE = {"externalproject_add": "ExternalProject_Add"}


def _rules(text, mixed_case=MIXED_CASE):
    """The set of rule ids reported for 'text'."""
    return {issue.rule for issue in cmake_style.check_text(text, mixed_case)}


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


def test_command_case_unlisted_mixed_case_flagged():
    # A mixed-case command not in the allow-list is held to the lowercase rule.
    assert _rules("UpdateableImage_Get(foo)\n") == {"command-case"}


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


# The mixed-case allow-list loader.
def test_load_mixed_case_appends_across_files(tmp_path):
    extra = tmp_path / "extra.txt"
    extra.write_text("""\
# downstream extension
UpdateableImage_Get  # trailing comment

""")
    mixed_case = cmake_style.load_mixed_case([cmake_style.DEFAULT_MIXED_CASE_FILE, extra])
    assert mixed_case["externalproject_add"] == "ExternalProject_Add"
    assert mixed_case["updateableimage_get"] == "UpdateableImage_Get"


def test_load_mixed_case_rejects_lowercase_entry(tmp_path):
    f = tmp_path / "bad.txt"
    f.write_text("all_lowercase\n")
    with pytest.raises(ValueError, match="all-lowercase"):
        cmake_style.load_mixed_case([f])


def test_load_mixed_case_rejects_invalid_name(tmp_path):
    f = tmp_path / "bad.txt"
    f.write_text("Not a command\n")
    with pytest.raises(ValueError, match="not a valid command name"):
        cmake_style.load_mixed_case([f])


def test_load_mixed_case_rejects_conflicting_case(tmp_path):
    f = tmp_path / "bad.txt"
    f.write_text("Foo_Bar\nFOO_Bar\n")
    with pytest.raises(ValueError, match="conflicts"):
        cmake_style.load_mixed_case([f])


def test_load_mixed_case_missing_file_raises(tmp_path):
    with pytest.raises(OSError):
        cmake_style.load_mixed_case([tmp_path / "nonexistent.txt"])


def test_default_mixed_case_file_loads():
    # Guards the format of the shipped allow-list.
    assert cmake_style.load_mixed_case([cmake_style.DEFAULT_MIXED_CASE_FILE])
