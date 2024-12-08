"""
Kconfig Extension
#################

Copyright (c) 2022 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This extension adds a new domain (``kconfig``) for the Kconfig language. Unlike
many other domains, the Kconfig options are not rendered by Sphinx directly but
on the client side using a database built by the extension. A special directive
``.. kconfig:search::`` can be inserted on any page to render a search box that
allows to browse the database. References to Kconfig options can be created by
using the ``:kconfig:option:`` role. Kconfig options behave as regular domain
objects, so they can also be referenced by other projects using Intersphinx.

Options
=======

- kconfig_generate_db: Set to True if you want to generate the Kconfig database.
  This is only required if you want to use the ``.. kconfig:search::``
  directive, not if you just need support for Kconfig domain (e.g. when using
  Intersphinx in another project). Defaults to False.
- kconfig_ext_paths: A list of base paths where to search for external modules
  Kconfig files when they use ``kconfig-ext: True``. The extension will look for
  ${BASE_PATH}/modules/${MODULE_NAME}/Kconfig.
"""

import argparse
import json
import os
import re
import sys
from collections.abc import Iterable
from itertools import chain
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any

from docutils import nodes
from sphinx.addnodes import pending_xref
from sphinx.application import Sphinx
from sphinx.builders import Builder
from sphinx.domains import Domain, ObjType
from sphinx.environment import BuildEnvironment
from sphinx.errors import ExtensionError
from sphinx.roles import XRefRole
from sphinx.util.display import progress_message
from sphinx.util.docutils import SphinxDirective
from sphinx.util.nodes import make_refnode

__version__ = "0.1.0"


sys.path.insert(0, str(Path(__file__).parents[4] / "scripts"))
sys.path.insert(0, str(Path(__file__).parents[4] / "scripts/kconfig"))

import kconfiglib
import list_boards
import list_hardware
import zephyr_module

RESOURCES_DIR = Path(__file__).parent / "static"
ZEPHYR_BASE = Path(__file__).parents[4]


