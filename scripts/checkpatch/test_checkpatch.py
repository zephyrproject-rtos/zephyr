# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Regression tests for C++ template angle brackets in checkpatch."""

import re
import subprocess
from collections import Counter
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
CHECKPATCH = ZEPHYR_BASE / "scripts" / "checkpatch.pl"
TEST_FILE = "include/zephyr/checkpatch_test.h"
DIAGNOSTIC_RE = re.compile(
    r"^.*?\b(?:ERROR|WARNING|CHECK):(?P<type>[A-Z0-9_]+):(?P<msg>[^\n]*)\n"
    r"#\d+: FILE: [^:\n]+:(?P<line>\d+):",
    re.MULTILINE,
)
OPERATOR_RE = re.compile(r"that '([^']+)'")
ANGLE_OPS = frozenset("<>")


def checkpatch_diagnostics(patch):
    """Return (kind, line, operator-or-None) tuples from checkpatch."""
    result = subprocess.run(
        [CHECKPATCH, "--no-tree", "--show-types", "-"],
        input=patch,
        text=True,
        capture_output=True,
        check=False,
        cwd=ZEPHYR_BASE,
    )
    # Without the summary line the run failed, and every "no diagnostics"
    # assertion below would pass without checking anything.
    assert "lines checked" in result.stdout, result.stderr
    diagnostics = []
    for match in DIAGNOSTIC_RE.finditer(result.stdout):
        operator = OPERATOR_RE.search(match.group("msg"))
        diagnostics.append(
            (
                match.group("type"),
                int(match.group("line")),
                operator.group(1) if operator else None,
            )
        )
    return diagnostics


def run_checkpatch(lines, context_lines=None, test_file=TEST_FILE):
    """Run checkpatch on added lines, optionally preceded by context lines."""
    context_lines = context_lines or []
    patch_lines = [f" {line}" for line in context_lines]
    patch_lines.extend(f"+{line}" for line in lines)
    old_count = len(context_lines)
    new_count = len(patch_lines)
    patch = (
        f"diff --git a/{test_file} b/{test_file}\n"
        f"--- a/{test_file}\n"
        f"+++ b/{test_file}\n"
        f"@@ -1,{old_count} +1,{new_count} @@\n" + "\n".join(patch_lines) + "\n"
    )

    return checkpatch_diagnostics(patch)


def angle_counts(diagnostics):
    """Count SPACING on '<' / '>' and CONSTANT_COMPARISON, ignoring other operators."""
    counts = Counter()
    for kind, line, operator in diagnostics:
        if kind == "CONSTANT_COMPARISON":
            counts[(kind, line, None)] += 1
        elif kind == "SPACING" and operator in ANGLE_OPS:
            counts[(kind, line, operator)] += 1
    return counts


def assert_no_angle_diagnostics(diagnostics):
    assert angle_counts(diagnostics) == Counter()


