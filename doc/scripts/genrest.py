# Generates a Kconfig symbol reference in RST format, with a separate
# CONFIG_FOO.rst file for each symbol, and an alphabetical index with links in
# index.rst.

import errno
import os
import sys
import textwrap

import kconfiglib

# "Extend" the standard kconfiglib.expr_str() to turn references to defined
# Kconfig symbols into RST links. Symbol.__str__() will then use the extended
# version.
#
# This is a bit hacky, but better than reimplementing Symbol.__str__() and/or
# kconfiglib.expr_str().

def expr_str_rst(expr):
    # Skip constant and undefined symbols by checking if expr.nodes is empty
    if isinstance(expr, kconfiglib.Symbol) and expr.nodes:
        # The "\ " avoids RST issues for !CONFIG_FOO -- see
        # http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#character-level-inline-markup
        return r"\ :option:`{0} <CONFIG_{0}>`".format(expr.name)

    # Choices appear as dependencies of choice symbols.
    #
    # Use a :ref: instead of an :option:. With an :option:, we'd have to have
    # an '.. option::' in the choice reference page as well. That would make
    # the internal choice ID show up in the documentation.
    #
    # Note that the first pair of <...> is non-syntactic here. We just display
    # choices links within <> in the documentation.
    if isinstance(expr, kconfiglib.Choice):
        return r"\ :ref:`<{}> <{}>`" \
               .format(choice_desc(expr), choice_id(expr))

    # We'll end up back in expr_str_rst() when expr_str_orig() does recursive
    # calls for subexpressions
    return expr_str_orig(expr)

expr_str_orig = kconfiglib.expr_str
kconfiglib.expr_str = expr_str_rst


INDEX_RST_HEADER = """.. _configuration:

Configuration Symbol Reference
##############################

Introduction
************

Kconfig files describe the configuration symbols supported in the build
system, the logical organization and structure that group the symbols in menus
and sub-menus, and the relationships between the different configuration
symbols that govern the valid configuration combinations.

The Kconfig files are distributed across the build directory tree. The files
are organized based on their common characteristics and on what new symbols
they add to the configuration menus.

The configuration options' information below is extracted directly from
:program:`Kconfig`. Click on
the option name in the table below for detailed information about each option.

Supported Options
*****************

.. list-table:: Alphabetized Index of Configuration Options
   :header-rows: 1

   * - Kconfig Symbol
     - Description
"""

def write_kconfig_rst():
    # The "main" function. Writes index.rst and the symbol RST files.

    if len(sys.argv) != 3:
        print("usage: {} <Kconfig> <output directory>", file=sys.stderr)
        sys.exit(1)

    kconf = kconfiglib.Kconfig(sys.argv[1])
    out_dir = sys.argv[2]

    # String with the RST for the index page
    index_rst = INDEX_RST_HEADER

    # - Sort the symbols by name so that they end up in sorted order in
    #   index.rst
    #
    # - Use set() to get rid of duplicates for symbols defined in multiple
    #   locations.
    for sym in sorted(set(kconf.defined_syms), key=lambda sym: sym.name):
        # Write an RST file for the symbol
        write_sym_rst(sym, out_dir)

        # Add an index entry for the symbol that links to its RST file. Also
        # list its prompt(s), if any. (A symbol can have multiple prompts if it
        # has multiple definitions.)
        index_rst += "   * - :option:`CONFIG_{}`\n     - {}\n".format(
            sym.name,
            " / ".join(node.prompt[0]
                       for node in sym.nodes if node.prompt))

    for choice in kconf.choices:
        # Write an RST file for the choice
        write_choice_rst(choice, out_dir)

    write_if_updated(os.path.join(out_dir, "index.rst"), index_rst)