def kconfig_load(app: Sphinx) -> tuple[kconfiglib.Kconfig, dict[str, str]]:
    """Load Kconfig"""
    with TemporaryDirectory() as td:
        modules = zephyr_module.parse_modules(ZEPHYR_BASE)

        # generate Kconfig.modules file
        kconfig = ""
        for module in modules:
            kconfig += zephyr_module.process_kconfig(module.project, module.meta)

        with open(Path(td) / "Kconfig.modules", "w") as f:
            f.write(kconfig)

        # generate dummy Kconfig.dts file
        kconfig = ""

        with open(Path(td) / "Kconfig.dts", "w") as f:
            f.write(kconfig)

        (Path(td) / 'soc').mkdir(exist_ok=True)
        root_args = argparse.Namespace(**{'soc_roots': [Path(ZEPHYR_BASE)]})
        v2_systems = list_hardware.find_v2_systems(root_args)

        soc_folders = {soc.folder[0] for soc in v2_systems.get_socs()}
        with open(Path(td) / "soc" / "Kconfig.defconfig", "w") as f:
            f.write('')

        with open(Path(td) / "soc" / "Kconfig.soc", "w") as f:
            for folder in soc_folders:
                f.write('source "' + (Path(folder) / 'Kconfig.soc').as_posix() + '"\n')

        with open(Path(td) / "soc" / "Kconfig", "w") as f:
            for folder in soc_folders:
                f.write('osource "' + (Path(folder) / 'Kconfig').as_posix() + '"\n')

        (Path(td) / 'arch').mkdir(exist_ok=True)
        root_args = argparse.Namespace(**{'arch_roots': [Path(ZEPHYR_BASE)], 'arch': None})
        v2_archs = list_hardware.find_v2_archs(root_args)
        kconfig = ""
        for arch in v2_archs['archs']:
            kconfig += 'source "' + (Path(arch['path']) / 'Kconfig').as_posix() + '"\n'
        with open(Path(td) / "arch" / "Kconfig", "w") as f:
            f.write(kconfig)

        (Path(td) / 'boards').mkdir(exist_ok=True)
        root_args = argparse.Namespace(**{'board_roots': [Path(ZEPHYR_BASE)],
                                          'soc_roots': [Path(ZEPHYR_BASE)], 'board': None,
                                          'board_dir': []})
        v2_boards = list_boards.find_v2_boards(root_args).values()

        with open(Path(td) / "boards" / "Kconfig.boards", "w") as f:
            for board in v2_boards:
                board_str = 'BOARD_' + re.sub(r"[^a-zA-Z0-9_]", "_", board.name).upper()
                f.write('config  ' + board_str + '\n')
                f.write('\t bool\n')
                for qualifier in list_boards.board_v2_qualifiers(board):
                    board_str = 'BOARD_' + re.sub(r"[^a-zA-Z0-9_]", "_", qualifier).upper()
                    f.write('config  ' + board_str + '\n')
                    f.write('\t bool\n')
                f.write('source "' + (board.dir / ('Kconfig.' + board.name)).as_posix() + '"\n\n')

        # base environment
        os.environ["ZEPHYR_BASE"] = str(ZEPHYR_BASE)
        os.environ["srctree"] = str(ZEPHYR_BASE)  # noqa: SIM112
        os.environ["KCONFIG_DOC_MODE"] = "1"
        os.environ["KCONFIG_BINARY_DIR"] = td

        # include all archs and boards
        os.environ["ARCH_DIR"] = "arch"
        os.environ["ARCH"] = "[!v][!2]*"
        os.environ["HWM_SCHEME"] = "v2"

        os.environ["BOARD"] = "boards"
        os.environ["KCONFIG_BOARD_DIR"] = str(Path(td) / "boards")

        # insert external Kconfigs to the environment
        module_paths = dict()
        for module in modules:
            name = module.meta["name"]
            name_var = module.meta["name-sanitized"].upper()
            module_paths[name] = module.project

            build_conf = module.meta.get("build")
            if not build_conf:
                continue

            if build_conf.get("kconfig"):
                kconfig = Path(module.project) / build_conf["kconfig"]
                os.environ[f"ZEPHYR_{name_var}_KCONFIG"] = str(kconfig)
            elif build_conf.get("kconfig-ext"):
                for path in app.config.kconfig_ext_paths:
                    kconfig = Path(path) / "modules" / name / "Kconfig"
                    if kconfig.exists():
                        os.environ[f"ZEPHYR_{name_var}_KCONFIG"] = str(kconfig)

        return kconfiglib.Kconfig(ZEPHYR_BASE / "Kconfig"), module_paths


class KconfigSearchNode(nodes.Element):
    @staticmethod
    def html():
        return '<div id="__kconfig-search"></div>'


def kconfig_search_visit_html(self, node: nodes.Node) -> None:
    self.body.append(node.html())
    raise nodes.SkipNode


def kconfig_search_visit_latex(self, node: nodes.Node) -> None:
    self.body.append("Kconfig search is only available on HTML output")
    raise nodes.SkipNode


class KconfigSearch(SphinxDirective):
    """Kconfig search directive"""

    has_content = False

    def run(self):
        if not self.config.kconfig_generate_db:
            raise ExtensionError(
                "Kconfig search directive can not be used without database"
            )

        if "kconfig_search_inserted" in self.env.temp_data:
            raise ExtensionError("Kconfig search directive can only be used once")

        self.env.temp_data["kconfig_search_inserted"] = True

        # register all options to the domain at this point, so that they all
        # resolve to the page where the kconfig:search directive is inserted
        domain = self.env.get_domain("kconfig")
        unique = set({option["name"] for option in self.env.kconfig_db})
        for option in unique:
            domain.add_option(option)

        return [KconfigSearchNode()]


class _FindKconfigSearchDirectiveVisitor(nodes.NodeVisitor):
    def __init__(self, document):
        super().__init__(document)
        self._found = False

    def unknown_visit(self, node: nodes.Node) -> None:
        if self._found:
            return

        self._found = isinstance(node, KconfigSearchNode)

    @property
    def found_kconfig_search_directive(self) -> bool:
        return self._found


