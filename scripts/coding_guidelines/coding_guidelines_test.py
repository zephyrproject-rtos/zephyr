# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Unit tests for coding_guidelines.py.
"""

import pytest

# coding_guidelines needs tree-sitter; skip the whole module if it is not installed.
coding_guidelines = pytest.importorskip("coding_guidelines")

# A fixture reserved list, so the rule tests don't depend on the contents of the
# shipped reserved_identifiers.txt.
RESERVED = {"malloc", "free", "errno", "abort"}


def _issues(text, reserved=RESERVED):
    return coding_guidelines.check_text(text, reserved)


def _rules(text, reserved=RESERVED):
    """The list of (rule, line) pairs reported for 'text'."""
    return [(issue.rule, issue.line) for issue in _issues(text, reserved)]


# reserved-identifier (Rule 21.2)
def test_reserved_object_flagged():
    assert _rules("int free;\n") == [("reserved-identifier", 1)]


def test_reserved_function_flagged():
    assert _rules("void abort(int code) { }\n") == [("reserved-identifier", 1)]


def test_reserved_parameter_flagged():
    assert _rules("void f(int *malloc);\n") == [("reserved-identifier", 1)]


def test_reserved_pointer_object_with_initializer_flagged():
    assert _rules("void f(void)\n{\n\tchar *errno = NULL;\n}\n") == [("reserved-identifier", 3)]


def test_reserved_name_as_call_not_flagged():
    # The rule is about declarations; using a library function is fine.
    assert not _rules("void f(void)\n{\n\tfree(p);\n}\n")


def test_reserved_name_as_member_flagged():
    assert _rules("struct s {\n\tint errno;\n};\n") == [("reserved-identifier", 2)]


def test_reserved_list_is_configurable():
    assert not _rules("int free;\n", reserved=set())


def test_reserved_list_does_not_apply_to_libc_and_posix():
    from pathlib import Path

    for path in (
        "lib/libc/minimal/source/stdlib/malloc.c",
        "/w/zephyr/include/zephyr/posix/time.h",
    ):
        assert coding_guidelines.reserved_for(Path(path), RESERVED) == set()
    assert coding_guidelines.reserved_for(Path("drivers/gpio/gpio_libc.c"), RESERVED) == RESERVED


# tag-name (Rule 5.7)
def test_struct_tag_as_object_name_flagged():
    assert _rules("struct item item;\n") == [("tag-name", 1)]


def test_struct_tag_as_pointer_object_name_flagged():
    assert _rules("struct k_thread *k_thread = NULL;\n") == [("tag-name", 1)]


def test_union_and_enum_tag_as_object_name_flagged():
    assert _rules("union u u;\nenum color color;\n") == [("tag-name", 1), ("tag-name", 2)]


def test_struct_tag_as_parameter_name_flagged():
    assert _rules("void f(const struct device *device);\n") == [("tag-name", 1)]


def test_struct_tag_as_member_name_flagged():
    assert _rules("struct outer {\n\tstruct inner inner;\n};\n") == [("tag-name", 2)]


def test_distinct_object_name_clean():
    assert not _rules("struct device *dev;\nstruct item it = {0};\n")


def test_typedef_name_clean():
    # Only tags are covered; a typedef name is not a tag.
    assert not _rules("k_timeout_t k_timeout_t;\n")


# bool-literal (Rule 10.1)
def test_bool_init_with_literal_flagged():
    assert _rules("bool b = 0;\n") == [("bool-literal", 1)]


def test_static_bool_init_with_literal_flagged():
    assert _rules("void f(void)\n{\n\tstatic bool b = 1;\n}\n") == [("bool-literal", 3)]


def test_bool_assign_with_literal_flagged():
    assert _rules("void f(void)\n{\n\tbool b;\n\n\tb = 1;\n}\n") == [("bool-literal", 5)]


def test_bool_parameter_assign_with_literal_flagged():
    assert _rules("void f(bool b)\n{\n\tb = 0;\n}\n") == [("bool-literal", 3)]


def test_file_scope_bool_assign_with_literal_flagged():
    assert _rules("static bool b;\n\nvoid f(void)\n{\n\tb = 1;\n}\n") == [("bool-literal", 5)]


def test_bool_init_with_true_false_clean():
    assert not _rules("bool a = true;\nbool b = false;\n\nvoid f(void)\n{\n\ta = false;\n}\n")


def test_int_assign_with_literal_clean():
    assert not _rules("void f(void)\n{\n\tint i = 0;\n\n\ti = 1;\n}\n")


def test_bool_pointer_is_not_a_bool_object():
    assert not _rules("void f(bool *p)\n{\n\tbool *q = 0;\n\n\tp = 0;\n\tq = 0;\n}\n")


def test_inner_scope_shadowing_int_clean():
    assert not _rules("bool b;\n\nvoid f(void)\n{\n\tint b;\n\n\tb = 1;\n}\n")


def test_bool_assign_with_other_literal_clean():
    # Not a 0/1 literal, so not this rule's business.
    assert not _rules("void f(void)\n{\n\tbool b;\n\n\tb = 2;\n}\n")


def test_bool_compound_assign_clean():
    assert not _rules("void f(void)\n{\n\tbool b;\n\n\tb |= 1;\n}\n")


def test_bool_tracking_skips_conditionally_compiled_declarations():
    text = "#ifdef A\nint v;\n#else\nbool v;\n#endif\nvoid f(void)\n{\n\tv = 1;\n}\n"
    assert not _rules(text)


# unnamed-parameter (Rule 8.2)
def test_unnamed_parameter_in_declaration_flagged():
    assert _rules("int foo(int, char *name);\n") == [("unnamed-parameter", 1)]


def test_unnamed_pointer_parameter_in_definition_flagged():
    assert _rules("void baz(struct foo *, int n)\n{\n}\n") == [("unnamed-parameter", 1)]


def test_unnamed_typedef_parameter_flagged():
    assert _rules("int k_foo(struct k_sem *sem, k_timeout_t);\n") == [("unnamed-parameter", 1)]


def test_unnamed_function_pointer_parameter_flagged():
    assert _rules("void f(void (*)(int x));\n") == [("unnamed-parameter", 1)]


def test_unnamed_parameter_of_function_pointer_parameter_flagged():
    assert _rules("void f(void (*cb)(int));\n") == [("unnamed-parameter", 1)]


def test_each_unnamed_parameter_reported():
    assert _rules("void f(int, int);\n") == [("unnamed-parameter", 1), ("unnamed-parameter", 1)]


def test_unnamed_parameter_of_pointer_returning_prototype_flagged():
    assert _rules("void *alloc(int);\nchar **split(const char *, int n);\n") == [
        ("unnamed-parameter", 1),
        ("unnamed-parameter", 2),
    ]


def test_named_parameters_clean():
    assert not _rules(
        "void f(const struct device *dev, void (*cb)(int err), int (*arr)[4], size_t len);\n"
    )


def test_void_and_variadic_clean():
    assert not _rules("void f(void);\nint g(const char *fmt, ...);\n")


def test_macro_invocation_not_a_function():
    # File scope macro invocations parse as declarations whose "parameters"
    # are the macro arguments; macro names are upper case.
    assert not _rules("SYS_INIT(init, PRE_KERNEL_1, 0);\nK_SEM_DEFINE(sem, 0, 1);\n")


def test_attribute_macro_before_type_tolerated():
    # Unknown words before the return type leave the declarator intact.
    assert _rules("static ALWAYS_INLINE int fast(int);\n__syscall int k_foo(int);\n") == [
        ("unnamed-parameter", 1),
        ("unnamed-parameter", 2),
    ]


def test_attribute_macro_after_declarator_not_a_function():
    assert not _rules("uint8_t buf[16] __aligned(FEATURE_X) __attribute__((section(\".s\")));\n")


def test_macro_typed_declaration_skipped():
    # The macro parses as the type of a declaration; its arguments are not
    # parameters and the "declarator" is not an object.
    assert not _rules("static K_SEM_DEFINE(free, 0, 1);\nFEATURE_X foo(dev, free);\n")


def test_function_pointer_object_unnamed_parameter_flagged():
    assert _rules("static int (*hook)(int) = hook_default;\n") == [("unnamed-parameter", 1)]


def test_function_pointer_array_unnamed_parameter_flagged():
    assert _rules("static void (*const set_compare[2])(TIM_TypeDef *, uint32_t);\n") == [
        ("unnamed-parameter", 1),
        ("unnamed-parameter", 1),
    ]


# Error recovery
def test_unparsable_region_skipped():
    # Inside an ERROR node a statement can show up as a declaration; nothing
    # in there is reported, while the rest of the file still is.
    assert _rules(") free(p, q);\nint free;\n") == [("reserved-identifier", 2)]


def test_function_definition_in_unparsable_region_checked():
    # Unbalanced conditional compilation wraps everything that follows in an
    # ERROR node; a complete function definition in there is still reliable.
    text = (
        "#if A\nvoid f(void)\n{\n#else\nvoid f(int x)\n{\n#endif\n}\n"
        "void g(const struct device *device)\n{\n\tbool b = 0;\n}\n"
    )
    assert _rules(text) == [("tag-name", 9), ("bool-literal", 11)]


def test_keyword_in_function_position_skipped():
    # A misparsed '} else if (...) {' comes through as a definition of 'if'.
    assert not _rules("else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {\n}\n")


def test_statement_after_stray_directive_skipped():
    assert not _rules("FEATURE_X cleanup(dev, enabled);\n")


def test_unrecognized_macro_body_skipped():
    # A '#define' body the parser did not recognize comes through as code
    # split by '##' tokens; the line continuations give it away.
    text = (
        "\t{\t\t\t\t\\\n"
        "\t\tz_ ## __lname ## _head_set(list,\t\t\\\n"
        "\t\t\tz_ ## __nname ## _next_peek(node));\t\\\n"
        "\t}\n"
    )
    assert not _rules(text)


def test_macro_wrapped_parameters_skipped():
    assert not _rules("void cb(STM32_ETH_ARGS(ETH_HandleTypeDef *heth, void **start))\n{\n}\n")


def test_deeply_nested_initializer_does_not_overflow():
    text = "const int8_t model[] = {\n" + "\t0x01, 0x02, 0x03, 0x04,\n" * 20000 + "};\n"
    assert not _rules(text)


# Reporting
def test_issue_position_is_one_based():
    (issue,) = _issues("void f(void)\n{\n\tint free;\n}\n")
    assert (issue.line, issue.col) == (3, 6)


def test_issues_sorted_by_position():
    issues = _issues("void f(int);\nint free;\n")
    assert [(i.line, i.col) for i in issues] == [(1, 8), (2, 5)]


# include-guard (Dir 4.10)
def _header_rules(text):
    return [(i.rule, i.line) for i in coding_guidelines.check_text(text, RESERVED, header=True)]


def test_header_with_guard_clean():
    assert not _header_rules("/* c */\n#ifndef FOO_H\n#define FOO_H\nint x;\n#endif\n")


def test_header_with_pragma_once_clean():
    assert not _header_rules("#pragma once\nint x;\n")


def test_header_with_includes_before_guard_clean():
    assert not _header_rules("#include <stdint.h>\n#ifndef FOO_H\n#define FOO_H\nint x;\n#endif\n")


def test_header_with_negated_defined_guard_clean():
    assert not _header_rules("#if !defined(FOO_H)\n#define FOO_H\nint x;\n#endif\n")


def test_header_without_guard_flagged():
    assert _header_rules("/* c */\nint x;\n") == [("include-guard", 1)]


def test_header_guard_defining_other_macro_reported_at_guard():
    assert _header_rules("/* c */\n#ifndef FOO_H\n#define BAR\nint x;\n#endif\n") == [
        ("include-guard", 2)
    ]


def test_header_with_code_outside_guard_reported_at_that_code():
    assert _header_rules("#ifndef FOO_H\n#define FOO_H\n#endif\nint x;\n") == [("include-guard", 4)]


def test_source_file_needs_no_guard():
    assert not _rules("int x;\n")


# Integer constants (Rules 7.1, 7.2, 7.3, 1.2)
def test_octal_constant_flagged():
    assert _rules("int mode = 0600;\n") == [("octal-constant", 1)]


def test_zero_is_not_octal():
    assert not _rules("int a = 0;\nint b = 0U;\nint c = 0x0;\n")


def test_lowercase_l_suffix_flagged():
    assert _rules("unsigned long a = 0x01ul;\nlong b = 1l;\n") == [
        ("lowercase-l-suffix", 1),
        ("lowercase-l-suffix", 2),
    ]


def test_uppercase_suffixes_clean():
    assert not _rules("unsigned long a = 0x01UL;\nlong b = 1L;\nunsigned c = 2U;\n")


def test_hex_constant_with_unsigned_type_flagged():
    assert _rules("uint32_t m = 0xffffffff;\nuint32_t n = 0x80000000;\n") == [
        ("unsigned-suffix", 1),
        ("unsigned-suffix", 2),
    ]


def test_hex_constant_with_suffix_or_signed_type_clean():
    assert not _rules(
        "uint32_t m = 0xffffffffU;\nuint32_t n = 0x7fffffff;\nuint64_t o = 0x100000000;\n"
    )


def test_decimal_constant_never_needs_suffix():
    assert not _rules("int64_t n = 4294967295;\n")


def test_float_constant_ignored():
    assert not _rules("double d = 0.5;\nfloat f = 1e10f;\n")


def test_binary_literal_flagged():
    assert _rules("int a = 0b1010;\n") == [("language-extension", 1)]


def test_case_range_flagged():
    assert _rules(
        "void f(int n)\n{\n\tswitch (n) {\n\tcase 1 ... 3:\n\t\tbreak;\n"
        "\tdefault:\n\t\tbreak;\n\t}\n}\n"
    ) == [("language-extension", 4)]


def test_elvis_operator_flagged():
    assert _rules("void f(int a, int b)\n{\n\ta = b ?: 1;\n}\n") == [("language-extension", 3)]


def test_conditional_with_both_operands_clean():
    assert not _rules("void f(int a, int b)\n{\n\ta = b ? b : 1;\n}\n")


# bitfield-type (Rule 6.1)
def test_plain_int_bitfield_flagged():
    assert _rules("struct s {\n\tint x: 1;\n\tchar y: 2;\n};\n") == [
        ("bitfield-type", 2),
        ("bitfield-type", 3),
    ]


def test_explicit_bitfield_types_clean():
    assert not _rules(
        "struct s {\n\tunsigned int x: 1;\n\tsigned int y: 2;\n\tbool z: 1;\n\tuint8_t w: 3;\n};\n"
    )


# inline-static (Rule 8.10)
def test_inline_without_static_flagged():
    assert _rules("inline int g(void)\n{\n\treturn 0;\n}\n") == [("inline-static", 1)]


def test_always_inline_without_static_flagged():
    assert _rules("ALWAYS_INLINE int g(void)\n{\n\treturn 0;\n}\n") == [("inline-static", 1)]


def test_extern_inline_flagged():
    assert _rules("extern inline int g(void)\n{\n\treturn 0;\n}\n") == [("inline-static", 1)]


def test_static_inline_clean():
    assert not _rules(
        "static inline int g(void)\n{\n\treturn 0;\n}\n"
        "static ALWAYS_INLINE int h(void)\n{\n\treturn 0;\n}\n"
    )


# switch rules (16.4, 16.5, 16.6)
def _switch(body):
    return _rules("void f(int n)\n{\n\tswitch (n) {\n" + body + "\t}\n}\n")


def test_switch_without_default_flagged():
    assert _switch("\tcase 1:\n\t\tbreak;\n\tcase 2:\n\t\tbreak;\n") == [("switch-default", 3)]


def test_switch_default_in_the_middle_flagged():
    assert _switch("\tcase 1:\n\t\tbreak;\n\tdefault:\n\t\tbreak;\n\tcase 2:\n\t\tbreak;\n") == [
        ("default-position", 6)
    ]


def test_switch_default_first_or_last_clean():
    assert not _switch("\tdefault:\n\t\tbreak;\n\tcase 1:\n\t\tbreak;\n")
    assert not _switch("\tcase 1:\n\t\tbreak;\n\tdefault:\n\t\tbreak;\n")


def test_switch_with_one_clause_flagged():
    assert _switch("\tcase 1:\n\tdefault:\n\t\tbreak;\n") == [("switch-clauses", 3)]


def test_switch_grouped_labels_count_as_one_clause():
    # 'case 1: case 2:' share a body: one clause plus the default is two.
    assert not _switch("\tcase 1:\n\tcase 2:\n\t\tbreak;\n\tdefault:\n\t\tbreak;\n")


def test_switch_defaults_under_conditional_compilation_not_ordered():
    assert not _switch(
        "\tcase 1:\n\t\tbreak;\n#ifdef A\n\tdefault:\n\t\treturn 1;\n"
        "#else\n\tdefault:\n\t\treturn 2;\n#endif\n"
    )


def test_switch_in_macro_body_skipped():
    assert not _rules("#define SW(n) \\\n\tswitch (n) { \\\n\tcase 1: \\\n\t\tbreak; \\\n\t}\n")


# Function body rules (15.2, 2.6, 17.2, 21.7)
def test_backward_goto_flagged():
    assert _rules("void f(void)\n{\nagain:\n\tg();\n\tgoto again;\n}\n") == [("backward-goto", 5)]


def test_forward_goto_clean():
    assert not _rules("void f(void)\n{\n\tgoto out;\nout:\n\treturn;\n}\n")


def test_unused_label_flagged():
    assert _rules("void f(void)\n{\n\tg();\nout:\n\treturn;\n}\n") == [("unused-label", 4)]


def test_direct_recursion_flagged():
    assert _rules("int f(int n)\n{\n\treturn n ? f(n - 1) : 0;\n}\n") == [("recursion", 3)]


def test_call_to_other_function_clean():
    assert not _rules("int f(int n)\n{\n\treturn g(n);\n}\n")


def test_atoi_flagged():
    assert _rules("void f(const char *s)\n{\n\tint n = atoi(s);\n\n\tg(n);\n}\n") == [
        ("stdlib-ato", 3)
    ]


# pointer-nesting (18.5) and vla (18.8)
def test_three_pointer_levels_flagged():
    assert _rules("char ***p;\nvoid f(int ***q);\nstruct s {\n\tint ***r;\n};\n") == [
        ("pointer-nesting", 1),
        ("pointer-nesting", 2),
        ("pointer-nesting", 4),
    ]


def test_two_pointer_levels_clean():
    assert not _rules("char **argv;\nvoid f(int **q);\n")


def test_vla_flagged():
    assert _rules(
        "void f(size_t n, const struct cfg *c)\n{\n\tint a[n];\n\tint b[c->len + 1];\n}\n"
    ) == [
        ("vla", 3),
        ("vla", 4),
    ]


def test_array_sized_by_enumerator_or_offsetof_clean():
    assert not _rules(
        "enum { count = 4 };\nvoid f(void)\n{\n\tint a[count];\n"
        "\tchar b[offsetof(struct s, m)];\n}\n"
    )


def test_array_sized_by_file_scope_object_flagged():
    assert _rules("int n;\nvoid f(void)\n{\n\tint a[n];\n}\n") == [("vla", 4)]


def test_constant_size_arrays_clean():
    assert not _rules(
        "void f(void)\n{\n\tint a[4];\n\tint b[BUF_SIZE + 2];\n\tint c[ARRAY_SIZE(a)];\n"
        "\tint d[sizeof(int) * 2];\n\tstatic int e[NUM_ITEMS];\n}\n"
    )


def test_file_scope_array_is_not_vla():
    assert not _rules("int a[n];\n")


# braces (15.6)
def test_unbraced_bodies_flagged():
    text = (
        "void f(int n)\n{\n\tif (n)\n\t\tg();\n\telse\n\t\th();\n\twhile (n--)\n\t\tg();\n"
        "\tfor (;;)\n\t\tg();\n\tdo\n\t\tg();\n\twhile (n);\n}\n"
    )
    assert [r for r, _ in _rules(text)] == ["braces"] * 5


def test_braced_bodies_and_else_if_clean():
    text = (
        "void f(int n)\n{\n\tif (n) {\n\t\tg();\n\t} else if (n > 1) {\n\t\th();\n"
        "\t} else {\n\t\ti();\n\t}\n}\n"
    )
    assert not _rules(text)


# line-comment (Zephyr style)
def test_line_comment_flagged():
    assert _rules("// hello\nint x; // there\n") == [("line-comment", 1), ("line-comment", 2)]


def test_block_comment_clean():
    assert not _rules("/* hello */\nint x; /* there */\n")


# macro-parameter (20.7)
def test_unparenthesized_macro_parameter_flagged():
    assert _rules("#define BAD(a, b) (a + b)\n") == [("macro-parameter", 1)]


def test_parenthesized_macro_parameters_clean():
    assert not _rules("#define ADD(a, b) ((a) + (b))\n")


def test_macro_parameter_as_argument_or_index_clean():
    text = (
        "#define CALL(f, x) f((x))\n#define IDX(a, i) ((a)[i])\n"
        "#define ASSIGN(v, x) do { (v) = x; } while (0)\n"
    )
    assert not _rules(text)


def test_macro_parameter_as_member_designator_clean():
    assert not _rules("#define GET(s, m) ((s)->m)\n#define VAL(s, m) ((s).m)\n")


def test_macro_parameter_before_member_access_flagged():
    assert _rules("#define NEXT(x) x->next\n#define LEN(x) x.len\n") == [
        ("macro-parameter", 1),
        ("macro-parameter", 2),
    ]


def test_macro_parameter_reported_at_its_use_in_the_body():
    (issue,) = _issues("#define ADD(a, b) \\\n\t((a) + b)\n")
    assert (issue.rule, issue.line, issue.col) == ("macro-parameter", 2, 9)


def test_macro_parameter_after_line_splice_clean():
    assert not _rules("#define CALL(x) call( \\\n\tx)\n")


def test_macro_parameter_as_cast_type_clean():
    assert not _rules("#define SIZEOF_FIELD(type, member) sizeof(((type *)0)->member)\n")


def test_stringified_and_pasted_parameters_clean():
    assert not _rules("#define STR(x) #x\n#define CAT(x, y) x##y\n")


def test_macro_parameter_next_to_word_clean():
    assert not _rules("#define DEF(_name, ...) struct item _name[] = {__VA_ARGS__};\n")


def test_macro_parameter_before_arrow_flagged():
    assert _rules("#define DATA(dev) (*(struct data *)dev->data)\n") == [("macro-parameter", 1)]


def test_switch_clauses_under_conditional_compilation_counted():
    assert not _switch(
        "#ifdef A\n\tcase 1:\n\t\treturn 1;\n#endif\n#ifdef B\n\tcase 2:\n\t\treturn 2;\n#endif\n"
        "\tdefault:\n\t\tbreak;\n"
    )


def test_do_while_zero_tail_of_macro_not_a_loop_body():
    # An unrecognized macro body ends in '} while (0)' with no backslash on
    # that line; the next declaration is not its loop body.
    assert not _rules("\t} while (0)\nint x;\n")


# --fix
def _fix(text, rules=None):
    new_text, fixed, remaining = coding_guidelines.fix_text(text, RESERVED, rules=rules)
    return new_text, [i.rule for i in fixed], [i.rule for i in remaining]


def test_fix_constant_suffixes():
    text, fixed, remaining = _fix(
        "unsigned long a = 0x01ul;\nuint32_t b = 0xffffffff;\nlong c = 1l;\n"
    )
    assert text == "unsigned long a = 0x01UL;\nuint32_t b = 0xffffffffU;\nlong c = 1L;\n"
    assert sorted(fixed) == ["lowercase-l-suffix", "lowercase-l-suffix", "unsigned-suffix"]
    assert not remaining


def test_fix_leaves_line_comment_containing_block_terminator():
    text, fixed, remaining = _fix("// see */ here\nint x;\n")
    assert text == "// see */ here\nint x;\n"
    assert not fixed and remaining == ["line-comment"]


def test_fix_binary_literal_keeps_width_and_suffix():
    text, _, _ = _fix("int a = 0b1011;\nint b = 0b000100000000U;\n")
    assert text == "int a = 0xB;\nint b = 0x100U;\n"


def test_fix_bool_literals():
    text, _, _ = _fix("void f(void)\n{\n\tbool b = 0;\n\n\tb = 1;\n}\n")
    assert text == "void f(void)\n{\n\tbool b = false;\n\n\tb = true;\n}\n"


def test_fix_line_comments():
    text, _, _ = _fix("// top\n/// brief\nint x; ///< after\n//! also brief\n//\n")
    assert text == "/* top */\n/** brief */\nint x; /**< after */\n/** also brief */\n/* */\n"


def test_fix_braces_if_else_and_loops():
    text, _, remaining = _fix(
        "void f(int n)\n{\n\tif (n)\n\t\tg();\n\telse\n\t\th();\n\twhile (n--)\n\t\tg();\n"
        "\tdo\n\t\tg();\n\twhile (n);\n\tfor (;;)\n\t\tg();\n}\n"
    )
    assert text == (
        "void f(int n)\n{\n\tif (n) {\n\t\tg();\n\t} else {\n\t\th();\n\t}\n"
        "\twhile (n--) {\n\t\tg();\n\t}\n\tdo {\n\t\tg();\n\t} while (n);\n"
        "\tfor (;;) {\n\t\tg();\n\t}\n}\n"
    )
    assert not remaining


def test_fix_braces_same_line_body_and_nesting():
    text, _, remaining = _fix("void f(int n)\n{\n\tif (n) if (n > 1) g();\n}\n")
    assert text == "void f(int n)\n{\n\tif (n) {\n\t\tif (n > 1) {\n\t\t\tg();\n\t\t}\n\t}\n}\n"
    assert not remaining


def test_fix_braces_else_if_chain_kept():
    text, _, _ = _fix("void f(int n)\n{\n\tif (n)\n\t\tg();\n\telse if (n > 1)\n\t\th();\n}\n")
    assert (
        text == "void f(int n)\n{\n\tif (n) {\n\t\tg();\n\t} else if (n > 1) {\n\t\th();\n\t}\n}\n"
    )


def test_fix_is_idempotent():
    text = "void f(int n)\n{\n\tif (n)\n\t\tg();\n}\n"
    once, _, _ = _fix(text)
    twice, fixed, _ = _fix(once)
    assert twice == once and not fixed


def test_fix_leaves_other_rules_alone():
    text, fixed, remaining = _fix("int f(int);\nint x = 0600;\n")
    assert text == "int f(int);\nint x = 0600;\n"
    assert not fixed
    assert sorted(remaining) == ["octal-constant", "unnamed-parameter"]


def test_fix_rule_selection():
    text, fixed, remaining = _fix(
        "long a = 1l;\nuint32_t b = 0xffffffff;\n", rules={"lowercase-l-suffix"}
    )
    assert text == "long a = 1L;\nuint32_t b = 0xffffffff;\n"
    assert fixed == ["lowercase-l-suffix"] and remaining == ["unsigned-suffix"]