def write_sym_rst(sym, out_dir):
    # Writes documentation for 'sym' to <out_dir>/CONFIG_<sym.name>.rst

    write_if_updated(os.path.join(out_dir, "CONFIG_{}.rst".format(sym.name)),
                     sym_header_rst(sym) +
                     help_rst(sym) +
                     direct_deps_rst(sym) +
                     defaults_rst(sym) +
                     select_imply_rst(sym) +
                     kconfig_definition_rst(sym))


def write_choice_rst(choice, out_dir):
    # Writes documentation for 'choice' to <out_dir>/choice_<n>.rst, where <n>
    # is the index of the choice in kconf.choices (where choices appear in the
    # same order as in the Kconfig files)

    write_if_updated(os.path.join(out_dir, choice_id(choice) + ".rst"),
                     choice_header_rst(choice) +
                     help_rst(choice) +
                     direct_deps_rst(choice) +
                     defaults_rst(choice) +
                     choice_syms_rst(choice) +
                     kconfig_definition_rst(choice))


def sym_header_rst(sym):
    # Returns RST that appears at the top of symbol reference pages

    # - :orphan: suppresses warnings for the symbol RST files not being
    #   included in any toctree
    #
    # - '.. title::' sets the title of the document (e.g. <title>). This seems
    #   to be poorly documented at the moment.
    return ":orphan:\n\n" \
           ".. title:: {0}\n\n" \
           ".. option:: CONFIG_{0}\n\n" \
           "{1}\n\n" \
           "Type: ``{2}``\n\n" \
           .format(sym.name, prompt_rst(sym),
                   kconfiglib.TYPE_TO_STR[sym.type])


def choice_header_rst(choice):
    # Returns RST that appears at the top of choice reference pages

    return ":orphan:\n\n" \
           ".. title:: {0}\n\n" \
           ".. _{1}:\n\n" \
           ".. describe:: {0}\n\n" \
           "{2}\n\n" \
           "Type: ``{3}``\n\n" \
           .format(choice_desc(choice), choice_id(choice),
                   prompt_rst(choice), kconfiglib.TYPE_TO_STR[choice.type])


def prompt_rst(sc):
    # Returns RST that lists the prompts of 'sc' (symbol or choice)

    return "\n\n".join("*{}*".format(node.prompt[0])
                       for node in sc.nodes if node.prompt) \
           or "*(No prompt -- not directly user assignable.)*"


def help_rst(sc):
    # Returns RST that lists the help text(s) of 'sc' (symbol or choice).
    # Symbols and choices with multiple definitions can have multiple help
    # texts.

    rst = ""

    for node in sc.nodes:
        if node.help is not None:
            rst += "Help\n" \
                   "====\n\n" \
                   "{}\n\n" \
                   .format(node.help)

    return rst


def direct_deps_rst(sc):
    # Returns RST that lists the direct dependencies of 'sc' (symbol or choice)

    if sc.direct_dep is sc.kconfig.y:
        return ""

    return "Direct dependencies\n" \
           "===================\n\n" \
           "{}\n\n" \
           "*(Includes any dependencies from if's and menus.)*\n\n" \
           .format(kconfiglib.expr_str(sc.direct_dep))


def defaults_rst(sc):
    # Returns RST that lists the 'default' properties of 'sc' (symbol or
    # choice)

    if not sc.defaults:
        return ""

    rst = "Defaults\n" \
          "========\n\n"

    for value, cond in sc.defaults:
        default_str = kconfiglib.expr_str(value)
        if cond is not sc.kconfig.y:
            default_str += " if " + kconfiglib.expr_str(cond)
        rst += "- {}\n".format(default_str)

    return rst + "\n"


def choice_syms_rst(choice):
    # Returns RST that lists the symbols contained in the choice

    if not choice.syms:
        return ""

    rst = "Choice options\n" \
          "==============\n\n"

    for sym in choice.syms:
        # Generates a link
        rst += "- {}\n".format(kconfiglib.expr_str(sym))

    return rst + "\n"