class KconfigDomain(Domain):
    """Kconfig domain"""

    name = "kconfig"
    label = "Kconfig"
    object_types = {"option": ObjType("option", "option")}
    roles = {"option": XRefRole()}
    directives = {"search": KconfigSearch}
    initial_data: dict[str, Any] = {"options": set()}

    def get_objects(self) -> Iterable[tuple[str, str, str, str, str, int]]:
        yield from self.data["options"]

    def merge_domaindata(self, docnames: list[str], otherdata: dict) -> None:
        self.data["options"].update(otherdata["options"])

    def resolve_xref(
        self,
        env: BuildEnvironment,
        fromdocname: str,
        builder: Builder,
        typ: str,
        target: str,
        node: pending_xref,
        contnode: nodes.Element,
    ) -> nodes.Element | None:
        match = [
            (docname, anchor)
            for name, _, _, docname, anchor, _ in self.get_objects()
            if name == target
        ]

        if match:
            todocname, anchor = match[0]

            return make_refnode(
                builder, fromdocname, todocname, anchor, contnode, anchor
            )
        else:
            return None

    def add_option(self, option):
        """Register a new Kconfig option to the domain."""

        self.data["options"].add(
            (option, option, "option", self.env.docname, option, 1)
        )


def sc_fmt(sc):
    if isinstance(sc, kconfiglib.Symbol):
        if sc.nodes:
            return f'<a href="#CONFIG_{sc.name}">CONFIG_{sc.name}</a>'
    elif isinstance(sc, kconfiglib.Choice):
        if not sc.name:
            return "&ltchoice&gt"
        return f'&ltchoice <a href="#CONFIG_{sc.name}">CONFIG_{sc.name}</a>&gt'

    return kconfiglib.standard_sc_expr_str(sc)


