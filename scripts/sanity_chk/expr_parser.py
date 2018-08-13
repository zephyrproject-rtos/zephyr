#!/usr/bin/env python3
#
# Copyright (c) 2016 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import copy
import threading
import re

try:
    import ply.lex as lex
    import ply.yacc as yacc
except ImportError:
    print("PLY library for Python 3 not installed.")
    print("Please install the ply package using your workstation's")
    print("package manager or the 'pip' tool.")
    sys.exit(1)

reserved = {
    'and' : 'AND',
    'or' : 'OR',
    'not' : 'NOT',
    'in' : 'IN',
}

tokens = [
    "HEX",
    "STR",
    "INTEGER",
    "EQUALS",
    "NOTEQUALS",
    "LT",
    "GT",
    "LTEQ",
    "GTEQ",
    "OPAREN",
    "CPAREN",
    "OBRACKET",
    "CBRACKET",
    "COMMA",
    "SYMBOL",
    "COLON",
] + list(reserved.values())

def t_HEX(t):
    r"0x[0-9a-fA-F]+"
    t.value = str(int(t.value, 16))
    return t

def t_INTEGER(t):
    r"\d+"
    t.value = str(int(t.value))
    return t

def t_STR(t):
    r'\"([^\\\n]|(\\.))*?\"|\'([^\\\n]|(\\.))*?\''
    # nip off the quotation marks
    t.value = t.value[1:-1]
    return t

t_EQUALS = r"=="

t_NOTEQUALS = r"!="

t_LT = r"<"

t_GT = r">"

t_LTEQ = r"<="

t_GTEQ = r">="

t_OPAREN = r"[(]"

t_CPAREN = r"[)]"

t_OBRACKET = r"\["

t_CBRACKET = r"\]"

t_COMMA = r","

t_COLON = ":"

def t_SYMBOL(t):
    r"[A-Za-z_][0-9A-Za-z_]*"
    t.type = reserved.get(t.value, "SYMBOL")
    return t

t_ignore = " \t\n"

def t_error(t):
    raise SyntaxError("Unexpected token '%s'" % t.value)

lex.lex()

precedence = (
    ('left', 'OR'),
    ('left', 'AND'),
    ('right', 'NOT'),
    ('nonassoc' , 'EQUALS', 'NOTEQUALS', 'GT', 'LT', 'GTEQ', 'LTEQ', 'IN'),
)

def p_expr_or(p):
    'expr : expr OR expr'
    p[0] = ("or", p[1], p[3])

def p_expr_and(p):
    'expr : expr AND expr'
    p[0] = ("and", p[1], p[3])

def p_expr_not(p):
    'expr : NOT expr'
    p[0] = ("not", p[2])

def p_expr_parens(p):
    'expr : OPAREN expr CPAREN'
    p[0] = p[2]

def p_expr_eval(p):
    """expr : SYMBOL EQUALS const
            | SYMBOL NOTEQUALS const
            | SYMBOL GT number
            | SYMBOL LT number
            | SYMBOL GTEQ number
            | SYMBOL LTEQ number
            | SYMBOL IN list
            | SYMBOL COLON STR"""
    p[0] = (p[2], p[1], p[3])

def p_expr_single(p):
    """expr : SYMBOL"""
    p[0] = ("exists", p[1])

def p_list(p):
    """list : OBRACKET list_intr CBRACKET"""
    p[0] = p[2]

def p_list_intr_single(p):
    """list_intr : const"""
    p[0] = [p[1]]

def p_list_intr_mult(p):
    """list_intr : list_intr COMMA const"""
    p[0] = copy.copy(p[1])
    p[0].append(p[3])

def p_const(p):
    """const : STR
             | number"""
    p[0] = p[1]

def p_number(p):
    """number : INTEGER
              | HEX"""
    p[0] = p[1]

def p_error(p):
    if p:
        raise SyntaxError("Unexpected token '%s'" % p.value)
    else:
        raise SyntaxError("Unexpected end of expression")

if "PARSETAB_DIR" not in os.environ:
    parser = yacc.yacc(debug=0)
else:
    parser = yacc.yacc(debug=0, outputdir=os.environ["PARSETAB_DIR"])

def ast_sym(ast, env):
    if ast in env:
        return str(env[ast])
    return ""

def ast_sym_int(ast, env):
    if ast in env:
        v = env[ast]
        if v.startswith("0x") or v.startswith("0X"):
            return int(v, 16)
        else:
            return int(v, 10)
    return 0

def ast_expr(ast, env):
    if ast[0] == "not":
        return not ast_expr(ast[1], env)
    elif ast[0] == "or":
        return ast_expr(ast[1], env) or ast_expr(ast[2], env)
    elif ast[0] == "and":
        return ast_expr(ast[1], env) and ast_expr(ast[2], env)
    elif ast[0] == "==":
        return ast_sym(ast[1], env) == ast[2]
    elif ast[0] == "!=":
        return ast_sym(ast[1], env) != ast[2]
    elif ast[0] == ">":
        return ast_sym_int(ast[1], env) > int(ast[2])
    elif ast[0] == "<":
        return ast_sym_int(ast[1], env) < int(ast[2])
    elif ast[0] == ">=":
        return ast_sym_int(ast[1], env) >= int(ast[2])
    elif ast[0] == "<=":
        return ast_sym_int(ast[1], env) <= int(ast[2])
    elif ast[0] == "in":
        return ast_sym(ast[1], env) in ast[2]
    elif ast[0] == "exists":
        return True if ast_sym(ast[1], env) else False
    elif ast[0] == ":":
        return True if re.compile(ast[2]).match(ast_sym(ast[1], env)) else False

mutex = threading.Lock()

def parse(expr_text, env):
    """Given a text representation of an expression in our language,
    use the provided environment to determine whether the expression
    is true or false"""

    # Like it's C counterpart, state machine is not thread-safe
    mutex.acquire()
    try:
        ast = parser.parse(expr_text)
    finally:
        mutex.release()

    return ast_expr(ast, env)

# Just some test code
if __name__ == "__main__":

    local_env = {
        "A" : "1",
        "C" : "foo",
        "D" : "20",
        "E" : 0x100,
        "F" : "baz"
    }


    for line in open(sys.argv[1]).readlines():
        lex.input(line)
        for tok in iter(lex.token, None):
            print(tok.type, tok.value)

        parser = yacc.yacc()
        print(parser.parse(line))

        print(parse(line, local_env))