@pytest.mark.parametrize(
    "lines",
    [
        pytest.param(["template<typename T> struct value {};"], id="tight-template"),
        pytest.param(["template <typename T> struct value {};"], id="spaced-template"),
        pytest.param(["template <> struct value<int> {};"], id="explicit-specialization"),
        pytest.param(["template <typename T = int> struct value {};"], id="default-type"),
        pytest.param(["template <int N = -1> struct value {};"], id="default-negative"),
        pytest.param(
            ["template <bool E = (N > 0)> struct value {};"],
            id="parenthesized-comparison",
        ),
        pytest.param(
            ["template <bool E = ((N) > (0))> struct value {};"],
            id="nested-parenthesized-comparison",
        ),
        pytest.param(
            ["template <bool E = a < b> struct value {};"],
            id="spaced-less-does-not-nest",
        ),
        pytest.param(["std::vector<int> value;"], id="qualified-name"),
        pytest.param(
            ["std::vector<std::pair<int, int>> value;"],
            id="nested-closing-angles",
        ),
        pytest.param(
            ["std::vector<std::vector<std::pair<int, int>>> value;"],
            id="triple-closing-angles",
        ),
        pytest.param(
            ["auto value = static_cast<uint8_t *>(ptr);"],
            id="static-cast",
        ),
        pytest.param(
            ["auto value = const_cast<int *>(ptr);"],
            id="const-cast",
        ),
        pytest.param(
            ["auto value = reinterpret_cast<uint8_t *>(ptr);"],
            id="reinterpret-cast",
        ),
        pytest.param(
            ["auto value = dynamic_cast<Derived *>(ptr);"],
            id="dynamic-cast",
        ),
        pytest.param(["typename value_type<T>::type value;"], id="dependent-name"),
        pytest.param(
            ["using type = typename T::template rebind<U>::other;"],
            id="dependent-template-keyword",
        ),
        pytest.param(
            ["std::array<int, value->size> array;"],
            id="member-access-inside-list",
        ),
        pytest.param(
            ["std::integral_constant<int, arr[i > j]> value;"],
            id="subscript-inside-list",
        ),
        pytest.param(
            ["std::function<void(const char *, int &)> value;"],
            id="unary-pointer-and-reference",
        ),
        pytest.param(
            ["std::array<int, (N << 2)> value;"],
            id="shift-inside-parentheses",
        ),
        pytest.param(
            ["std::array<uint8_t, MAX_LEN> buffer;"],
            id="constant-as-template-argument",
        ),
        pytest.param(
            ["template <template <typename> class T> struct value {};"],
            id="template-template-parameter",
        ),
        pytest.param(
            ["template <typename... Ts> struct value {};"],
            id="parameter-pack",
        ),
        pytest.param(
            ["template <typename T> void func(std::vector<T> &&value);"],
            id="rvalue-reference",
        ),
        pytest.param(
            ["template <typename T, typename = std::enable_if_t<A && B>> struct X {};"],
            id="logical-and-inside-list",
        ),
        pytest.param(
            [
                "template <typename... Ts> constexpr size_t n ="
                " std::integral_constant<size_t, sizeof...(Ts)>::value;"
            ],
            id="sizeof-pack-inside-list",
        ),
        pytest.param(
            ["template <typename T> using V = std::vector<T>;"],
            id="alias-template",
        ),
        pytest.param(
            ["std::ostream &operator<<(std::ostream &os, const Foo &f);"],
            id="operator-shift",
        ),
        pytest.param(
            ["#include <zephyr/kernel.h>"],
            id="include-directive",
        ),
        pytest.param(
            ['const char *s = "std::vector<int>";'],
            id="angles-inside-string",
        ),
        pytest.param(
            ["template <int N> constexpr int value = N << 2;"],
            id="shift-after-closed-list",
        ),
        pytest.param(
            ["int value = static_cast<int>(left) < right;"],
            id="cast-then-spaced-comparison",
        ),
        pytest.param(
            ["#define VALUE std::array<int, 4>"],
            id="template-in-define",
        ),
        pytest.param(
            ["export template <typename T> struct value {};"],
            id="export-template",
        ),
        pytest.param(
            ["template <typename T> template <typename U> void X<T>::f();"],
            id="member-template-after-close",
        ),
        pytest.param(
            ["namespace ns { template <typename T> struct X {}; }"],
            id="template-after-brace",
        ),
        pytest.param(
            [
                "template <typename T,",
                "          typename U>",
                "struct value {};",
            ],
            id="multiline-declaration",
        ),
        pytest.param(
            [
                "template <bool E =",
                "          (N > 0)>",
                "struct value {};",
            ],
            id="multiline-parenthesized-comparison",
        ),
        pytest.param(
            [
                "std::vector<",
                "    std::pair<int, int>> value;",
            ],
            id="multiline-qualified-name",
        ),
        pytest.param(
            ["template <typename T", ">", "struct value {};"],
            id="closing-angle-alone-on-line",
        ),
        pytest.param(
            [
                "template <bool E = (N >",
                "                    0)>",
                "struct value {};",
            ],
            id="parentheses-span-lines",
        ),
        pytest.param(
            [
                "template <typename T,",
                "#if OPTION > 0",
                "          typename U = int,",
                "#endif",
                "          typename V>",
                "struct value {};",
            ],
            id="preprocessor-if-inside-list",
        ),
        pytest.param(
            [
                "template <typename T,",
                "#define OPTION 1",
                "\ttypename U>",
                "struct value {};",
            ],
            id="preprocessor-define-inside-list",
        ),
        pytest.param(
            [
                "template <typename T,",
                "\ttypename U> struct X : Base<T> {",
            ],
            id="another-list-after-multiline-close",
        ),
    ],
)
def test_cxx_template_brackets_are_not_operators(lines):
    assert_no_angle_diagnostics(run_checkpatch(lines))


