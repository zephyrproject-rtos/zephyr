"""
Generates a Kconfig symbol reference in RST format, with a separate
CONFIG_FOO.rst file for each symbol, and an alphabetical index with links in
index.rst.

The generated symbol pages can be referenced in RST as :option:`foo`, and the
generated index page as `configuration options`_.

Optionally, the documentation can be split up based on where symbols are
defined. See the --modules flag.
"""

import argparse
import collections
import errno
from operator import attrgetter
import os
import pathlib
import sys
import textwrap

import kconfiglib


def rst_link(sc):
    # Returns an RST link (string) for the symbol/choice 'sc', or the normal
    # Kconfig expression format (e.g. just the name) for 'sc' if it can't be
    # turned into a link.

    if isinstance(sc, kconfiglib.Symbol):
        # Skip constant and undefined symbols by checking if expr.nodes is
        # empty
        if sc.nodes:
            # The "\ " avoids RST issues for !CONFIG_FOO -- see
            # http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#character-level-inline-markup
            return r"\ :option:`{0} <CONFIG_{0}>`".format(sc.name)

    elif isinstance(sc, kconfiglib.Choice):
        # Choices appear as dependencies of choice symbols.
        #
        # Use a :ref: instead of an :option:. With an :option:, we'd have to have
        # an '.. option::' in the choice reference page as well. That would make
        # the internal choice ID show up in the documentation.
        #
        # Note that the first pair of <...> is non-syntactic here. We just display
        # choices links within <> in the documentation.
        return r"\ :ref:`<{}> <{}>`" \
               .format(choice_desc(sc), choice_id(sc))

    # Can't turn 'sc' into a link. Use the standard Kconfig format.
    return kconfiglib.standard_sc_expr_str(sc)


def expr_str(expr):
    # Returns the Kconfig representation of 'expr', with symbols/choices turned
    # into RST links

    return kconfiglib.expr_str(expr, rst_link)


def main():
    # Writes index.rst and the symbol RST files

    init()

    # Writes index page(s)
    if modules:
        write_module_index_pages()
    else:
        write_index_page(kconf.unique_defined_syms, None, None,
                         desc_from_file(top_index_desc))

    # Write symbol pages
    if os.getenv("KCONFIG_TURBO_MODE") == "1":
        write_dummy_syms_page()
    else:
        write_sym_pages()


def init():
    # Initializes these globals:
    #
    # kconf:
    #   Kconfig instance for the configuration
    #
    # out_dir:
    #   Output directory
    #
    # non_module_title:
    #   Title for index of non-module symbols, as passed via
    #   --non-module-title
    #
    # top_index_desc/non_module_index_desc/all_index_desc:
    #   Set to the corresponding command-line arguments (or None if
    #   missing)
    #
    # modules:
    #   A list of (<title>, <suffix>, <path>, <index description>) tuples. See
    #   the --modules argument. Empty if --modules wasn't passed.
    #
    #   <path> is an absolute pathlib.Path instance, which is handy for robust
    #   path comparisons.
    #
    # strip_module_paths:
    #   True unless --keep-module-paths was passed

    global kconf
    global out_dir
    global non_module_title
    global top_index_desc
    global non_module_index_desc
    global all_index_desc
    global modules
    global strip_module_paths

    args = parse_args()

    kconf = kconfiglib.Kconfig(args.kconfig)
    out_dir = args.out_dir
    non_module_title = args.non_module_title
    top_index_desc = args.top_index_desc
    non_module_index_desc = args.non_module_index_desc
    all_index_desc = args.all_index_desc
    strip_module_paths = args.strip_module_paths

    modules = []
    for module_spec in args.modules:
        if module_spec.count(":") == 2:
            title, suffix, path_s = module_spec.split(":")
            index_text = DEFAULT_INDEX_DESCRIPTION
        elif module_spec.count(":") == 3:
            title, suffix, path_s, index_text_fname = module_spec.split(":")
            index_text = desc_from_file(index_text_fname)
        else:
            sys.exit("error: --modules argument '{}' should have the format "
                     "<title>:<suffix>:<path> or the format "
                     "<title>:<suffix>:<path>:<index description filename>"
                     .format(module_spec))

        path = pathlib.Path(path_s).resolve()
        if not path.exists():
            sys.exit("error: path '{}' in --modules argument does not exist"
                     .format(path))

        modules.append((title, suffix, path, index_text))