def kconfig_build_resources(app: Sphinx) -> None:
    """Build the Kconfig database and install HTML resources."""

    if not app.config.kconfig_generate_db:
        return

    with progress_message("Building Kconfig database..."):
        kconfig, module_paths = kconfig_load(app)
        db = list()

        for sc in sorted(
            chain(kconfig.unique_defined_syms, kconfig.unique_choices),
            key=lambda sc: sc.name if sc.name else "",
        ):
            # skip nameless symbols
            if not sc.name:
                continue

            # store alternative defaults (from defconfig files)
            alt_defaults = list()
            for node in sc.nodes:
                if "defconfig" not in node.filename:
                    continue

                for value, cond in node.orig_defaults:
                    fmt = kconfiglib.expr_str(value, sc_fmt)
                    if cond is not sc.kconfig.y:
                        fmt += f" if {kconfiglib.expr_str(cond, sc_fmt)}"
                    alt_defaults.append([fmt, node.filename])

            # build list of symbols that select/imply the current one
            # note: all reverse dependencies are ORed together, and conditionals
            # (e.g. select/imply A if B) turns into A && B. So we first split
            # by OR to include all entries, and we split each one by AND to just
            # take the first entry.
            selected_by = list()
            if isinstance(sc, kconfiglib.Symbol) and sc.rev_dep != sc.kconfig.n:
                for select in kconfiglib.split_expr(sc.rev_dep, kconfiglib.OR):
                    sym = kconfiglib.split_expr(select, kconfiglib.AND)[0]
                    selected_by.append(f"CONFIG_{sym.name}")

            implied_by = list()
            if isinstance(sc, kconfiglib.Symbol) and sc.weak_rev_dep != sc.kconfig.n:
                for select in kconfiglib.split_expr(sc.weak_rev_dep, kconfiglib.OR):
                    sym = kconfiglib.split_expr(select, kconfiglib.AND)[0]
                    implied_by.append(f"CONFIG_{sym.name}")

            # only process nodes with prompt or help
            nodes = [node for node in sc.nodes if node.prompt or node.help]

            inserted_paths = list()
            for node in nodes:
                # avoid duplicate symbols by forcing unique paths. this can
                # happen due to dependencies on 0, a trick used by some modules
                path = f"{node.filename}:{node.linenr}"
                if path in inserted_paths:
                    continue
                inserted_paths.append(path)

                dependencies = None
                if node.dep is not sc.kconfig.y:
                    dependencies = kconfiglib.expr_str(node.dep, sc_fmt)

                defaults = list()
                for value, cond in node.orig_defaults:
                    fmt = kconfiglib.expr_str(value, sc_fmt)
                    if cond is not sc.kconfig.y:
                        fmt += f" if {kconfiglib.expr_str(cond, sc_fmt)}"
                    defaults.append(fmt)

                selects = list()
                for value, cond in node.orig_selects:
                    fmt = kconfiglib.expr_str(value, sc_fmt)
                    if cond is not sc.kconfig.y:
                        fmt += f" if {kconfiglib.expr_str(cond, sc_fmt)}"
                    selects.append(fmt)

                implies = list()
                for value, cond in node.orig_implies:
                    fmt = kconfiglib.expr_str(value, sc_fmt)
                    if cond is not sc.kconfig.y:
                        fmt += f" if {kconfiglib.expr_str(cond, sc_fmt)}"
                    implies.append(fmt)

                ranges = list()
                for min, max, cond in node.orig_ranges:
                    fmt = (
                        f"[{kconfiglib.expr_str(min, sc_fmt)}, "
                        f"{kconfiglib.expr_str(max, sc_fmt)}]"
                    )
                    if cond is not sc.kconfig.y:
                        fmt += f" if {kconfiglib.expr_str(cond, sc_fmt)}"
                    ranges.append(fmt)

                choices = list()
                if isinstance(sc, kconfiglib.Choice):
                    for sym in sc.syms:
                        choices.append(kconfiglib.expr_str(sym, sc_fmt))

                menupath = ""
                iternode = node
                while iternode.parent is not iternode.kconfig.top_node:
                    iternode = iternode.parent
                    if iternode.prompt:
                        title = iternode.prompt[0]
                    else:
                        title = kconfiglib.standard_sc_expr_str(iternode.item)
                    menupath = f" > {title}" + menupath

                menupath = "(Top)" + menupath

                filename = node.filename
                for name, path in module_paths.items():
                    path += "/"
                    if node.filename.startswith(path):
                        filename = node.filename.replace(path, f"<module:{name}>/")
                        break

                db.append(
                    {
                        "name": f"CONFIG_{sc.name}",
                        "prompt": node.prompt[0] if node.prompt else None,
                        "type": kconfiglib.TYPE_TO_STR[sc.type],
                        "help": node.help,
                        "dependencies": dependencies,
                        "defaults": defaults,
                        "alt_defaults": alt_defaults,
                        "selects": selects,
                        "selected_by": selected_by,
                        "implies": implies,
                        "implied_by": implied_by,
                        "ranges": ranges,
                        "choices": choices,
                        "filename": filename,
                        "linenr": node.linenr,
                        "menupath": menupath,
                    }
                )

        app.env.kconfig_db = db  # type: ignore

        outdir = Path(app.outdir) / "kconfig"
        outdir.mkdir(exist_ok=True)

        kconfig_db_file = outdir / "kconfig.json"

        with open(kconfig_db_file, "w") as f:
            json.dump(db, f)

    app.config.html_extra_path.append(kconfig_db_file.as_posix())
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())


def kconfig_install(
    app: Sphinx,
    pagename: str,
    templatename: str,
    context: dict,
    doctree: nodes.Node | None,
) -> None:
    """Install the Kconfig library files on pages that require it."""
    if (
        not app.config.kconfig_generate_db
        or app.builder.format != "html"
        or not doctree
    ):
        return

    visitor = _FindKconfigSearchDirectiveVisitor(doctree)
    doctree.walk(visitor)
    if visitor.found_kconfig_search_directive:
        app.add_css_file("kconfig.css")
        app.add_js_file("kconfig.mjs", type="module")


def setup(app: Sphinx):
    app.add_config_value("kconfig_generate_db", False, "env")
    app.add_config_value("kconfig_ext_paths", [], "env")

    app.add_node(
        KconfigSearchNode,
        html=(kconfig_search_visit_html, None),
        latex=(kconfig_search_visit_latex, None),
    )

    app.add_domain(KconfigDomain)

    app.connect("builder-inited", kconfig_build_resources)
    app.connect("html-page-context", kconfig_install)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