def test_template_opener_in_context_line():
    diagnostics = run_checkpatch(
        ["          typename U>", "struct value {};"],
        context_lines=["template <typename T,"],
    )

    assert_no_angle_diagnostics(diagnostics)


def test_template_state_is_reset_between_hunks():
    patch = (
        f"diff --git a/{TEST_FILE} b/{TEST_FILE}\n"
        f"--- a/{TEST_FILE}\n"
        f"+++ b/{TEST_FILE}\n"
        "@@ -0,0 +1 @@\n"
        "+template <typename T,\n"
        "@@ -99,0 +100 @@\n"
        "+int value = left>right;\n"
    )

    assert angle_counts(checkpatch_diagnostics(patch)) == Counter({("SPACING", 100, ">"): 1})


def test_removed_template_opener_does_not_change_state():
    patch = (
        f"diff --git a/{TEST_FILE} b/{TEST_FILE}\n"
        f"--- a/{TEST_FILE}\n"
        f"+++ b/{TEST_FILE}\n"
        "@@ -1 +1 @@\n"
        "-template <typename T,\n"
        "+int value = left>right;\n"
    )

    assert angle_counts(checkpatch_diagnostics(patch)) == Counter({("SPACING", 1, ">"): 1})


def test_template_identifier_in_c_file_is_not_treated_as_cxx():
    diagnostics = run_checkpatch(
        ["template < limit", "    && left>right;"],
        test_file="src/checkpatch_test.c",
    )

    assert angle_counts(diagnostics) == Counter({("SPACING", 2, ">"): 1})