def parse_args():
    # Parses command-line arguments

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument(
        "--kconfig",
        metavar="KCONFIG",
        default="Kconfig",
        help="Top-level Kconfig file (default: Kconfig)")

    parser.add_argument(
        "--non-module-title",
        metavar="NON_MODULE_TITLE",
        default="Zephyr",
        help="""\
The title used for the index page that lists the symbols
that do not appear in any module (index-main.rst). Only
meaningful in --modules mode.""")

    parser.add_argument(
        "--top-index-desc",
        metavar="FILE",
        help="""\
Path to an RST file with description text for the top-level
index.rst index page. If missing, a generic description will
be used. Used both in --modules and non-modules mode.

See <index description filename> in the --modules
description as well.""")

    parser.add_argument(
        "--non-module-index-desc",
        metavar="FILE",
        help="""\
Like --top-index-desc, but for the index page that lists the
non-module symbols in --modules mode (index-main.rst).""")

    parser.add_argument(
        "--all-index-desc",
        metavar="FILE",
        help="""\
Like --top-index-desc, but for the index page that lists all
symbols in --modules mode (index-all.rst).""")

    parser.add_argument(
        "--modules",
        metavar="MODULE_SPECIFICATION",
        nargs="+",
        default=[],
        help="""\
Used to split the documentation into several index pages,
based on where symbols are defined.

MODULE_SPECIFICATION is either a <title>:<suffix>:<path>
tuple or a
<title>:<suffix>:<path>:<index description filename> tuple.
If the second form is used, <index description filename>
should be the path to an RST file, the contents of which
will appear on the index page that lists the symbols for the
module (under an automatically-inserted Overview heading).
If the first form is used, a generic description will be
used instead.

A separate index-<suffix>.rst index page is generated for
each tuple, with the title "<title> Configuration Options",
a 'configuration_options_<suffix>' RST link target, and
links to all symbols that appear under the tuple's <path>
(possibly more than one level deep). Symbols that do not
appear in any module are added to index-main.rst.

A separate index-all.rst page is generated that lists all
symbols, regardless of whether they're from a module or not.

The generated index.rst contains a TOC tree that links to
the other index-*.rst pages.

If a symbol is defined in more than one module (or both
inside and outside a module), it will be listed on several
index pages.

Passing --modules also tweaks how paths are displayed on
symbol information pages, showing
'<title>/path/within/module/Kconfig' for paths that fall
within modules. This path rewriting can be disabled with
--keep-module-paths.""")

    parser.add_argument(
        "--keep-module-paths",
        dest="strip_module_paths",
        action="store_false",
        help="Do not rewrite paths that fall within modules. See --modules.")

    parser.add_argument(
        "out_dir",
        metavar="OUTPUT_DIRECTORY",
        help="Directory to write .rst output files to")

    return parser.parse_args()


def write_module_index_pages():
    # Generate all index pages. Passing --modules will generate more than one.

    write_toplevel_index()

    # Maps each module title to a set of Symbols in the module
    module2syms = collections.defaultdict(set)
    # Symbols that do not appear in any module
    non_module_syms = set()

    for sym in kconf.unique_defined_syms:
        # Loop over all definition locations
        for node in sym.nodes:
            mod_title = path2module(node.filename)

            if mod_title is None:
                non_module_syms.add(node.item)
            else:
                module2syms[mod_title].add(node.item)

    # Write the index-main.rst index page, which lists the symbols that aren't
    # from a module
    write_index_page(non_module_syms, non_module_title, "main",
                     desc_from_file(non_module_index_desc))

    # Write the index-<suffix>.rst index pages, which list symbols from
    # modules. Iterate 'modules' instead of 'module2syms' so that an index page
    # gets written even if a module has no symbols, for consistency.
    for title, suffix, _, text in modules:
        write_index_page(module2syms[title], title, suffix, text)

    # Write the index-all.rst index page, which lists all symbols, including
    # both module and non-module symbols
    write_index_page(kconf.unique_defined_syms, "All", "all",
                     desc_from_file(all_index_desc))


