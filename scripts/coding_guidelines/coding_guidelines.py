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
                       standard library names).
  tag-name             MISRA C:2012 Rule 5.7. An object, parameter or member is
                       named after the tag of its own struct, union or enum type
                       ('struct device *device').
  bool-literal         MISRA C:2012 Rule 10.1 (Zephyr guideline on boolean
                       essential type). A bool object is initialized or assigned
                       with the integer literal 0 or 1 instead of false or true.
  unnamed-parameter    MISRA C:2012 Rule 8.2. A function declaration or
                       definition has a parameter with a type but no name,
                       including the parameters of a function pointer parameter.

The rules are derived from the tree-sitter-c syntax tree. tree-sitter parses C
without preprocessing and recovers from the constructs it cannot parse (macro
invocations at file scope, conditionally compiled fragments), so the checks run
on the source as written, independently of any build configuration, and only
the constructs they understand are reported on.

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
class Issue:
    """A single guideline violation, used for reporting."""

    line: int  # 1-based
    col: int  # 1-based
    rule: str
    message: str


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


def _issue(node: Node, rule: str, message: str) -> Issue:
    return Issue(node.start_point.row + 1, node.start_point.column + 1, rule, message)


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


class _Checker:
    def __init__(self, reserved: set[str]):
        self.reserved = reserved
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
                    self.issues.append(
                        _issue(
                            declarator.child_by_field_name("value"),
                            "bool-literal",
                            "bool object initialized with an integer literal, use true/false "
                            "(MISRA C:2012 Rule 10.1)",
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
            elif node_type == "field_declaration":
                for declarator in node.children_by_field_name("declarator"):
                    if declarator.type != "function_declarator":
                        self._check_name(_declared_name(declarator), node, "member")
            elif node_type == "assignment_expression":
                self._check_assignment(node, scopes)

            # Siblings share one scope dict, so a declaration recorded while
            # visiting an earlier sibling is visible to the later ones; push in
            # reverse so they are visited in source order.
            stack.extend((child, scopes, recovered) for child in reversed(node.children))


def check_text(text: str, reserved: set[str]) -> list[Issue]:
    """
    Return the list of guideline issues for the given C file contents.
    'reserved' is the set of reserved identifiers (see load_reserved()).
    """
    root = _PARSER.parse(text.encode("utf-8")).root_node
    checker = _Checker(reserved)
    checker.walk(root)
    checker.issues.sort(key=lambda issue: (issue.line, issue.col))
    return checker.issues


def check_file(path: Path, reserved: set[str]) -> list[Issue]:
    """Return the list of guideline issues for a single file."""
    return check_text(path.read_text(encoding="utf-8", errors="replace"), reserved)


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
