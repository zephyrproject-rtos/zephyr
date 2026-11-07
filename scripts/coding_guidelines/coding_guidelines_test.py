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