def write_toplevel_index():
    # Used in --modules mode. Writes an index.rst with a TOC tree that links to
    # index-main.rst and the index-<suffix>.rst pages.

    rst = index_page_header(None, None, desc_from_file(top_index_desc)) + """
Subsystems
**********

.. toctree::
   :maxdepth: 1

"""

    rst += "   index-main\n"
    for _, suffix, _, _ in modules:
        rst += "   index-{}\n".format(suffix)
    rst += "   index-all\n"

    write_if_updated("index.rst", rst)


def write_index_page(syms, title, suffix, text):
    # Writes an index page for the Symbols in 'syms' to 'index-<suffix>.rst'
    # (or index.rst if 'suffix' is None). 'title', 'suffix', and 'text' are
    # also used for to generate the index page header. See index_page_header().

    rst = index_page_header(title, suffix, text)

    rst += """
Configuration symbols
*********************

.. list-table:: Alphabetized Index of Configuration Options
   :header-rows: 1

   * - Kconfig Symbol
     - Description
"""

    for sym in sorted(syms, key=attrgetter("name")):
        # Add an index entry for the symbol that links to its page. Also list
        # its prompt(s), if any. (A symbol can have multiple prompts if it has
        # multiple definitions.)
        rst += "   * - :option:`CONFIG_{}`\n     - {}\n".format(
            sym.name, " / ".join(node.prompt[0] for node in sym.nodes
                                 if node.prompt))

    if suffix is None:
        fname = "index.rst"
    else:
        fname = "index-{}.rst".format(suffix)

    write_if_updated(fname, rst)


def index_page_header(title, link, description):
    # write_index_page() helper. Returns the RST for the beginning of a symbol
    # index page.
    #
    # title:
    #   String used for the page title, as '<title> Configuration Options'. If
    #   None, just 'Configuration Options' is used as the title.
    #
    # link:
    #   String used for link target, as 'configuration_options_<link>'. If
    #   None, the link will be 'configuration_options'.
    #
    # description:
    #   RST put into an Overview section at the beginning of the page

    if title is None:
        title = "Configuration Options"
    else:
        title = title + " Configuration Options"

    if link is None:
        link = "configuration_options"
    else:
        link = "configuration_options_" + link

    title += "\n" + len(title)*"="

    return """\
.. _{}:

{}

Overview
********

{}

This documentation is generated automatically from the :file:`Kconfig` files by
the :file:`{}` script. Click on symbols for more information.
""".format(link, title, description, os.path.basename(__file__))


DEFAULT_INDEX_DESCRIPTION = """\
:file:`Kconfig` files describe build-time configuration options (called symbols
in Kconfig-speak), how they're grouped into menus and sub-menus, and
dependencies between them that determine what configurations are valid.

:file:`Kconfig` files appear throughout the directory tree. For example,
:file:`subsys/power/Kconfig` defines power-related options.
"""


def desc_from_file(fname):
    # Helper for loading files with descriptions for index pages. Returns
    # DEFAULT_INDEX_DESCRIPTION if 'fname' is None, and the contents of the
    # file otherwise.

    if fname is None:
        return DEFAULT_INDEX_DESCRIPTION

    try:
        with open(fname, "r", encoding="utf-8") as f:
            return f.read()
    except OSError as e:
        sys.exit("error: failed to open index description file '{}': {}"
                 .format(fname, e))


def write_sym_pages():
    # Generates all symbol and choice pages

    for sym in kconf.unique_defined_syms:
        write_sym_page(sym)

    for choice in kconf.unique_choices:
        write_choice_page(choice)


def write_dummy_syms_page():
    # Writes a dummy page that just has targets for all symbol links so that
    # they can be referenced from elsewhere in the documentation, to speed up
    # builds when we don't need the Kconfig symbol documentation

    rst = ":orphan:\n\nDummy symbols page for turbo mode.\n\n"
    for sym in kconf.unique_defined_syms:
        rst += ".. option:: CONFIG_{}\n".format(sym.name)

    write_if_updated("dummy-syms.rst", rst)


