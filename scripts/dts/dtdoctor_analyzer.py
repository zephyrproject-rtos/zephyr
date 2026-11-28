#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""
A script to help diagnose build errors related to Devicetree.

To use this script as a standalone tool, provide the path to an edt.pickle file
(e.g ./build/zephyr/edt.pickle) and a symbol that appeared in the build error
message (e.g. __device_dts_ord_123, or __device_dts_ord_DT_N_ALIAS_led0_ORD when
the node identifier did not resolve to a node).

Example usage:

./scripts/dts/dtdoctor_analyzer.py \\
    --edt-pickle ./build/zephyr/edt.pickle \\
    --symbol __device_dts_ord_123

"""

import argparse
import difflib
import os
import pickle
import re
import sys
import textwrap
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "python-devicetree" / "src"))
sys.path.insert(0, str(Path(__file__).parents[1] / "kconfig"))

import kconfiglib
from devicetree import edtlib
from gen_defines import node_z_path_id, str2ident
from tabulate import tabulate


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--edt-pickle",
        required=True,
        help="path to edt.pickle file corresponding to the build to analyze",
    )
    parser.add_argument(
        "--symbol", required=True, help="symbol for which to obtain troubleshooting information"
    )
    return parser.parse_args()


def load_edt(path: str) -> edtlib.EDT:
    with open(path, "rb") as f:
        return pickle.load(f)


def setup_kconfig() -> kconfiglib.Kconfig | None:
    zephyr_base = os.environ.get("ZEPHYR_BASE")
    if not zephyr_base:
        return None
    return kconfiglib.Kconfig(os.path.join(zephyr_base, "Kconfig"), warn=False)


def format_node(node: edtlib.Node) -> str:
    return f"{node.labels[0]}: {node.path}" if node.labels else node.path


def find_kconfig_deps(kconf: kconfiglib.Kconfig, dt_has_symbol: str) -> set[str]:
    """
    Find all Kconfig symbols that depend on the provided DT_HAS symbol.
    """
    prefix = os.environ.get("CONFIG_", "CONFIG_")
    target = f"{prefix}{dt_has_symbol}"
    # Word-boundary match so e.g. DT_HAS_FOO_ENABLED doesn't match DT_HAS_FOO_ENABLED_EXT
    target_re = re.compile(rf"(?<!\w){re.escape(target)}(?!\w)")
    deps = set()

    def expr_to_str(expr):
        return kconfiglib.expr_str(
            expr,
            lambda sc: f"{prefix}{sc.name}" if hasattr(sc, 'name') and sc.name else str(sc),
        )

    def collect_syms(expr):
        # Recursively collect all symbol names in the expression tree except the target
        for item in kconfiglib.expr_items(expr):
            if not isinstance(item, kconfiglib.Symbol):
                continue
            sym_name = f"{prefix}{item.name}"
            if sym_name != target:
                deps.add(sym_name)

    for sym in kconf.unique_defined_syms:
        for node in sym.nodes:
            # Check dependencies
            if node.dep is not None and target_re.search(expr_to_str(node.dep)):
                collect_syms(node.dep)

            # A symbol whose select/imply is conditioned on the DT_HAS symbol is itself
            # an option worth enabling
            for attr in ["orig_selects", "orig_implies"]:
                for _, cond in getattr(node, attr, []) or []:
                    if cond is not None and target_re.search(expr_to_str(cond)):
                        deps.add(f"{prefix}{sym.name}")
                        collect_syms(cond)

    return deps


def handle_enabled_node(node: edtlib.Node) -> list[str]:
    """
    Handle diagnosis for an enabled DT node (linker error, one or more Kconfigs might be gating
    the device driver).
    """
    lines = [f"'{format_node(node)}' is enabled but no driver appears to be available for it.\n"]

    compats = list(getattr(node, "compats", []))
    kconf = setup_kconfig() if compats else None
    if not compats:
        lines.append("Could not determine compatible; check driver Kconfig manually.")
    elif not kconf:
        lines.append("ZEPHYR_BASE is not set; check driver Kconfig manually.")
    else:
        deps = set()
        for compat in compats:
            dt_has = f"DT_HAS_{edtlib.str_as_token(compat.upper())}_ENABLED"
            deps.update(find_kconfig_deps(kconf, dt_has))

        if deps:
            lines.append("Try enabling these Kconfig options:\n")
            lines.extend(f" - {dep}=y" for dep in sorted(deps))

    return lines


def handle_disabled_node(node: edtlib.Node) -> list[str]:
    """
    Handle diagnosis for a disabled DT node.
    """
    edt = node.edt
    status_prop = node._node.props.get('status')
    lines = [f"'{format_node(node)}' is disabled in {status_prop.filename}:{status_prop.lineno}"]

    # Show dependency
    users = getattr(node, "required_by", [])
    if users:
        lines.append("The following nodes depend on it:")
        lines.extend(f" - {u.path}" for u in users)

    # Show chosen/alias references
    chosen_refs = [name for name, n in edt.chosen_nodes.items() if n is node]
    alias_refs = node.aliases

    if chosen_refs or alias_refs:
        lines.append("")

    if chosen_refs:
        lines.append(
            "It is referenced as a \"chosen\" in "
            f"""{', '.join([f"'{ref}'" for ref in sorted(chosen_refs)])}"""
        )
    if alias_refs:
        lines.append(
            "It is referenced by the following aliases: "
            f"""{', '.join([f"'{ref}'" for ref in sorted(alias_refs)])}"""
        )

    lines.append("\nTry enabling the node by setting its 'status' property to 'okay'.")

    return lines


def wrap_list(intro: str, items: list[str]) -> list[str]:
    # Break at spaces only, so names such as 'zephyr,bt-mon-uart' stay whole
    return textwrap.wrap(
        f"{intro} {', '.join(items)}",
        width=76,
        subsequent_indent="  ",
        break_on_hyphens=False,
        break_long_words=False,
    )


def handle_missing_alias(edt: edtlib.EDT, token: str) -> list[str]:
    # Alias names use '-' where their DT_ALIAS() token has '_'
    name = token.replace("_", "-")
    lines = [f"DT_ALIAS({token}) refers to the devicetree alias '{name}', which is not defined."]

    target = "<node-label>"
    label = next((label for label in edt.label2node if str2ident(label) == token), None)
    if label:
        lines.append(
            f"\nThe node {edt.label2node[label].path} has the label '{label}'.\n"
            f"If that is the node you meant, try DT_NODELABEL({token}) instead."
        )
        target = label

    aliases = sorted({alias for node in edt.nodes for alias in node.aliases})
    lines.append("")
    lines.extend(wrap_list("Defined aliases:", aliases) if aliases else ["No aliases are defined."])

    lines.append("\nTry defining the alias in the board devicetree or an overlay, for example:\n")
    lines.extend(["/ {", "    aliases {", f"        {name} = &{target};", "    };", "};"])
    return lines


def handle_missing_nodelabel(edt: edtlib.EDT, token: str) -> list[str]:
    lines = [f"DT_NODELABEL({token}) refers to the node label '{token}', which no node has."]

    aliased = next(((a, n) for n in edt.nodes for a in n.aliases if str2ident(a) == token), None)
    if aliased:
        alias, node = aliased
        lines.append(
            f"\nThe alias '{alias}' points to {node.path}.\n"
            f"If that is the node you meant, try DT_ALIAS({token}) instead."
        )

    similar = difflib.get_close_matches(token, list(edt.label2node), n=5)
    if similar:
        lines.append("")
        lines.extend(wrap_list("Similar node labels:", similar))

    lines.append("\nCheck the spelling against the labels in the board devicetree.")
    return lines


def handle_missing_chosen(edt: edtlib.EDT, token: str) -> list[str]:
    lines = [f"DT_CHOSEN({token}) refers to a /chosen property that is not defined."]

    chosen = sorted(edt.chosen_nodes)
    lines.append("")
    lines.extend(
        wrap_list("Defined /chosen properties:", chosen)
        if chosen
        else ["No /chosen properties are defined."]
    )

    # Zephyr's own chosen properties are all spelled "zephyr,<name-with-dashes>"
    prop = "<property>"
    if token.startswith("zephyr_"):
        prop = "zephyr," + token.removeprefix("zephyr_").replace("_", "-")
    lines.append("\nTry setting it in the board devicetree or an overlay, for example:\n")
    lines.extend(["/ {", "    chosen {", f"        {prop} = &<node-label>;", "    };", "};"])
    return lines


def handle_missing_instance(edt: edtlib.EDT, token: str) -> list[str]:
    m = re.fullmatch(r"(\d+)_([a-z0-9_]+)", token)
    if not m:
        return []
    inst, compat_token = int(m.group(1)), m.group(2)

    compat = next((c for c in edt.compat2nodes if str2ident(c) == compat_token), None)
    if compat is None:
        return [
            f"DT_INST({inst}, {compat_token}) refers to a node with compatible "
            f"'{compat_token}', but no devicetree node has it.",
            "",
            "Check the compatible (DT_DRV_COMPAT in a driver). If it is correct, the",
            "board may not have this hardware, or its devicetree does not describe it.",
        ]

    # Instance numbers cover all nodes with the compatible, enabled ones first
    nodes = edt.compat2nodes[compat]
    if inst < len(nodes):
        return []
    lines = [
        f"DT_INST({inst}, {compat_token}) refers to instance {inst} of '{compat}', but only "
        f"{len(nodes)} node(s) have this compatible:"
    ]
    lines.extend(f" - {i}: {format_node(n)} (status '{n.status}')" for i, n in enumerate(nodes))
    return lines


def handle_invalid_node() -> list[str]:
    return [
        "The node identifier is DT_INVALID_NODE, which DT_COMPAT_GET_ANY_STATUS_OKAY()",
        "gives when no enabled node has the requested compatible.",
        "",
        "Check that a node with this compatible exists and has its 'status' property",
        "set to 'okay'.",
    ]


def describe_node(node: edtlib.Node) -> str:
    if not node.aliases:
        return f"'{format_node(node)}'"
    kind = "alias" if len(node.aliases) == 1 else "aliases"
    return f"'{format_node(node)}' ({kind} {', '.join(repr(a) for a in node.aliases)})"


def handle_missing_property(node: edtlib.Node, token: str) -> list[str]:
    # All properties set in the devicetree, not only those the node's binding declares
    names = {str2ident(name): name for name in node._node.props}

    if token not in names:
        lines = [f"{describe_node(node)} has no '{token.replace('_', '-')}' property."]
        if names:
            lines.append("")
            lines.extend(wrap_list("Properties set on this node:", sorted(names.values())))
        return lines

    # Property macros come from the binding (edtlib rejects properties a binding does
    # not declare), so a node without one has none
    if not node.binding_path:
        return [
            f"{describe_node(node)} has a '{names[token]}' property, but the node has no",
            "binding, so no devicetree macros are generated for it.",
        ]

    return []


def handle_missing_child(node: edtlib.Node, token: str) -> list[str]:
    lines = [f"{describe_node(node)} has no child node matching '{token}'."]

    children = {str2ident(name): name for name in node.children}
    similar = difflib.get_close_matches(token, list(children), n=5)
    if similar:
        lines.append("")
        lines.extend(wrap_list("Similar child nodes:", [children[s] for s in similar]))
    return lines


def handle_unresolved_path(edt: edtlib.EDT, ident: str) -> list[str]:
    # Find the deepest node whose path identifier is a prefix of 'ident'. What follows it
    # is a property (_P_<prop>), a child (_S_<name>) or another suffix, each starting with
    # an uppercase letter unlike the lowercase node names.
    node, rest = None, ""
    for n in edt.nodes:
        pid = f"DT_{node_z_path_id(n)}"
        tail = ident[len(pid) :]
        if (
            ident.startswith(pid)
            and re.match(r"_[A-Z]", tail)
            and (node is None or len(tail) < len(rest))
        ):
            node, rest = n, tail
    if node is None:
        return []

    m = re.match(r"_P_([a-z0-9_]+?)(?=_[A-Z])", rest)
    if m:
        return handle_missing_property(node, m.group(1))

    m = re.match(r"_S_([a-z0-9_]+?)(?=_[A-Z])", rest)
    if m:
        return handle_missing_child(node, m.group(1))

    return []


def handle_unresolved_node_id(edt: edtlib.EDT, ident: str) -> list[str]:
    """
    Handle diagnosis for a node identifier that does not resolve to a node. The compiler
    then reports __device_dts_ord_<identifier>_ORD instead of an ordinal symbol, e.g.
    __device_dts_ord_DT_N_ALIAS_led0_P_gpios_IDX_0_PH_ORD for a missing 'led0' alias.
    """
    # DT_INVALID_NODE is '_'
    if re.fullmatch(r"_(_[A-Z]\w*)?_ORD", ident):
        return handle_invalid_node()

    # Names are lowercase tokens, while what follows them (_P_<prop>, _PARENT, ...)
    # starts with an uppercase letter
    m = re.fullmatch(r"DT_(N_ALIAS|N_NODELABEL|CHOSEN|N_INST)_([a-z0-9_]+?)(_[A-Z]\w*)?_ORD", ident)
    if not m:
        # A node path identifier (DT_PATH() or a resolved alias, label, ...) followed by
        # a property or child that does not exist
        return handle_unresolved_path(edt, ident)

    handlers = {
        "N_ALIAS": handle_missing_alias,
        "N_NODELABEL": handle_missing_nodelabel,
        "CHOSEN": handle_missing_chosen,
        "N_INST": handle_missing_instance,
    }
    return handlers[m.group(1)](edt, m.group(2))


def main() -> int:
    args = parse_args()

    m = re.search(r"__device_dts_ord_(\w+)", args.symbol)
    if not m:
        return 1

    edt = load_edt(args.edt_pickle)

    if not m.group(1).isdigit():
        lines = handle_unresolved_node_id(edt, m.group(1))
        if not lines:
            return 1
    else:
        # Find node by ordinal amongst all nodes
        node = next((n for n in edt.nodes if n.dep_ordinal == int(m.group(1))), None)
        if not node:
            print(f"Ordinal {m.group(1)} not found in edt.pickle", file=sys.stderr)
            return 1

        if node.status == "okay":
            lines = handle_enabled_node(node)
        else:
            lines = handle_disabled_node(node)

    print(tabulate([["\n".join(lines)]], headers=["DT Doctor"], tablefmt="grid"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
