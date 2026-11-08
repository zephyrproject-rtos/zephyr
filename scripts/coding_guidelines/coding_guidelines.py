#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Coding guideline checker for Zephyr C sources (*.c and *.h).

Checks the subset of the "Coding Guidelines" (doc/contribute/coding_guidelines/)
that can be decided from the syntax of a single, unpreprocessed file:

  reserved-identifier  MISRA C:2012 Rule 21.2. A declared object, function or
                       parameter name is a reserved identifier, i.e. one listed
                       in reserved_identifiers.txt next to this script (the C
                       standard library names). The C library and the POSIX
                       layer (RESERVED_EXEMPT_DIRS) are exempt.
  tag-name             MISRA C:2012 Rule 5.7. An object, parameter or member is
                       named after the tag of its own struct, union or enum type
                       ('struct device *device').
  bool-literal         MISRA C:2012 Rule 10.1 (Zephyr guideline on boolean
                       essential type). A bool object is initialized or assigned
                       with the integer literal 0 or 1 instead of false or true.
  unnamed-parameter    MISRA C:2012 Rule 8.2. A function declaration or
                       definition has a parameter with a type but no name,
                       including the parameters of a function pointer parameter.
  include-guard        MISRA C:2012 Dir 4.10. A header is not wrapped in an
                       '#ifndef X / #define X ... #endif' include guard.
  language-extension   MISRA C:2012 Rule 1.2. A binary literal, a case range
                       ('case 1 ... 3:') or a conditional with an omitted
                       operand ('a ?: b').
  bitfield-type        MISRA C:2012 Rule 6.1. A bit-field of plain int, char,
                       short or long, whose signedness is implementation-defined.
  octal-constant       MISRA C:2012 Rule 7.1. An octal integer constant.
  unsigned-suffix      MISRA C:2012 Rule 7.2. A hexadecimal or octal constant
                       that has an unsigned type without a u or U suffix.
  lowercase-l-suffix   MISRA C:2012 Rule 7.3. A lowercase l in a literal suffix.
  inline-static        MISRA C:2012 Rule 8.10. An inline function definition
                       without the static storage class.
  switch-default       MISRA C:2012 Rule 16.4. A switch without a default label.
  default-position     MISRA C:2012 Rule 16.5. A default label that is neither
                       the first nor the last label of its switch.
  switch-clauses       MISRA C:2012 Rule 16.6. A switch with fewer than two
                       clauses.
  backward-goto        MISRA C:2012 Rule 15.2. A goto to a label declared
                       earlier in the function.
  unused-label         MISRA C:2012 Rule 2.6. A label no goto refers to.
  recursion            MISRA C:2012 Rule 17.2. A function calling itself.
  pointer-nesting      MISRA C:2012 Rule 18.5. A declarator with more than two
                       levels of pointer nesting.
  vla                  MISRA C:2012 Rule 18.8. A block scope array whose size
                       is not a constant expression.
  stdlib-ato           MISRA C:2012 Rule 21.7. A call to atof, atoi, atol or
                       atoll, which have undefined behavior on invalid input.
  braces               MISRA C:2012 Rule 15.6. The body of an if, else, for,
                       while or do statement is not a compound statement.
  macro-parameter      MISRA C:2012 Rule 20.7. A function-like macro uses a
                       parameter without parentheses next to an operator.
  line-comment         Zephyr coding style (doc/contribute/style/code.rst). A
                       // comment.

The last three overlap with checkpatch, which reports them on the lines of a
patch; this checker reports them on the tree as it is.

The rules are derived from the tree-sitter-c syntax tree. tree-sitter parses C
without preprocessing and recovers from the constructs it cannot parse (macro
invocations at file scope, conditionally compiled fragments), so the checks run
on the source as written, independently of any build configuration, and only
the constructs they understand are reported on.

With --fix, the issues whose correction is purely a matter of spelling are
rewritten in place (bool-literal, braces, line-comment, lowercase-l-suffix,
unsigned-suffix and the binary literals of language-extension) and the rest
are reported.

Issues are printed as '<file>:<line>:<col>: [<rule>] <message>'. The exit status
distinguishes "found issues" from "the checker failed":

  0 (EXIT_OK)     - no issues found
  1 (EXIT_ISSUES) - issues were found and printed to stdout
  2 (EXIT_ERROR)  - the checker itself failed (e.g. a file could not be read);
                    a message is printed to stderr

