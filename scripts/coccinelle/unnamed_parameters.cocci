// Check violations of rule 8.2
// Function types shall be in prototype form with named parameters.
// https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_02.c
//
// A parameter is a violation when it carries a type but no identifier, both in
// the parameter list itself and in the parameter list of any function pointer
// parameter. Both declarations and definitions are checked, since the rule
// applies to every function type.
//
// Confidence: Moderate
// Copyright: (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

virtual report

@initialize:python@
@@

import re

IDENTIFIER = re.compile(r"^[A-Za-z_]\w*$")
STDINT = re.compile(r"^u?int(_least|_fast)?\d+_t$")

# Type names that can legally end a parameter with no identifier of its own.
TYPE_WORDS = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "bool", "_Bool", "const", "volatile", "restrict",
    "__restrict", "__restrict__", "ZRESTRICT", "struct", "union", "enum",
    "register", "size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t",
    "intmax_t", "uintmax_t", "wchar_t", "va_list", "FILE",
}


def split_params(tokens):
    """Split a token list on the commas that are not nested in parentheses."""
    params = []
    current = []
    depth = 0
    for token in tokens:
        if token in ("(", "["):
            depth += 1
        elif token in (")", "]"):
            depth -= 1
        if token == "," and depth == 0:
            params.append(current)
            current = []
        else:
            current.append(token)
    if current:
        params.append(current)
    return params


def strip_array(tokens):
    """Drop the trailing [] / [n] suffixes of an array parameter."""
    while tokens and tokens[-1] == "]":
        depth = 0
        for i in range(len(tokens) - 1, -1, -1):
            if tokens[i] == "]":
                depth += 1
            elif tokens[i] == "[":
                depth -= 1
                if depth == 0:
                    tokens = tokens[:i]
                    break
        else:
            break
    return tokens


def matching_paren(tokens, start):
    """Index of the ')' closing the '(' at index start, or -1."""
    depth = 0
    for i in range(start, len(tokens)):
        if tokens[i] == "(":
            depth += 1
        elif tokens[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def is_unnamed(tokens):
    """True when this parameter, or one nested in it, has no identifier."""
    tokens = [t for t in tokens if t]
    if not tokens or tokens == ["void"] or tokens == ["..."]:
        return False

    if "(" in tokens:
        open_paren = tokens.index("(")
        close_paren = matching_paren(tokens, open_paren)
        if close_paren < 0:
            return False

        if tokens[open_paren + 1 : open_paren + 2] == ["*"]:
            # Pointer declarator: type (*name)(args) or type (*name)[n]
            declarator = tokens[open_paren + 1 : close_paren]
            trailer = tokens[close_paren + 1 :]
        else:
            # Function typed parameter: type name(args)
            declarator = tokens[:open_paren]
            trailer = tokens[open_paren:]

        if not [t for t in declarator if t != "*"]:
            return True

        # A pointer to an array has no nested parameters to look at.
        if trailer[:1] == ["("] and trailer[-1:] == [")"]:
            for nested in split_params(trailer[1:-1]):
                if is_unnamed(nested):
                    return True
        return False

    tokens = strip_array(tokens)
    if not tokens:
        return True

    # A lone token is a type with no parameter name, e.g. "size_t".
    if len(tokens) == 1:
        return True

    last = tokens[-1]
    if not IDENTIFIER.match(last):
        return True
    if last in TYPE_WORDS or STDINT.match(last):
        return True

    # "struct tag" / "union tag" / "enum tag" without a parameter name.
    identifiers = [t for t in tokens if IDENTIFIER.match(t)]
    if tokens[0] in ("struct", "union", "enum") and len(identifiers) == 2:
        return True

    return False


def check_params(function, params, position):
    # Macro invocations parse as function declarations. Zephyr spells macros in
    # upper case, so anything without a lower case letter is not a function.
    if not any(c.islower() for c in str(function)):
        return

    # A parameter list may be split by conditional compilation. The directives
    # come through as part of the parameter text, drop them before parsing.
    text = re.sub(r"#[^\n]*", " ", str(params))

    for param in split_params(text.split()):
        if is_unnamed(param):
            msg = (
                "WARNING: Violation to rule 8.2 (Function types shall be in "
                "prototype form with named parameters) function: {}".format(function)
            )
            coccilib.report.print_report(position[0], msg)
            return

@r_decl@
identifier f;
type T;
parameter list P;
position p;
@@

T f@p(P);

@script:python depends on report@
f << r_decl.f;
P << r_decl.P;
p << r_decl.p;
@@

check_params(f, P, p)

@r_def@
identifier f;
type T;
parameter list P;
position p;
@@

T f@p(P) { ... }

@script:python depends on report@
f << r_def.f;
P << r_def.P;
p << r_def.p;
@@

check_params(f, P, p)
