#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""
A script to help diagnose build errors related to Devicetree.

To use this script as a standalone tool, provide the path to an edt.pickle file
(e.g ./build/zephyr/edt.pickle) and a symbol that appeared in the build error
message (e.g. __device_dts_ord_123).

Example usage:

./scripts/utils/dtdoctor_analyzer.py \
    --edt-pickle ./build/zephyr/edt.pickle \
    --symbol __device_dts_ord_123

"""

import argparse
import os
import pickle
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "dts", "python-devicetree", "src"))

from tabulate import tabulate


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--edt-pickle", required=True)
    parser.add_argument("--symbol", required=True)
    return parser.parse_args()


def load_edt(path):
    with open(path, "rb") as f:
        return pickle.load(f)


def setup_kconfig():
    scripts_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    sys.path.extend([scripts_dir, os.path.join(scripts_dir, "kconfig")])
    import kconfiglib

    kconf = kconfiglib.Kconfig(os.path.join(os.environ.get("ZEPHYR_BASE"), "Kconfig"), warn=False)
    return kconf, kconfiglib


def format_node(node):
    if node.labels:
        return f"{node.labels[0]}: {node.path}"
    return node.path


def find_kconfig_deps(kconf, kconfiglib, dt_has_symbol):
    """
    Find all Kconfig symbols that depend on the provided DT_HAS symbol.
    """
    prefix = os.environ.get("CONFIG_", "CONFIG_")
    target = f"{prefix}{dt_has_symbol}"
    deps = set()

    def collect_syms(expr):
        # Recursively collect all symbol names in the expression tree except the target
        for item in kconfiglib.expr_items(expr):
            if not isinstance(item, kconfiglib.Symbol):
                continue
            sym_name = f"{prefix}{item.name}"
            if sym_name != target:
                deps.add(sym_name)

    for sym in getattr(kconf, "unique_defined_syms", []):
        for node in sym.nodes:
            # Check dependencies
            if node.dep is None:
                continue
            dep_str = kconfiglib.expr_str(
                node.dep,
                lambda sc: f"{prefix}{sc.name}" if hasattr(sc, 'name') and sc.name else str(sc),
            )
            if target in dep_str:
                collect_syms(node.dep)

            # Check selects/implies
            for attr in ["orig_selects", "orig_implies"]:
                for value, _ in getattr(node, attr, []) or []:
                    value_str = kconfiglib.expr_str(value, str)
                    if target in value_str:
                        collect_syms(value)

    return deps


def handle_enabled_node(node):
    """
    Handle diagnosis for an enabled DT node (linker error, one or more Kconfigs might be gating
    the device driver).
    """
    lines = []
    lines.append(
        f"'{format_node(node)}' is enabled but no driver appears to be available for it.\n"
    )

    compats = list(getattr(node, "compats", []))
    if compats:
        kconf, kcl = setup_kconfig()
        deps = set()
        for compat in compats:
            dt_has = f"DT_HAS_{re.sub(r'\W', '_', compat.upper())}_ENABLED"
            deps.update(find_kconfig_deps(kconf, kcl, dt_has))

        if deps:
            lines.append("Try enabling these Kconfig options:\n")
            lines.extend(f" - {dep}=y" for dep in sorted(deps))
    else:
        lines.append("Could not determine compatible; check driver Kconfig manually.")

    return lines


def handle_disabled_node(node, edt):
    """
    Handle diagnosis for a disabled DT node.
    """
    lines = []
    status_prop = node._node.props.get('status')
    lines.append(
        f"'{format_node(node)}' is disabled in {status_prop.filename}:{status_prop.lineno}"
    )

    # Show dependency
    users = getattr(node, "required_by", [])
    if users:
        lines.append("The following nodes depend on it:")
        lines.extend(f" - {u.path}" for u in users)

    # Show chosen/alias references
    chosen_refs = [
        name
        for name, n in (getattr(edt, "chosen_nodes", {}) or getattr(edt, "chosen", {})).items()
        if n is node
    ]
    alias_refs = [name for name, n in getattr(edt, "aliases", {}).items() if n is node]

    if chosen_refs or alias_refs:
        lines.append("")

    if chosen_refs:
        lines.append(
            "It is referenced as a \"chosen\" in "
            f"{', '.join([f"'{ref}'" for ref in sorted(chosen_refs)])}"
        )
    if alias_refs:
        lines.append(
            "It is referenced by the following aliases: "
            f"{', '.join([f"'{ref}'" for ref in sorted(alias_refs)])}"
        )

    lines.append("\nTry enabling the node by setting its 'status' property to 'okay'.")

    return lines


def main():
    args = parse_args()

    m = re.search(r"__device_dts_ord_(\d+)", args.symbol)
    if not m:
        return

    # Find node by ordinal amongst all nodes
    edt = load_edt(args.edt_pickle)
    node = next((n for n in edt.nodes if n.dep_ordinal == int(m.group(1))), None)
    if not node:
        print(f"Ordinal {m.group(1)} not found in edt.pickle", file=sys.stderr)
        return

    if node.status == "okay":
        lines = handle_enabled_node(node)
    else:
        lines = handle_disabled_node(node, edt)

    print(tabulate([["\n".join(lines)]], headers=["DT Doctor"], tablefmt="grid"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