def select_imply_rst(sym):
    # Returns RST that lists the symbols that are 'select'ing or 'imply'ing the
    # symbol

    rst = ""

    def add_select_imply_rst(type_str, expr):
        # Writes a link for each selecting symbol (if 'expr' is sym.rev_dep) or
        # each implying symbol (if 'expr' is sym.weak_rev_dep). Also adds a
        # heading at the top, derived from type_str ("select"/"imply").

        nonlocal rst

        heading = "Symbols that ``{}`` this symbol".format(type_str)
        rst += "{}\n{}\n\n".format(heading, len(heading)*"=")

        # The reverse dependencies from each select/imply are ORed together
        for select in kconfiglib.split_expr(expr, kconfiglib.OR):
            # - 'select/imply A if B' turns into A && B
            # - 'select/imply A' just turns into A
            #
            # In both cases, we can split on AND and pick the first
            # operand.

            # kconfiglib.expr_str() generates a link
            rst += "- {}\n".format(kconfiglib.expr_str(
                kconfiglib.split_expr(select, kconfiglib.AND)[0]))

        rst += "\n"

    if sym.rev_dep is not sym.kconfig.n:
        add_select_imply_rst("select", sym.rev_dep)

    if sym.weak_rev_dep is not sym.kconfig.n:
        add_select_imply_rst("imply", sym.weak_rev_dep)

    return rst


def kconfig_definition_rst(sc):
    # Returns RST that lists the Kconfig definition(s) of 'sc' (symbol or
    # choice)

    def menu_path(node):
        path = ""

        while True:
            # This excludes indented submenus created in the menuconfig
            # interface when items depend on the preceding symbol.
            # is_menuconfig means anything that would be shown as a separate
            # menu (not indented): proper 'menu's, menuconfig symbols, and
            # choices.
            node = node.parent
            while not node.is_menuconfig:
                node = node.parent

            if node is node.kconfig.top_node:
                break

            # Fancy Unicode arrow. Added in '93, so ought to be pretty safe.
            path = " → " + node.prompt[0] + path

        return "(top menu)" + path

    heading = "Kconfig definition"
    if len(sc.nodes) > 1: heading += "s"
    rst = "{}\n{}\n\n".format(heading, len(heading)*"=")

    rst += ".. highlight:: kconfig\n\n"

    rst += "\n\n".join(
        "At ``{}:{}``, in menu ``{}``:\n\n"
        ".. parsed-literal::\n\n"
        "{}".format(node.filename, node.linenr, menu_path(node),
                    textwrap.indent(str(node), " "*4))
        for node in sc.nodes)

    rst += "\n\n*(Definitions include propagated dependencies, " \
           "including from if's and menus.)*"

    return rst


def choice_id(choice):
    # Returns "choice_<n>", where <n> is the index of the choice in the Kconfig
    # files. The choice that appears first has index 0, the next one index 1,
    # etc.
    #
    # This gives each choice a unique ID, which is used to generate its RST
    # filename and in cross-references. Choices (usually) don't have names, so
    # we can't use that, and the prompt isn't guaranteed to be unique.

    # Pretty slow, but fast enough
    return "choice_{}".format(choice.kconfig.choices.index(choice))


def choice_desc(choice):
    # Returns a description of the choice, used as the title of choice
    # reference pages and in link texts. The format is
    # "choice <name, if any>: <prompt text>"

    desc = "choice"

    if choice.name:
        desc += " " + choice.name

    # The choice might be defined in multiple locations. Use the prompt from
    # the first location that has a prompt.
    for node in choice.nodes:
        if node.prompt:
            desc += ": " + node.prompt[0]
            break

    return desc


def write_if_updated(filename, s):
    # Writes 's' as the contents of 'filename', but only if it differs from the
    # current contents of the file. This avoids unnecessary timestamp updates,
    # which trigger documentation rebuilds.

    try:
        with open(filename, 'r', encoding='utf-8') as f:
            if s == f.read():
                return
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

    with open(filename, "w", encoding='utf-8') as f:
        f.write(s)


if __name__ == "__main__":
    write_kconfig_rst()
