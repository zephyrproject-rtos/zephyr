# Generates a Kconfig symbol reference in RST format, with a separate
# CONFIG_FOO.rst file for each symbol, and an alphabetical index with links in
# index.rst.

import kconfiglib
import os
import sys
import textwrap

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
:program:`Kconfig` using the :file:`~/doc/scripts/genrest/genrest.py` script.
Click on the option name in the table below for detailed information about
each option.

For symbols defined in multiple locations, the defaults, selects, implies, and
ranges from all instances will be listed on the first instance of the symbol in
the Kconfig display on the symbol's help page. This has to do with Kconfig
internals, as those properties belong to symbols, while e.g. prompts belong to
menu entries.

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

    with open(os.path.join(out_dir, "index.rst"), "w") as index_rst:
        index_rst.write(INDEX_RST_HEADER)

        # - Sort the symbols by name so that they end up in sorted order in
        #   index.rst
        #
        # - Use set() to get rid of duplicates for symbols defined in multiple
        #   locations.
        for sym in sorted(set(kconf.defined_syms), key=lambda sym: sym.name):
            # Write an RST file for the symbol
            write_sym_rst(sym, out_dir)

            # Add an index entry for the symbol that links to its RST file.
            # Also list its prompt(s), if any. (A symbol can have multiple
            # prompts if it has multiple definitions.)
            #
            # The strip() avoids RST choking on stuff like *foo *, when people
            # accidentally include leading/trailing whitespace in prompts.
            index_rst.write("   * - :option:`CONFIG_{}`\n     - {}\n"
                            .format(sym.name,
                                    " / ".join(node.prompt[0].strip()
                                               for node in sym.nodes
                                               if node.prompt)))

def write_sym_rst(sym, out_dir):
    # Writes documentation for 'sym' to <out_dir>/CONFIG_<sym.name>.rst

    kconf = sym.kconfig

    with open(os.path.join(out_dir, "CONFIG_{}.rst".format(sym.name)),
              "w") as sym_rst:

        # List all prompts on separate lines
        prompt_str = "\n\n".join("*{}*".format(node.prompt[0].strip())
                                 for node in sym.nodes if node.prompt) \
                     or "*(No prompt -- not directly user assignable.)*"

        # - :orphan: suppresses warnings for the symbol RST files not being
        #   included in any toctree
        #
        # - '.. title::' sets the title of the document (e.g. <title>). This
        #   seems to be poorly documented at the moment.
        sym_rst.write(":orphan:\n\n"
                      ".. title:: {0}\n\n"
                      ".. option:: CONFIG_{0}\n\n"
                      "{1}\n\n"
                      "Type: ``{2}``\n\n"
                      .format(sym.name,
                              prompt_str,
                              kconfiglib.TYPE_TO_STR[sym.type]))

        # Symbols with multiple definitions can have multiple help texts
        for node in sym.nodes:
            if node.help is not None:
                sym_rst.write("Help\n"
                              "====\n\n"
                              "{}\n\n"
                              .format(node.help))

        if sym.direct_dep is not kconf.y:
            sym_rst.write("Direct dependencies\n"
                          "===================\n\n"
                          "{}\n\n"
                          "*(Includes any dependencies from if's and menus.)*\n\n"
                          .format(kconfiglib.expr_str(sym.direct_dep)))

        if sym.defaults:
            sym_rst.write("Defaults\n"
                          "========\n\n")

            for value, cond in sym.defaults:
                default_str = kconfiglib.expr_str(value)
                if cond is not kconf.y:
                    default_str += " if " + kconfiglib.expr_str(cond)
                sym_rst.write(" - {}\n".format(default_str))

            sym_rst.write("\n")

        def write_select_imply_rst(expr):
            # Writes a link for each selecting symbol (if 'expr' is
            # sym.rev_dep) or each implying symbol (if 'expr' is
            # sym.weak_rev_dep)

            # The reverse dependencies from each select/imply are ORed together
            for select in kconfiglib.split_expr(expr, kconfiglib.OR):
                # - 'select/imply A if B' turns into A && B
                # - 'select/imply A' just turns into A
                #
                # In both cases, we can split on AND and pick the first
                # operand.
                sym_rst.write(" - :option:`CONFIG_{}`\n".format(
                    kconfiglib.split_expr(select, kconfiglib.AND)[0].name))

            sym_rst.write("\n")

        if sym.rev_dep is not kconf.n:
            sym_rst.write("Symbols that ``select`` this symbol\n"
                          "===================================\n\n")
            write_select_imply_rst(sym.rev_dep)

        if sym.weak_rev_dep is not kconf.n:
            sym_rst.write("Symbols that ``imply`` this symbol\n"
                          "==================================\n\n")
            write_select_imply_rst(sym.weak_rev_dep)

        # parsed-literal supports links in the definition
        sym_rst.write("Kconfig definition\n"
                      "==================\n\n"
                      ".. parsed-literal::\n\n"
                      "{}\n\n"
                      "*(Includes propagated dependencies, including from if's and menus.)*\n\n"
                      .format(textwrap.indent(str(sym), " "*4)))

        sym_rst.write("Definition location{}\n"
                      "====================\n\n"
                      "{}".format(
            "s" if len(sym.nodes) > 1 else "",
            "\n".join(" - ``{}:{}``".format(node.filename, node.linenr) for
                      node in sym.nodes)))

if __name__ == "__main__":
    write_kconfig_rst()