Requires: pip install tree-sitter tree-sitter-c
"""

import argparse
import re
import sys
import traceback
from dataclasses import dataclass
from pathlib import Path

import tree_sitter_c
from tree_sitter import Language, Node, Parser

# Default reserved identifier list (rule reserved-identifier), shipped next to
# this script.
DEFAULT_RESERVED_FILE = Path(__file__).parent / "reserved_identifiers.txt"

# Directories whose files define the reserved identifiers themselves: the C
# library implementation and the POSIX layer. The reserved-identifier rule does
# not apply to them. Matched against the path as given, so a path from the
# tree root and an absolute path both work.
RESERVED_EXEMPT_DIRS = (
    "lib/libc/",
    "subsys/portability/posix/",
    "include/zephyr/posix/",
    "tests/subsys/portability/",
)

EXIT_OK = 0
EXIT_ISSUES = 1
EXIT_ERROR = 2

_PARSER = Parser(Language(tree_sitter_c.language()))

# Type specifier node types whose tag is checked by the tag-name rule.
_TAGGED_SPECIFIERS = ("struct_specifier", "union_specifier", "enum_specifier")

# Nodes that scope an identifier: the names declared in them are not visible
# outside, so the bool-literal rule tracks bool objects per such node (on top
# of the file scope).
_SCOPES = ("compound_statement",)


@dataclass
class Edit:
    """Replace the bytes in [start, end) of the file with 'text'."""

    start: int
    end: int
    text: bytes


@dataclass
class Issue:
    """A single guideline violation, used for reporting."""

    line: int  # 1-based
    col: int  # 1-based
    rule: str
    message: str
    # Edits that resolve the issue without changing what the code does, for
    # the rules whose fix is mechanical (--fix); None for the others.
    fix: list[Edit] | None = None


def load_reserved(paths: list[Path]) -> set[str]:
    """
    Load the reserved identifier list files at 'paths'. Each file holds one
    identifier per line; blank lines and '#' comments are ignored. Raises
    OSError for an unreadable file.
    """
    reserved: set[str] = set()
    for path in paths:
        for line in path.read_text(encoding="utf-8").splitlines():
            name = line.split("#", 1)[0].strip()
            if name:
                reserved.add(name)
    return reserved


def _text(node: Node) -> str:
    """Decoded source text of a node ('' if the node carries no text)."""
    return node.text.decode("utf-8", errors="replace") if node.text is not None else ""


def _issue(node: Node, rule: str, message: str, fix: list[Edit] | None = None) -> Issue:
    return Issue(node.start_point.row + 1, node.start_point.column + 1, rule, message, fix)


def _replace(node: Node, text: str) -> list[Edit]:
    return [Edit(node.start_byte, node.end_byte, text.encode("utf-8"))]


def _declared_name(declarator: Node | None) -> Node | None:
    """
    The identifier node a declarator declares, looking through pointer, array,
    function, parenthesized and initialized declarators; None for an abstract
    declarator (one that declares no name).
    """
    while declarator is not None:
        if declarator.type in ("identifier", "field_identifier", "type_identifier"):
            return declarator
        if declarator.type.startswith("abstract_"):
            return None
        if declarator.type == "parenthesized_declarator":
            # '(*name)': the inner declarator is not a field, it is the only
            # named child that is not a qualifier or attribute.
            inner = [
                c
                for c in declarator.named_children
                if c.type not in ("type_qualifier", "attribute_specifier", "ms_call_modifier")
            ]
            declarator = inner[0] if inner else None
            continue
        declarator = declarator.child_by_field_name("declarator")
    return None


def _type_node(decl: Node) -> Node | None:
    """The type specifier of a declaration, parameter or field node."""
    return decl.child_by_field_name("type")


def _is_bool_type(decl: Node) -> bool:
    type_node = _type_node(decl)
    return type_node is not None and _text(type_node) in ("bool", "_Bool")


def _is_scalar(declarator: Node | None) -> bool:
    """Whether a declarator declares the type itself, not a pointer, array or function."""
    if declarator is not None and declarator.type == "init_declarator":
        declarator = declarator.child_by_field_name("declarator")
    return declarator is not None and declarator.type == "identifier"


def _is_bool_literal_violation(value: Node | None) -> bool:
    return value is not None and value.type == "number_literal" and _text(value) in ("0", "1")


_WORD = re.compile(r"[A-Za-z_]\w*")

# An integer constant: the digits by radix and the suffix; floats do not match.
_INTEGER = re.compile(r"(?P<hex>0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*|[1-9]\d*)(?P<suffix>[uUlL]*)")

# Bit-field types whose signedness is implementation-defined (Rule 6.1).
_PLAIN_INT_TYPES = {"int", "char", "short", "short int", "long", "long int", "long long"}

_STDLIB_ATO = {"atof", "atoi", "atol", "atoll"}

# Statements whose body must be a compound statement (Rule 15.6).
_CONTROL_STATEMENTS = ("if_statement", "for_statement", "while_statement", "do_statement")

# Tokens of a macro body: identifiers, numbers, strings, character constants,
# the preprocessor operators and single punctuation characters.
_TOKEN = re.compile(
    r'[A-Za-z_]\w*|0[xX][0-9a-fA-F]+\w*|\d+\.?\d*\w*|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27'
    r"|##|#|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[^\s]"
)

# Tokens next to which a bare macro parameter needs no parentheses: it is the
# whole operand, an argument, an array index or a statement by itself.
_PARAM_SAFE_NEIGHBORS = {"(", ")", ",", "[", "]", "{", "}", ";", "#", "##", ":", "?", "="}


_NOT_DEFINED = re.compile(r"!\s*defined\s*\(?\s*([A-Za-z_]\w*)\s*\)?\s*$")


def _guard_name(node: Node) -> str | None:
    """The macro an include guard tests, for '#ifndef X' and '#if !defined(X)'."""
    if node.type == "preproc_ifdef" and _text(node).startswith("#ifndef"):
        name = node.child_by_field_name("name")
        return _text(name) if name is not None else None
    if node.type == "preproc_if":
        condition = node.child_by_field_name("condition")
        if condition is not None:
            m = _NOT_DEFINED.fullmatch(_text(condition).strip())
            if m is not None:
                return m.group(1)
    return None


def _is_token_safe(token: str) -> bool:
    """
    Whether a macro parameter next to 'token' is not an operand of an operator:
    punctuation that delimits it, or a word ('struct name[]', 'sep F(x)'),
    which juxtaposes rather than combines.
    """
    return token in _PARAM_SAFE_NEIGHBORS or _WORD.fullmatch(token) is not None


# Statements that end a function's control flow.
_JUMPS = ("break_statement", "return_statement", "goto_statement", "continue_statement")

# Keywords that error recovery can leave in function position, as in
# '} else if (x) {' parsed as a definition of a function named 'if'.
_KEYWORDS = {"if", "else", "for", "while", "do", "switch", "return", "sizeof", "goto"}


def _in_macro_body(node: Node) -> bool:
    """
    Whether a node spans a line continuation, i.e. sits in the body of a macro
    definition. A '#define' body is opaque text to the parser; when it does
    not recognize one (a long, nested body), the statements inside come
    through as declarations split by '##' tokens and are not checked either.
    """
    return node.text is not None and b"\\\n" in node.text


def _is_macro_name(name: str) -> bool:
    """
    Whether a name in function position is a macro rather than a function.

    A macro invocation at file scope ('SYS_INIT(init, PRE_KERNEL_1, 0);') or an
    attribute macro after a declarator ('uint8_t buf[16] __aligned(8);') parses
    as a function declarator whose "parameters" are the macro arguments. Zephyr
    spells macros in upper case, and the attribute macros of the toolchain and
    of sys/util.h start with a double underscore.
    """
    return name.startswith("__") or name in _KEYWORDS or not any(c.islower() for c in name)


def _block_comment(comment: str) -> str | None:
    """
    The /* */ spelling of a // comment, keeping Doxygen markers; None when the
    comment contains '*/', which would end the block comment early.
    """
    if "*/" in comment:
        return None
    body = comment[2:]
    opening = "/*"
    if body.startswith(("/", "!")):
        body = body[1:]
        opening = "/**"
        if body.startswith("<"):
            body = body[1:]
            opening = "/**<"
    body = body.strip()
    return f"{opening} {body} */" if body else f"{opening} */"


def _comment_fix(node: Node) -> list[Edit] | None:
    block = _block_comment(_text(node))
    return None if block is None else _replace(node, block)


def _pointer_depth(declarator: Node | None) -> int:
    """Number of pointer levels in a declarator ('char ***p' has three)."""
    depth = 0
    while declarator is not None:
        if declarator.type in ("pointer_declarator", "abstract_pointer_declarator"):
            depth += 1
        elif declarator.type == "parenthesized_declarator":
            inner = [c for c in declarator.named_children if c.type != "type_qualifier"]
            declarator = inner[0] if inner else None
            continue
        declarator = declarator.child_by_field_name("declarator")
    return depth


# Function-like operators that yield a constant (Rule 18.8).
_CONSTANT_FUNCTIONS = {
    "sizeof",
    "offsetof",
    "alignof",
    "_Alignof",
    "__alignof__",
    "__builtin_offsetof",
}


def _is_constant_expression(node: Node, objects: set[str]) -> bool:
    """
    Whether an array size is a constant expression, as far as the syntax tells.
    'objects' are the names declared as objects in the scopes visible at the
    array: a size that reads one of them, or a member, an element or a pointer
    target, is variable, as is a call to anything but a macro (upper case name)
    or a constant operator such as offsetof. A name that is not a known object
    (an enumerator, a macro) counts as constant.
    """
    stack = [node]
    while stack:
        n = stack.pop()
        if n.type == "identifier":
            if _text(n) in objects:
                return False
            continue
        if n.type in ("field_expression", "subscript_expression", "pointer_expression"):
            return False
        if n.type == "call_expression":
            function = n.child_by_field_name("function")
            if function is None:
                return False
            callee = _text(function)
            if not _is_macro_name(callee) and callee not in _CONSTANT_FUNCTIONS:
                return False
            # Macro arguments can be anything, e.g. ARRAY_SIZE(buf).
            continue
        if n.type == "sizeof_expression":
            continue
        stack.extend(n.named_children)
    return True


def _switch_clauses(body: Node) -> list[list[Node]]:
    """
    The clauses of a switch body: each is the run of case_statement nodes that
    ends with the one carrying statements ('case 1: case 2: x(); break;' is one
    clause). A trailing run of bare labels is a clause of its own.
    """
    clauses: list[list[Node]] = []
    run: list[Node] = []
    for case in _flatten_preproc(body):
        if case.type != "case_statement":
            continue
        run.append(case)
        if _case_statements(case):
            clauses.append(run)
            run = []
    if run:
        clauses.append(run)
    return clauses


def _flatten_preproc(node: Node) -> list[Node]:
    """The named children of a node, with conditional compilation blocks opened up."""
    result: list[Node] = []
    for child in node.named_children:
        if child.type.startswith("preproc_") and child.type not in ("preproc_call", "preproc_def"):
            result.extend(_flatten_preproc(child))
        else:
            result.append(child)
    return result


def _case_statements(case: Node) -> list[Node]:
    """The statements of a case_statement, without its label value and comments."""
    value = case.child_by_field_name("value")
    return [c for c in case.named_children if c != value and c.type != "comment"]


class _Checker:
    def __init__(self, reserved: set[str], source: bytes = b""):
        self.reserved = reserved
        self.source = source
        self.issues: list[Issue] = []

    # The bool-literal rule needs to know which identifiers denote bool objects
    # at the point of an assignment. Names are tracked per enclosing scope and
    # resolved innermost first; a declaration of the same name with another
    # type in an inner scope shadows the outer bool.
    def _record_declared_type(self, scopes: list[dict[str, bool]], decl: Node) -> None:
        if decl.parent is not None and decl.parent.type.startswith("preproc_"):
            # Declared under conditional compilation: another branch may give
            # the name another type, so its assignments are not judged.
            return
        is_bool = _is_bool_type(decl)
        for child in decl.children_by_field_name("declarator"):
            name = _declared_name(child)
            if name is not None and name.type == "identifier":
                # 'bool *p = 0' holds a null pointer, not a boolean value.
                scopes[-1][_text(name)] = is_bool and _is_scalar(child)

    def _is_bool_object(self, scopes: list[dict[str, bool]], name: str) -> bool:
        for scope in reversed(scopes):
            if name in scope:
                return scope[name]
        return False

    def _check_name(self, name: Node | None, decl: Node, what: str) -> None:
        """reserved-identifier and tag-name for one declared name."""
        if name is None:
            return
        text = _text(name)
        if text in self.reserved:
            self.issues.append(
                _issue(
                    name,
                    "reserved-identifier",
                    f"{what} '{text}' uses a reserved identifier (MISRA C:2012 Rule 21.2)",
                )
            )
        type_node = _type_node(decl)
        if type_node is not None and type_node.type in _TAGGED_SPECIFIERS:
            tag = type_node.child_by_field_name("name")
            if tag is not None and _text(tag) == text:
                keyword = type_node.type.removesuffix("_specifier")
                self.issues.append(
                    _issue(
                        name,
                        "tag-name",
                        f"{what} '{text}' is named after its {keyword} tag (MISRA C:2012 Rule 5.7)",
                    )
                )

    def _check_parameters(self, function: str, params: Node) -> None:
        """unnamed-parameter for one parameter list, recursing into function pointers."""
        for param in params.named_children:
            if param.type != "parameter_declaration" or any(
                c.type == "ERROR" for c in param.children
            ):
                # variadic_parameter ('...'), comments, or error recovery.
                continue
            declarator = param.child_by_field_name("declarator")
            name = _declared_name(declarator)
            type_node = _type_node(param)
            if (
                type_node is not None
                and type_node.type == "type_identifier"
                and _is_macro_name(_text(type_node))
            ):
                # 'f(ARGS(int a, int b))': a macro wraps the parameters.
                continue
            if name is None and not (
                declarator is None and type_node is not None and _text(type_node) == "void"
            ):
                self.issues.append(
                    _issue(
                        param,
                        "unnamed-parameter",
                        f"parameter of '{function}' has no name (MISRA C:2012 Rule 8.2)",
                    )
                )
            # A function pointer parameter carries its own parameter list.
            inner = declarator
            while inner is not None and inner.type != "function_declarator":
                inner = inner.child_by_field_name("declarator")
            if inner is not None:
                inner_params = inner.child_by_field_name("parameters")
                if inner_params is not None:
                    self._check_parameters(function, inner_params)

    def _check_function_declarator(self, declarator: Node, decl: Node) -> None:
        if _in_macro_body(declarator):
            return
        name = _declared_name(declarator)
        if name is None or name.type != "identifier":
            return
        function = _text(name)
        if _is_macro_name(function):
            return
        self._check_name(name, decl, "function")
        params = declarator.child_by_field_name("parameters")
        if params is not None:
            self._check_parameters(function, params)
            for param in params.named_children:
                if param.type == "parameter_declaration":
                    self._check_name(
                        _declared_name(param.child_by_field_name("declarator")),
                        param,
                        "parameter",
                    )

    def _check_declaration(self, decl: Node, scopes: list[dict[str, bool]]) -> None:
        """A declaration at file or block scope."""
        type_node = _type_node(decl)
        if _in_macro_body(decl) or type_node is None or type_node.type == "macro_type_specifier":
            # 'K_SEM_DEFINE(sem, 0, 1);' parses as a declaration whose type is
            # the macro invocation.
            return
        if (
            type_node.type == "type_identifier"
            and _is_macro_name(_text(type_node))
            and not any(c.type == "ERROR" and _WORD.fullmatch(_text(c)) for c in decl.children)
        ):
            # A statement after a stray '#ifdef FEATURE_X' parses as a
            # declaration of type FEATURE_X. When the macro is followed by an
            # ERROR node holding a single word, as in 'ALWAYS_INLINE int f(int
            # x);', that word is the real type and the declarator is a
            # function after all.
            return
        for declarator in decl.children_by_field_name("declarator"):
            if declarator.type == "function_declarator":
                self._check_function_declarator(declarator, decl)
                continue
            inner = declarator
            if inner.type == "init_declarator":
                inner = inner.child_by_field_name("declarator")
                if (
                    _is_bool_type(decl)
                    and _is_scalar(inner)
                    and _is_bool_literal_violation(declarator.child_by_field_name("value"))
                ):
                    value = declarator.child_by_field_name("value")
                    self.issues.append(
                        _issue(
                            value,
                            "bool-literal",
                            "bool object initialized with an integer literal, use true/false "
                            "(MISRA C:2012 Rule 10.1)",
                            _replace(value, "true" if _text(value) == "1" else "false"),
                        )
                    )
            # 'void *alloc(int);' wraps the function declarator in a pointer.
            probe = inner
            while probe is not None and probe.type == "pointer_declarator":
                probe = probe.child_by_field_name("declarator")
            if probe is not None and probe.type == "function_declarator":
                self._check_function_declarator(probe, decl)
                continue
            self._check_name(_declared_name(inner), decl, "object")
        self._record_declared_type(scopes, decl)

    def _check_assignment(self, node: Node, scopes: list[dict[str, bool]]) -> None:
        left = node.child_by_field_name("left")
        operator = node.child_by_field_name("operator")
        value = node.child_by_field_name("right")
        if left is None or left.type != "identifier" or operator is None or _text(operator) != "=":
            return
        if self._is_bool_object(scopes, _text(left)) and _is_bool_literal_violation(value):
            self.issues.append(
                _issue(
                    value,
                    "bool-literal",
                    "bool object assigned an integer literal, use true/false "
                    "(MISRA C:2012 Rule 10.1)",
                    _replace(value, "true" if _text(value) == "1" else "false"),
                )
            )

    def _check_function_definition(self, node: Node, declarator: Node) -> None:
        """inline-static, then the body rules."""
        specifiers = {_text(c) for c in node.children if c.type == "storage_class_specifier"}
        macros = {_text(c) for c in node.children if c.type == "type_identifier"}
        if ("inline" in specifiers or "ALWAYS_INLINE" in macros) and "static" not in specifiers:
            self.issues.append(
                _issue(
                    node,
                    "inline-static",
                    "inline function is not static (MISRA C:2012 Rule 8.10)",
                )
            )
        name = _declared_name(declarator)
        body = node.child_by_field_name("body")
        if name is not None and body is not None:
            self._check_function_body(_text(name), body)

    def _check_bitfield(self, field: Node) -> None:
        if not any(c.type == "bitfield_clause" for c in field.children):
            return
        type_node = _type_node(field)
        if type_node is not None and _text(type_node) in _PLAIN_INT_TYPES:
            self.issues.append(
                _issue(
                    field,
                    "bitfield-type",
                    f"bit-field of plain '{_text(type_node)}' has implementation-defined "
                    "signedness, use an explicitly unsigned or signed type "
                    "(MISRA C:2012 Rule 6.1)",
                )
            )

    def _check_braces(self, node: Node) -> None:
        """Rule 15.6: the body of an if, else, for, while or do is a compound statement."""
        condition = node.child_by_field_name("condition")
        if (
            node.type == "while_statement"
            and condition is not None
            and _text(condition)
            in (
                "(0)",
                "(false)",
            )
        ):
            # The tail of a 'do { } while (0)' macro body the parser did not
            # recognize; what follows it is not the loop body.
            return
        for field in ("consequence", "body"):
            body = node.child_by_field_name(field)
            if body is not None and body.type != "compound_statement":
                self.issues.append(
                    _issue(
                        body,
                        "braces",
                        f"body of '{_text(node.children[0])}' is not a compound statement, "
                        "add braces (MISRA C:2012 Rule 15.6)",
                        self._brace_body(node, body),
                    )
                )
        alternative = node.child_by_field_name("alternative")
        if alternative is not None:
            body = alternative.named_children[-1] if alternative.named_children else None
            if body is not None and body.type not in ("compound_statement", "if_statement"):
                self.issues.append(
                    _issue(
                        body,
                        "braces",
                        "body of 'else' is not a compound statement, add braces "
                        "(MISRA C:2012 Rule 15.6)",
                        self._brace_body(alternative, body),
                    )
                )

    def _brace_body(self, statement: Node, body: Node) -> list[Edit] | None:
        """
        Edits wrapping 'body' in braces, Zephyr style: ' {' after the header,
        the body on its own line one indentation level in, '}' on its own line
        at the header's indentation ('} while (...)' for a do statement).
        """
        if body.type == "preproc_call" or body.type.startswith("preproc_"):
            # The body is split by conditional compilation; not mechanical.
            return None
        source = self.source
        line_start = source.rfind(b"\n", 0, statement.start_byte) + 1
        indent = source[line_start : statement.start_byte]
        if indent.strip():
            # The header does not start its line ('} else' does): take the
            # indentation of that line.
            indent = indent[: len(indent) - len(indent.lstrip())]
        header_end = len(source[: body.start_byte].rstrip())
        gap = source[header_end : body.start_byte]
        edits = [
            Edit(
                header_end,
                body.start_byte,
                b" {" + (gap if b"\n" in gap else b"\n" + indent + b"\t"),
            )
        ]
        after = source[body.end_byte :]
        trailing = len(after) - len(after.lstrip())
        if statement.type == "do_statement" or after[trailing:].startswith(b"else"):
            # 'while' of a do statement and 'else' of an if statement go on
            # the line of the closing brace.
            edits.append(Edit(body.end_byte, body.end_byte + trailing, b"\n" + indent + b"} "))
        else:
            edits.append(Edit(body.end_byte, body.end_byte, b"\n" + indent + b"}"))
        return edits

    def _check_macro_parameters(self, node: Node) -> None:
        """Rule 20.7: each parameter instance in a macro body is parenthesized."""
        params = node.child_by_field_name("parameters")
        value = node.child_by_field_name("value")
        if params is None or value is None:
            return
        names = {_text(p) for p in params.named_children if p.type == "identifier"}
        if not names:
            return
        body = _text(value)
        # A line splice is not a token.
        matches = [m for m in _TOKEN.finditer(body) if m.group() != "\\"]
        tokens = [m.group() for m in matches]
        for i, token in enumerate(tokens):
            if token not in names:
                continue
            before = tokens[i - 1] if i > 0 else "("
            after = tokens[i + 1] if i + 1 < len(tokens) else ")"
            if before in ("#", "##") or after == "##":
                # Stringified or pasted: parentheses would change the result.
                continue
            if before in ("->", "."):
                # A member designator ('(s)->m') cannot be parenthesized.
                continue
            if after == "(":
                # Used as a function name: (f)(x) is not what is meant.
                continue
            if after == "*" and i + 2 < len(tokens) and tokens[i + 2] == ")":
                # A type in a cast: '((type *)0)->member'.
                continue
            if _is_token_safe(before) and _is_token_safe(after):
                continue
            # Report at the occurrence in the body, which is the line a change
            # to the macro touches.
            prefix = body[: matches[i].start()]
            line = value.start_point.row + 1 + prefix.count("\n")
            if "\n" in prefix:
                col = len(prefix.rsplit("\n", 1)[1].encode("utf-8")) + 1
            else:
                col = value.start_point.column + len(prefix.encode("utf-8")) + 1
            self.issues.append(
                Issue(
                    line,
                    col,
                    "macro-parameter",
                    f"macro parameter '{token}' is used without parentheses in the body, "
                    f"next to '{before if before not in _PARAM_SAFE_NEIGHBORS else after}' "
                    "(MISRA C:2012 Rule 20.7)",
                )
            )
            break

    def _check_number(self, node: Node) -> None:
        """octal-constant, unsigned-suffix, lowercase-l-suffix, language-extension."""
        text = _text(node)
        m = _INTEGER.fullmatch(text)
        if m is None:
            return
        digits, suffix = m.group("hex"), m.group("suffix")
        if "l" in suffix:
            self.issues.append(
                _issue(
                    node,
                    "lowercase-l-suffix",
                    f"'{text}' uses a lowercase l suffix, write L (MISRA C:2012 Rule 7.3)",
                    _replace(node, digits + suffix.upper()),
                )
            )
        if digits.startswith(("0b", "0B")):
            # The same value in hexadecimal, one digit per four bits.
            hexadecimal = f"0x{int(digits[2:], 2):0{(len(digits) - 2 + 3) // 4}X}"
            self.issues.append(
                _issue(
                    node,
                    "language-extension",
                    f"'{text}' is a binary literal, a language extension (MISRA C:2012 Rule 1.2)",
                    _replace(node, hexadecimal + suffix),
                )
            )
            return
        if digits.startswith(("0x", "0X")):
            value = int(digits, 16)
        elif len(digits) > 1 and digits.startswith("0"):
            self.issues.append(
                _issue(
                    node, "octal-constant", f"'{text}' is an octal constant (MISRA C:2012 Rule 7.1)"
                )
            )
            value = int(digits, 8)
        else:
            # A decimal constant without a suffix never has an unsigned type.
            return
        if not suffix and (0x80000000 <= value < 0x100000000 or value >= 0x8000000000000000):
            # Without a suffix the constant already has an unsigned type on
            # every target (it does not fit int, nor long long), so adding U
            # changes nothing but the spelling. With an L suffix the type
            # depends on the width of long, so that case is left alone.
            self.issues.append(
                _issue(
                    node,
                    "unsigned-suffix",
                    f"'{text}' has an unsigned type, add a U suffix (MISRA C:2012 Rule 7.2)",
                    _replace(node, text + "U"),
                )
            )

    def _check_switch(self, node: Node) -> None:
        """switch-default, default-position, switch-clauses."""
        body = node.child_by_field_name("body")
        if body is None:
            return
        clauses = _switch_clauses(body)
        labels = [case for clause in clauses for case in clause]
        defaults = [case for case in labels if case.child_by_field_name("value") is None]
        if not defaults:
            self.issues.append(
                _issue(
                    node, "switch-default", "switch has no default label (MISRA C:2012 Rule 16.4)"
                )
            )
        elif len(defaults) == 1 and defaults[0] != labels[0] and defaults[0] != labels[-1]:
            # Several defaults come from conditional compilation; which one is
            # last depends on the configuration, so their order is not judged.
            self.issues.append(
                _issue(
                    defaults[0],
                    "default-position",
                    "default is neither the first nor the last label of the switch "
                    "(MISRA C:2012 Rule 16.5)",
                )
            )
        if len(clauses) < 2:
            self.issues.append(
                _issue(
                    node,
                    "switch-clauses",
                    "switch has fewer than two clauses, use an if statement "
                    "(MISRA C:2012 Rule 16.6)",
                )
            )

    def _check_function_body(self, function: str, body: Node) -> None:
        """recursion, unused-label and backward-goto for one function body."""
        labels: list[Node] = []
        gotos: list[Node] = []
        stack = [body]
        while stack:
            n = stack.pop()
            if n.type == "labeled_statement":
                labels.append(n)
            elif n.type == "goto_statement":
                gotos.append(n)
            elif n.type == "call_expression" and not _in_macro_body(n):
                callee = n.child_by_field_name("function")
                if callee is not None and callee.type == "identifier":
                    name = _text(callee)
                    if name == function:
                        self.issues.append(
                            _issue(
                                n, "recursion", f"'{name}' calls itself (MISRA C:2012 Rule 17.2)"
                            )
                        )
                    elif name in _STDLIB_ATO:
                        self.issues.append(
                            _issue(
                                n,
                                "stdlib-ato",
                                f"'{name}' has undefined behavior on invalid input, use "
                                "strtol/strtoul/strtod (MISRA C:2012 Rule 21.7)",
                            )
                        )
            stack.extend(n.named_children)

        targets: dict[str, list[Node]] = {}
        for goto in gotos:
            label = goto.child_by_field_name("label")
            if label is not None:
                targets.setdefault(_text(label), []).append(goto)
        for labeled in labels:
            label = labeled.child_by_field_name("label")
            if label is None or _in_macro_body(labeled):
                continue
            name = _text(label)
            if name not in targets:
                self.issues.append(
                    _issue(
                        label, "unused-label", f"label '{name}' is unused (MISRA C:2012 Rule 2.6)"
                    )
                )
                continue
            for goto in targets[name]:
                if goto.start_byte > labeled.start_byte:
                    self.issues.append(
                        _issue(
                            goto,
                            "backward-goto",
                            f"goto jumps back to '{name}' (MISRA C:2012 Rule 15.2)",
                        )
                    )

    def _check_pointer_nesting(self, decl: Node) -> None:
        for declarator in decl.children_by_field_name("declarator"):
            if _pointer_depth(declarator) > 2:
                name = _declared_name(declarator)
                what = f"'{_text(name)}'" if name is not None else "declarator"
                self.issues.append(
                    _issue(
                        declarator,
                        "pointer-nesting",
                        f"{what} has more than two levels of pointer nesting "
                        "(MISRA C:2012 Rule 18.5)",
                    )
                )

    def _check_vla(self, decl: Node, scopes: list[dict[str, bool]]) -> None:
        if any(
            c.type == "storage_class_specifier" and _text(c) in ("static", "extern")
            for c in decl.children
        ):
            return
        objects = {name for scope in scopes for name in scope}
        for declarator in decl.children_by_field_name("declarator"):
            inner = declarator
            if inner.type == "init_declarator":
                inner = inner.child_by_field_name("declarator")
            while inner is not None and inner.type in (
                "pointer_declarator",
                "parenthesized_declarator",
            ):
                inner = (
                    inner.child_by_field_name("declarator")
                    if inner.type == "pointer_declarator"
                    else (inner.named_children[0] if inner.named_children else None)
                )
            while inner is not None and inner.type == "array_declarator":
                size = inner.child_by_field_name("size")
                if size is not None and not _is_constant_expression(size, objects):
                    name = _declared_name(inner)
                    self.issues.append(
                        _issue(
                            inner,
                            "vla",
                            f"'{_text(name) if name else ''}' is a variable-length array "
                            "(MISRA C:2012 Rule 18.8)",
                        )
                    )
                    break
                inner = inner.child_by_field_name("declarator")

    def check_include_guard(self, root: Node) -> None:
        """include-guard: the header is one '#ifndef X' / '#define X' block."""
        # Includes are guarded by their own header and may precede the guard.
        top = [c for c in root.named_children if c.type not in ("comment", "preproc_include")]
        if not top:
            return
        for n in top:
            if n.type == "preproc_call":
                directive = n.child_by_field_name("directive")
                if directive is not None and _text(directive) == "#pragma":
                    argument = n.child_by_field_name("argument")
                    if argument is not None and _text(argument).strip() == "once":
                        return
        guards = [(n, name) for n in top if (name := _guard_name(n)) is not None]
        if not guards:
            self.issues.append(
                Issue(1, 1, "include-guard", "header has no include guard (MISRA C:2012 Dir 4.10)")
            )
            return
        guard, name = guards[0]
        define = next((c for c in guard.named_children if c.type == "preproc_def"), None)
        defined = define.child_by_field_name("name") if define is not None else None
        if defined is None or _text(defined) != name:
            self.issues.append(
                _issue(
                    guard,
                    "include-guard",
                    f"include guard '{name}' is not defined right after its test "
                    "(MISRA C:2012 Dir 4.10)",
                )
            )
            return
        outside = next((n for n in top if n != guard), None)
        if outside is not None:
            self.issues.append(
                _issue(
                    outside,
                    "include-guard",
                    f"code outside the include guard '{name}' (MISRA C:2012 Dir 4.10)",
                )
            )

    def walk(self, root: Node) -> None:
        """
        Check the tree under 'root'.

        Error recovery wraps the tokens it could not fit into the grammar in an
        ERROR node, and what it recovered inside is unreliable (a statement
        shows up as a declaration, for instance), except for a complete function
        definition. The root itself is an ERROR node when the file as a whole
        did not parse; only its function definitions are checked then.

        The walk is iterative: a generated header holding one large initializer
        nests deeper than the Python stack allows.
        """
        # (node, scopes visible at node, inside an ERROR node)
        stack = [(root, [{}], root.type == "ERROR")]
        while stack:
            node, scopes, recovered = stack.pop()
            node_type = node.type
            if node_type == "ERROR":
                recovered = True
            if node_type in _SCOPES:
                scopes = [*scopes, {}]
            elif node_type == "function_definition":
                recovered = False
                declarator = node.child_by_field_name("declarator")
                while declarator is not None and declarator.type != "function_declarator":
                    declarator = declarator.child_by_field_name("declarator")
                if declarator is not None:
                    self._check_function_declarator(declarator, node)
                    self._check_function_definition(node, declarator)
                    # Parameters are visible in the body, which is the next scope.
                    params = declarator.child_by_field_name("parameters")
                    scopes = [*scopes, {}]
                    if params is not None:
                        for param in params.named_children:
                            if param.type == "parameter_declaration":
                                self._record_declared_type(scopes, param)
            elif recovered:
                pass
            elif node_type == "declaration":
                self._check_declaration(node, scopes)
                self._check_pointer_nesting(node)
                if len(scopes) > 1:
                    self._check_vla(node, scopes)
            elif node_type == "field_declaration":
                for declarator in node.children_by_field_name("declarator"):
                    if declarator.type != "function_declarator":
                        self._check_name(_declared_name(declarator), node, "member")
                self._check_pointer_nesting(node)
                self._check_bitfield(node)
            elif node_type == "parameter_declaration":
                self._check_pointer_nesting(node)
            elif node_type == "assignment_expression":
                self._check_assignment(node, scopes)
            elif node_type == "number_literal":
                self._check_number(node)
            elif node_type == "switch_statement" and not _in_macro_body(node):
                self._check_switch(node)
            elif node_type == "case_statement" and "..." in _text(node).split(":", 1)[0]:
                self.issues.append(
                    _issue(
                        node,
                        "language-extension",
                        "case range is a language extension (MISRA C:2012 Rule 1.2)",
                    )
                )
            elif node_type in _CONTROL_STATEMENTS and not _in_macro_body(node):
                self._check_braces(node)
            elif node_type == "comment" and _text(node).startswith("//"):
                self.issues.append(
                    _issue(
                        node,
                        "line-comment",
                        "use /* */ comments, not // (Zephyr coding style)",
                        _comment_fix(node),
                    )
                )
            elif node_type == "preproc_function_def":
                self._check_macro_parameters(node)
            elif (
                node_type == "conditional_expression"
                and node.child_by_field_name("consequence") is None
            ):
                self.issues.append(
                    _issue(
                        node,
                        "language-extension",
                        "'?:' with an omitted operand is a language extension "
                        "(MISRA C:2012 Rule 1.2)",
                    )
                )

            # Siblings share one scope dict, so a declaration recorded while
            # visiting an earlier sibling is visible to the later ones; push in
            # reverse so they are visited in source order.
            stack.extend((child, scopes, recovered) for child in reversed(node.children))


def check_text(text: str, reserved: set[str], header: bool = False) -> list[Issue]:
    """
    Return the list of guideline issues for the given C file contents.
    'reserved' is the set of reserved identifiers (see load_reserved());
    'header' selects the rules that apply to header files only.
    """
    source = text.encode("utf-8")
    root = _PARSER.parse(source).root_node
    checker = _Checker(reserved, source)
    if header:
        checker.check_include_guard(root)
    checker.walk(root)
    checker.issues.sort(key=lambda issue: (issue.line, issue.col))
    return checker.issues


MAX_FIX_PASSES = 8

# The rules whose issues carry a fix. A fix changes the spelling of the code,
# never what it does: an unsigned constant gets its U, a bool gets true/false,
# a comment or a literal is respelled, an unbraced body is braced.
FIXABLE_RULES = {
    "bool-literal",
    "braces",
    "language-extension",
    "line-comment",
    "lowercase-l-suffix",
    "unsigned-suffix",
}


def fix_text(
    text: str, reserved: set[str], header: bool = False, rules: set[str] | None = None
) -> tuple[str, list[Issue], list[Issue]]:
    """
    Apply the mechanical fixes to the given C file contents and return the new
    contents, the issues fixed and the issues that remain. 'rules' limits the
    fixes to those rules. Edits that overlap are applied over several passes,
    re-parsing in between, so nested statements all end up braced.
    """
    fixed: list[Issue] = []
    for _ in range(MAX_FIX_PASSES):
        issues = check_text(text, reserved, header)
        source = text.encode("utf-8")
        applied_end = len(source) + 1
        applied = False
        # Apply from the end so earlier offsets stay valid; skip an issue whose
        # edits overlap one already applied in this pass.
        for issue in sorted(
            issues, key=lambda i: -max(e.start for e in i.fix or [Edit(0, 0, b"")])
        ):
            if issue.fix is None or (rules is not None and issue.rule not in rules):
                continue
            if max(e.end for e in issue.fix) > applied_end:
                continue
            for edit in sorted(issue.fix, key=lambda e: -e.start):
                source = source[: edit.start] + edit.text + source[edit.end :]
            applied_end = min(e.start for e in issue.fix)
            fixed.append(issue)
            applied = True
        text = source.decode("utf-8")
        if not applied:
            break
    remaining = check_text(text, reserved, header)
    return text, fixed, remaining


def reserved_for(path: Path, reserved: set[str]) -> set[str]:
    """The reserved identifiers that apply to 'path': none in an exempt directory."""
    posix = path.as_posix()
    if any(f"/{d}" in f"/{posix}" for d in RESERVED_EXEMPT_DIRS):
        return set()
    return reserved


def check_file(path: Path, reserved: set[str]) -> list[Issue]:
    """Return the list of guideline issues for a single file."""
    return check_text(
        path.read_text(encoding="utf-8", errors="replace"),
        reserved_for(path, reserved),
        header=path.suffix == ".h",
    )


def fix_file(
    path: Path, reserved: set[str], rules: set[str] | None = None
) -> tuple[list[Issue], list[Issue]]:
    """Fix a file in place; return the issues fixed and those remaining."""
    text = path.read_text(encoding="utf-8")
    new_text, fixed, remaining = fix_text(
        text, reserved_for(path, reserved), path.suffix == ".h", rules
    )
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")
    return fixed, remaining


def _is_c_file(name: str) -> bool:
    return name.endswith((".c", ".h"))


def expand_paths(paths: list[str]) -> list[Path]:
    """
    Expand the given paths into a de-duplicated list of files. Files are used
    as-is; directories are searched recursively for C sources and headers.
    """
    files: list[Path] = []
    for path in paths:
        p = Path(path)
        if p.is_dir():
            files.extend(sorted(f for f in p.rglob("*") if f.is_file() and _is_c_file(f.name)))
        else:
            files.append(p)
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
        help="C files to check. Directories are searched recursively for *.c and *.h files.",
    )
    parser.add_argument(
        "--reserved-file",
        metavar="FILE",
        type=Path,
        action="append",
        default=[],
        help="extra reserved identifier list file (one name per line, '#' "
        f"comments), appended to '{DEFAULT_RESERVED_FILE.name}'. May be given "
        "multiple times.",
    )
    parser.add_argument(
        "--skip-rule",
        metavar="RULE",
        action="append",
        default=[],
        help="do not report this rule (may be given multiple times)",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="rewrite the files, applying the fixes that change spelling only: "
        f"{', '.join(sorted(FIXABLE_RULES))}. The remaining issues are reported.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        reserved = load_reserved([DEFAULT_RESERVED_FILE, *args.reserved_file])
    except OSError as err:
        print(f"error: {err}", file=sys.stderr)
        return EXIT_ERROR

    total = 0
    failed = False
    for path in expand_paths(args.paths):
        try:
            if args.fix:
                fixed, issues = fix_file(path, reserved, FIXABLE_RULES - set(args.skip_rule))
                for issue in sorted(fixed, key=lambda i: (i.line, i.col)):
                    print(f"{path}:{issue.line}:{issue.col}: [{issue.rule}] fixed")
            else:
                issues = check_file(path, reserved)
        except Exception:
            # Never let a checker failure masquerade as "no issues" (exit 0) or
            # "issues found" (exit 1): report it and exit with EXIT_ERROR.
            print(f"error: failed to check '{path}':", file=sys.stderr)
            traceback.print_exc()
            failed = True
            continue
        for issue in issues:
            if issue.rule in args.skip_rule:
                continue
            print(f"{path}:{issue.line}:{issue.col}: [{issue.rule}] {issue.message}")
            total += 1

    if failed:
        return EXIT_ERROR
    return EXIT_ISSUES if total else EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
