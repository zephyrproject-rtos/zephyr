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
:program:`Kconfig` using the :file:`~/doc/scripts/genrest.py` script. Click on
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

    write_if_updated(os.path.join(out_dir, "index.rst"), index_rst)

def write_sym_rst(sym, out_dir):
    # Writes documentation for 'sym' to <out_dir>/CONFIG_<sym.name>.rst

    kconf = sym.kconfig

    # List all prompts on separate lines
    prompt_str = "\n\n".join("*{}*".format(node.prompt[0])
                             for node in sym.nodes if node.prompt) \
                 or "*(No prompt -- not directly user assignable.)*"

    # String with the RST for the symbol page
    #
    # - :orphan: suppresses warnings for the symbol RST files not being
    #   included in any toctree
    #
    # - '.. title::' sets the title of the document (e.g. <title>). This seems
    #   to be poorly documented at the moment.
    sym_rst = ":orphan:\n\n" \
              ".. title:: {0}\n\n" \
              ".. option:: CONFIG_{0}\n\n" \
              "{1}\n\n" \
              "Type: ``{2}``\n\n" \
              .format(sym.name, prompt_str, kconfiglib.TYPE_TO_STR[sym.type])

    # Symbols with multiple definitions can have multiple help texts
    for node in sym.nodes:
        if node.help is not None:
            sym_rst += "Help\n" \
                       "====\n\n" \
                       "{}\n\n" \
                       .format(node.help)

    if sym.direct_dep is not kconf.y:
        sym_rst += "Direct dependencies\n" \
                   "===================\n\n" \
                   "{}\n\n" \
                   "*(Includes any dependencies from if's and menus.)*\n\n" \
                   .format(kconfiglib.expr_str(sym.direct_dep))

    if sym.defaults:
        sym_rst += "Defaults\n" \
                   "========\n\n"

        for value, cond in sym.defaults:
            default_str = kconfiglib.expr_str(value)
            if cond is not kconf.y:
                default_str += " if " + kconfiglib.expr_str(cond)
            sym_rst += " - {}\n".format(default_str)

        sym_rst += "\n"

    def add_select_imply_rst(type_str, expr):
        # Writes a link for each selecting symbol (if 'expr' is sym.rev_dep) or
        # each implying symbol (if 'expr' is sym.weak_rev_dep). Also adds a
        # heading at the top, derived from type_str ("select"/"imply").

        nonlocal sym_rst

        heading = "Symbols that ``{}`` this symbol".format(type_str)
        sym_rst += "{}\n{}\n\n".format(heading, len(heading)*"=")

        # The reverse dependencies from each select/imply are ORed together
        for select in kconfiglib.split_expr(expr, kconfiglib.OR):
            # - 'select/imply A if B' turns into A && B
            # - 'select/imply A' just turns into A
            #
            # In both cases, we can split on AND and pick the first
            # operand.
            sym_rst += " - :option:`CONFIG_{}`\n".format(
                kconfiglib.split_expr(select, kconfiglib.AND)[0].name)

        sym_rst += "\n"

    if sym.rev_dep is not kconf.n:
        add_select_imply_rst("select", sym.rev_dep)

    if sym.weak_rev_dep is not kconf.n:
        add_select_imply_rst("imply", sym.weak_rev_dep)

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

            if node is kconf.top_node:
                break

            # Fancy Unicode arrow. Added in '93, so ought to be pretty safe.
            path = " → " + node.prompt[0] + path

        return "(top menu)" + path

    heading = "Kconfig definition"
    if len(sym.nodes) > 1:
        heading += "s"
    sym_rst += "{}\n{}\n\n".format(heading, len(heading)*"=")

    sym_rst += ".. highlight:: kconfig\n\n"

    sym_rst += "\n\n".join(
        "At ``{}:{}``, in menu ``{}``:\n\n"
        ".. parsed-literal::\n\n"
        "{}".format(node.filename, node.linenr, menu_path(node),
                    textwrap.indent(str(node), " "*4))
        for node in sym.nodes)

    sym_rst += "\n\n*(Definitions include propagated dependencies, " \
               "including from if's and menus.)*"

    write_if_updated(os.path.join(out_dir, "CONFIG_{}.rst".format(sym.name)),
                     sym_rst)


def write_if_updated(filename, s):
    # Writes 's' as the contents of 'filename', but only if it differs from the
    # current contents of the file. This avoids unnecessary timestamp updates,
    # which trigger documentation rebuilds.

    try:
        with open(filename) as f:
            if s == f.read():
                return
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

    with open(filename, "w") as f:
        f.write(s)


if __name__ == "__main__":
    write_kconfig_rst()