@pytest.mark.parametrize(
    ("lines", "expected"),
    [
        pytest.param(
            ["if (left<right) {", "}"],
            {("SPACING", 1, "<"): 1},
            id="tight-comparison",
        ),
        pytest.param(
            ["if (MAX<value> limit) {", "}"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1, ("CONSTANT_COMPARISON", 1, None): 1},
            id="constant-on-left",
        ),
        pytest.param(
            ["if (left<right, upper> lower) {", "}"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="comma-separated-comparisons",
        ),
        pytest.param(
            ["if (left<(middle)> right) {", "}"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="parenthesized-operand",
        ),
        pytest.param(
            ["for (i=0; i<n; i++) {", "}"],
            {("SPACING", 1, "<"): 1},
            id="for-loop",
        ),
        pytest.param(
            ["int x = a<b ? a : b;"],
            {("SPACING", 1, "<"): 1},
            id="ternary",
        ),
        pytest.param(
            ['std::cout << "value";', "int value = left>right;"],
            {("SPACING", 2, ">"): 1},
            id="shift-does-not-leak",
        ),
        pytest.param(
            ["bool value = ns::field <= 3;", "int other = left>right;"],
            {("SPACING", 2, ">"): 1},
            id="less-equal-does-not-leak",
        ),
        pytest.param(
            ["if (template < limit", "    && left>right) {", "}"],
            {("SPACING", 2, ">"): 1},
            id="template-c-identifier",
        ),
        pytest.param(
            ["if (ns::left<right>limit) {", "}"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="word-after-closing-angle",
        ),
        pytest.param(
            ["bool v = static_cast<int>(a)>b;"],
            {("SPACING", 1, ">"): 1},
            id="cast-then-tight-comparison",
        ),
        pytest.param(
            [
                "int value = arr[std::max<int>(left, right) + a<b];",
                "int other = left>right;",
            ],
            {("SPACING", 1, "<"): 1, ("SPACING", 2, ">"): 1},
            id="unmatched-bracket-aborts-nested-list",
        ),
        pytest.param(
            ["bool value = ns::field<", "\tlimit;", "int other = left>right;"],
            {("SPACING", 3, ">"): 1},
            id="continuation-abort-does-not-leak",
        ),
        pytest.param(
            [
                "int value = f(std::max<int>(left, right) + g(a<b));",
                "int other = left>right;",
            ],
            {("SPACING", 1, "<"): 1, ("SPACING", 2, ">"): 1},
            id="unmatched-paren-aborts",
        ),
    ],
)
def test_comparison_operators_are_still_checked(lines, expected):
    assert angle_counts(run_checkpatch(lines)) == Counter(expected)


@pytest.mark.parametrize(
    ("lines", "expected"),
    [
        pytest.param(
            ["template <typename T> bool less(T left, T right) { return left<right; }"],
            {("SPACING", 1, "<"): 1},
            id="comparison-in-one-line-body",
        ),
        pytest.param(
            ["template <typename T> struct value {}; int result = left<right;"],
            {("SPACING", 1, "<"): 1},
            id="comparison-after-semicolon",
        ),
        pytest.param(
            ["template <> struct value<int> { static constexpr bool v = a<b; };"],
            {("SPACING", 1, "<"): 1},
            id="comparison-in-specialization-body",
        ),
        pytest.param(
            ["int value = func(std::max<int>(left, right) + func(a<b));"],
            {("SPACING", 1, "<"): 1},
            id="comparison-after-qualified-template",
        ),
        pytest.param(
            ["bool value = Type::operator<(const Type &other) const;"],
            {("SPACING", 1, "<"): 1},
            id="operator-less",
        ),
        pytest.param(
            [
                "template <typename T> static inline constexpr int foo(T x) { return -42; }",
                "bool value = (a<b) && (c>d);",
            ],
            {("SPACING", 2, "<"): 1, ("SPACING", 2, ">"): 1},
            id="issue-54023-example",
        ),
    ],
)
def test_mixed_line_reports_only_the_comparison(lines, expected):
    assert angle_counts(run_checkpatch(lines)) == Counter(expected)


@pytest.mark.parametrize(
    ("lines", "expected"),
    [
        pytest.param(
            ["std::vector <int> value;"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="space-before-qualified-angle",
        ),
        pytest.param(
            ["Foo<int> value;"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="unqualified-template-id",
        ),
        pytest.param(
            ["template <bool E = N > 0> struct value {};"],
            {("SPACING", 1, ">"): 1, ("CONSTANT_COMPARISON", 1, None): 1},
            id="unparenthesized-greater",
        ),
        pytest.param(
            ["template <bool E = a<b> struct value {};"],
            {("SPACING", 1, "<"): 2, ("SPACING", 1, ">"): 1},
            id="unparenthesized-tight-less-looks-nested",
        ),
        pytest.param(
            ["template <auto V = Foo{}> struct value {};"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="brace-in-nttp-aborts",
        ),
        pytest.param(
            ["template <typename T,", "int value = left>right;"],
            {},
            id="open-list-swallows-rest-of-hunk",
        ),
        pytest.param(
            ["template <int N = 1 << 2> struct value {};"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="unparenthesized-shift-aborts",
        ),
        pytest.param(
            ["template <bool E = N >= 0> struct value {};"],
            {("SPACING", 1, "<"): 1, ("SPACING", 1, ">"): 1},
            id="unparenthesized-greater-equal-aborts",
        ),
        pytest.param(
            ["template < limit", "    && left>right;"],
            {},
            id="template-c-identifier-at-declaration-boundary",
        ),
    ],
)
def test_known_limitations_are_unchanged(lines, expected):
    assert angle_counts(run_checkpatch(lines)) == Counter(expected)