def write_sym_page(sym):
    # Writes documentation for 'sym' to <out_dir>/CONFIG_<sym.name>.rst

    write_if_updated("CONFIG_{}.rst".format(sym.name),
                     sym_header_rst(sym) +
                     help_rst(sym) +
                     direct_deps_rst(sym) +
                     defaults_rst(sym) +
                     select_imply_rst(sym) +
                     selecting_implying_rst(sym) +
                     kconfig_definition_rst(sym))


def write_choice_page(choice):
    # Writes documentation for 'choice' to <out_dir>/choice_<n>.rst, where <n>
    # is the index of the choice in kconf.choices (where choices appear in the
    # same order as in the Kconfig files)

    write_if_updated(choice_id(choice) + ".rst",
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
           "*(Includes any dependencies from ifs and menus.)*\n\n" \
           .format(expr_str(sc.direct_dep))


def defaults_rst(sc):
    # Returns RST that lists the 'default' properties of 'sc' (symbol or
    # choice)

    if isinstance(sc, kconfiglib.Symbol) and sc.choice:
        # 'default's on choice symbols have no effect (and generate a warning).
        # The implicit value hint below would be misleading as well.
        return ""

    heading = "Default"
    if len(sc.defaults) != 1:
        heading += "s"
    rst = "{}\n{}\n\n".format(heading, len(heading)*"=")

    if sc.defaults:
        for value, cond in sc.orig_defaults:
            rst += "- " + expr_str(value)
            if cond is not sc.kconfig.y:
                rst += " if " + expr_str(cond)
            rst += "\n"
    else:
        rst += "No defaults. Implicitly defaults to "
        if isinstance(sc, kconfiglib.Choice):
            rst += "the first (visible) choice option.\n"
        elif sc.orig_type in (kconfiglib.BOOL, kconfiglib.TRISTATE):
            rst += "``n``.\n"
        else:
            # This is accurate even for int/hex symbols, though an active
            # 'range' might clamp the value (which is then treated as zero)
            rst += "the empty string.\n"

    return rst + "\n"


def choice_syms_rst(choice):
    # Returns RST that lists the symbols contained in the choice

    if not choice.syms:
        return ""

    rst = "Choice options\n" \
          "==============\n\n"

    for sym in choice.syms:
        # Generates a link
        rst += "- {}\n".format(expr_str(sym))

    return rst + "\n"


def select_imply_rst(sym):
    # Returns RST that lists the symbols 'select'ed or 'imply'd by the symbol

    rst = ""

    def add_select_imply_rst(type_str, lst):
        # Adds RST that lists the selects/implies from 'lst', which holds
        # (<symbol>, <condition>) tuples, if any. Also adds a heading derived
        # from 'type_str' if there any selects/implies.

        nonlocal rst

        if lst:
            heading = "Symbols {} by this symbol".format(type_str)
            rst += "{}\n{}\n\n".format(heading, len(heading)*"=")

            for select, cond in lst:
                rst += "- " + rst_link(select)
                if cond is not sym.kconfig.y:
                    rst += " if " + expr_str(cond)
                rst += "\n"

            rst += "\n"

    add_select_imply_rst("selected", sym.orig_selects)
    add_select_imply_rst("implied", sym.orig_implies)

    return rst


def selecting_implying_rst(sym):
    # Returns RST that lists the symbols that are 'select'ing or 'imply'ing the
    # symbol

    rst = ""

    def add_selecting_implying_rst(type_str, expr):
        # Writes a link for each symbol that selects the symbol (if 'expr' is
        # sym.rev_dep) or each symbol that imply's the symbol (if 'expr' is
        # sym.weak_rev_dep). Also adds a heading at the top derived from
        # type_str ("select"/"imply"), if there are any selecting/implying
        # symbols.

        nonlocal rst

        if expr is not sym.kconfig.n:
            heading = "Symbols that {} this symbol".format(type_str)
            rst += "{}\n{}\n\n".format(heading, len(heading)*"=")

            # The reverse dependencies from each select/imply are ORed together
            for select in kconfiglib.split_expr(expr, kconfiglib.OR):
                # - 'select/imply A if B' turns into A && B
                # - 'select/imply A' just turns into A
                #
                # In both cases, we can split on AND and pick the first
                # operand.

                rst += "- {}\n".format(rst_link(
                    kconfiglib.split_expr(select, kconfiglib.AND)[0]))

            rst += "\n"

    add_selecting_implying_rst("select", sym.rev_dep)
    add_selecting_implying_rst("imply", sym.weak_rev_dep)

    return rst


def kconfig_definition_rst(sc):
    # Returns RST that lists the Kconfig definition location, include path,
    # menu path, and Kconfig definition for each node (definition location) of
    # 'sc' (symbol or choice)

    # Fancy Unicode arrow. Added in '93, so ought to be pretty safe.
    arrow = " \N{RIGHTWARDS ARROW} "

    def include_path(node):
        if not node.include_path:
            # In the top-level Kconfig file
            return ""

        return "Included via {}\n\n".format(
            arrow.join("``{}:{}``".format(strip_module_path(filename), linenr)
                       for filename, linenr in node.include_path))

    def menu_path(node):
        path = ""

        while node.parent is not node.kconfig.top_node:
            node = node.parent

            # Promptless choices can show up as parents, e.g. when people
            # define choices in multiple locations to add symbols. Use
            # standard_sc_expr_str() to show them. That way they show up as
            # '<choice (name if any)>'.
            path = arrow + \
                   (node.prompt[0] if node.prompt else
                    kconfiglib.standard_sc_expr_str(node.item)) + \
                   path

        return "(Top)" + path

    heading = "Kconfig definition"
    if len(sc.nodes) > 1: heading += "s"
    rst = "{}\n{}\n\n".format(heading, len(heading)*"=")

    rst += ".. highlight:: kconfig"

    for node in sc.nodes:
        rst += "\n\n" \
               "At ``{}:{}``\n\n" \
               "{}" \
               "Menu path: {}\n\n" \
               ".. parsed-literal::\n\n{}" \
               .format(strip_module_path(node.filename), node.linenr,
                       include_path(node), menu_path(node),
                       textwrap.indent(node.custom_str(rst_link), 4*" "))

        # Not the last node?
        if node is not sc.nodes[-1]:
            # Add a horizontal line between multiple definitions
            rst += "\n\n----"

    rst += "\n\n*(The 'depends on' condition includes propagated " \
           "dependencies from ifs and menus.)*"

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
    return "choice_{}".format(choice.kconfig.unique_choices.index(choice))


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


def path2module(path):
    # Returns the name of module that 'path' appears in, or None if it does not
    # appear in a module. 'path' is assumed to be relative to 'srctree'.

    # Have to be careful here so that e.g. foo/barbaz/qaz isn't assumed to be
    # part of a module with path foo/bar/. Play it safe with pathlib.

    abspath = pathlib.Path(kconf.srctree).joinpath(path).resolve()
    for name, _, mod_path, _ in modules:
        try:
            abspath.relative_to(mod_path)
        except ValueError:
            # Not within the module
            continue

        return name

    return None


def strip_module_path(path):
    # If 'path' is within a module, strips the module path from it, and adds a
    # '<module name>/' prefix. Otherwise, returns 'path' unchanged. 'path' is
    # assumed to be relative to 'srctree'.

    if strip_module_paths:
        abspath = pathlib.Path(kconf.srctree).joinpath(path).resolve()
        for title, _, mod_path, _ in modules:
            try:
                relpath = abspath.relative_to(mod_path)
            except ValueError:
                # Not within the module
                continue

            return "<{}>{}{}".format(title, os.path.sep, relpath)

    return path


def write_if_updated(filename, s):
    # Writes 's' as the contents of <out_dir>/<filename>, but only if it
    # differs from the current contents of the file. This avoids unnecessary
    # timestamp updates, which trigger documentation rebuilds.

    path = os.path.join(out_dir, filename)

    try:
        with open(path, "r", encoding="utf-8") as f:
            if s == f.read():
                return
    except OSError as e:
        if e.errno != errno.ENOENT:
            raise

    with open(path, "w", encoding="utf-8") as f:
        f.write(s)


if __name__ == "__main__":
    main()
