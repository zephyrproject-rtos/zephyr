#!/usr/bin/env python3
# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

"""Print SiWx91x clock wiring from the build devicetree (edt.pickle).

The tree is built from ``silabs,siwx91x-clock-route`` and PLL nodes under the
three clock managers, rooted at ``/clocks`` sources.  Clock names match
``SIWX91X_CLK_*`` in include/zephyr/dt-bindings/clock/silabs/siwx91x-clock.h;
mux names are devicetree nodelabels.

Supplemental nodes not expressed as clock-route children in devicetree:

* **gate-only** — HP/ULP peripherals gated from CPU / ULP_REF_CPU (see
  siwx91x-clock.h clock IDs, no dedicated mux node in siwg917.dtsi).
* **hardlink** — fixed hardware tap off REF_AON_LF (UULP_GPIO); see
  comments in dts/arm/silabs/siwg917.dtsi under cmu_aon.
* **sleeptimer HAL** — SYSRTC source mux is configured in the Silicon Labs
  sleeptimer HAL (``sl_sleeptimer_hal_si91x_sysrtc.c``), not as a
  ``clock-route`` child; devicetree ``clocks`` on ``sysrtc0`` is informative.
* **fixed LF mux** — RTC and watchdog LF source fixed to REF_AON_LF in HW.
* **static_clock_gates** — external I2S master-clock inputs (STATIC_I2S).

Run after a build::

    west build -b <board> -t clock_tree

Or directly::

    python3 soc/silabs/silabs_siwx91x/siwx91x_clock_tree_report.py \\
        --edt-pickle build/zephyr/edt.pickle
"""

from __future__ import annotations

import argparse
import os
import pickle
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING, Optional

if TYPE_CHECKING:
    from devicetree import edtlib

from colorama import Fore, Style, init as colorama_init

CLOCK_MANAGER_COMPAT = {
    "silabs,siwx91x-cmu-aon": "aon",
    "silabs,siwx91x-cmu-hp": "hp",
    "silabs,siwx91x-cmu-ulp": "ulp",
}

ROUTE_COMPAT = "silabs,siwx91x-clock-route"
PLL_COMPAT = "silabs,siwx91x-pll"

# Keep in sync with clock_control_silabs_siwx91x_common.h (SIWX91X_CLOCK_NODE_TO_ID).
DT_LABEL_TO_CLOCK_NAME = {
    "xtal_mhz": "XTAL_MHZ",
    "xtal_khz": "XTAL_KHZ",
    "rc_mhz": "RC_MHZ",
    "rc_khz": "RC_KHZ",
    "ref_hp_mux": "REF_HP",
    "ref_ulp_mux": "REF_ULP",
    "aon_ref_hf_mux": "REF_AON_HF",
    "aon_ref_lf_mux": "REF_AON_LF",
    "ulp_ref_cpu_mux": "ULP_REF_CPU",
    "ulp_uart_mux": "ULP_UART",
    "ulp_i2s_mux": "ULP_I2S",
    "ulp_ssi_mux": "ULP_SSI",
    "ulp_adc_mux": "ULP_ADC",
    "ulp_timer_mux": "ULP_TIMER",
    "ulp_ref_aon_mux": "ULP_REF_AON",
    "ref_pll_mux": "REF_PLL",
    "cpu_mux": "CPU",
    "cpu_lp_mux": "CPU_LP",
    "qspi_mux": "QSPI",
    "qspi2_mux": "QSPI2",
    "uart0_mux": "UART0",
    "uart1_mux": "UART1",
    "ssi_mux": "SSI",
    "i2s_mux": "I2S",
    "gspi_mux": "GSPI",
    "hp_ref_ulp_mux": "HP_REF_ULP",
    "pin_out_mux": "PIN_OUT",
    "pll_soc": "PLL_SOC",
    "pll_intf": "PLL_INTF",
    "pll_i2s": "PLL_I2S",
}

# Gate-only clocks: no clock-route node in devicetree (children of CPU / ULP_REF_CPU).
# Keep in sync with HP/ULP domain IDs in siwx91x-clock.h and the silabs clocks shell.
HP_CPU_GATE_ONLY = [
    "I2C0", "I2C1", "PWM", "UDMA", "GPDMA", "RNG", "GPIO", "ICACHE",
]

ULP_CPU_GATE_ONLY = [
    "ULP_I2C", "ULP_UDMA", "ULP_GPIO",
]

# Static I2S gates: external master-clock inputs, not routed through CPU muxes.
STATIC_CLOCK_GATES = [
    ("STATIC_I2S", "external reference (I2S master clock input)"),
    ("ULP_STATIC_I2S", "external reference (ULP I2S master clock input)"),
]

# SYSRTC is grouped under XTAL_KHZ for LF-domain context only; the actual source
# mux (RC_KHZ or XTAL_KHZ) is selected in sleeptimer HAL, not devicetree.
XTAL_KHZ_SYSRTC = ("SYSRTC", "sysrtc0")

NOTE_FIXED_LF_MUX = "fixed LF mux"
NOTE_HARDLINK = "hardlink"
NOTE_GATE_ONLY = "gate-only"
NOTE_SLEEPTIMER_HAL = "sleeptimer HAL"

# REF_AON_LF children not modeled as clock-route nodes in devicetree.
XTAL_KHZ_AON_REF_LF_CHILDREN = [
    ("RTC", "rtc0", NOTE_FIXED_LF_MUX),
    ("WATCHDOG", "watchdog", NOTE_FIXED_LF_MUX),
    ("UULP_GPIO", "uulpgpio", NOTE_HARDLINK),
]

SOURCE_ORDER = ["xtal_mhz", "xtal_khz", "rc_mhz", "rc_khz"]

PRIMARY_SOURCES = frozenset(SOURCE_ORDER)


@dataclass(frozen=True)
class TreeLayout:
    tree_step: int = 4
    branch_len: int = 4
    col_clock: int = 18
    col_mux: int = 22
    col_rate: int = 11
    col_div: int = 8


LAYOUT = TreeLayout()


@dataclass
class TreeNode:
    clock_name: str = ""
    mux_name: str = ""
    rate_hz: Optional[int] = None
    div: Optional[int] = None
    note: str = ""
    primary_source: bool = False
    children: list[TreeNode] = field(default_factory=list)


def resolve_zephyr_base(explicit: Optional[str] = None) -> Path:
    """Resolve the Zephyr tree root (``ZEPHYR_BASE`` or ``--zephyr-base``)."""
    if explicit:
        base = Path(explicit)
    elif "ZEPHYR_BASE" in os.environ:
        base = Path(os.environ["ZEPHYR_BASE"])
    else:
        # This script lives in zephyr/soc/silabs/silabs_siwx91x/.
        base = Path(__file__).resolve().parents[3]

    base = base.resolve()
    devicetree_path = base / "scripts/dts/python-devicetree/src"
    if not devicetree_path.is_dir():
        print(f"Invalid Zephyr base (missing devicetree): {base}", file=sys.stderr)
        raise SystemExit(1)

    return base


def setup_devicetree_path(zephyr_base: Path) -> None:
    """Add the devicetree package to sys.path so EDT can be unpickled."""
    devicetree_path = zephyr_base / "scripts/dts/python-devicetree/src"
    sys.path.insert(0, str(devicetree_path))


def init_color(no_color: bool) -> None:
    """Initialize colorama; neutralize escape codes when color is disabled."""
    colorama_init(strip=not sys.stdout.isatty() or no_color)
    if no_color or not sys.stdout.isatty():
        Fore.RED = Style.RESET_ALL = ""


def resolve_edt_pickle(path: str) -> Path:
    """Resolve and validate an edt.pickle path from the command line.

    The report only reads a pre-existing pickle produced by the build.
    Canonicalising with ``Path.resolve()`` and requiring the file to sit under
    the current working directory keeps a malformed CLI argument (path
    traversal, symlink, or a non-file) from being opened.
    """
    resolved = Path(path).expanduser().resolve()
    if not resolved.is_file():
        print(f"edt.pickle not found: {path}", file=sys.stderr)
        raise SystemExit(1)

    cwd = Path.cwd().resolve()
    try:
        resolved.relative_to(cwd)
    except ValueError:
        print(
            f"error: {path!r} must resolve to a file under the current working "
            f"directory ({cwd})",
            file=sys.stderr,
        )
        raise SystemExit(1)

    return resolved


def safe_open_edt_pickle(path: str):
    """Open a validated edt.pickle path (see ``resolve_edt_pickle``)."""
    return open(resolve_edt_pickle(path), "rb")  # noqa: SIM115


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print SiWx91x clock wiring from edt.pickle.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Example: west build -b siwx917_rb4338a/siwg917/cpu0 -t clock_tree",
    )
    parser.add_argument(
        "--zephyr-base",
        default=None,
        help="Path to the Zephyr base directory (default: $ZEPHYR_BASE)",
    )
    parser.add_argument("--edt-pickle", required=True, help="Path to edt.pickle")
    parser.add_argument("--no-color", action="store_true", help="Disable ANSI colors")
    parser.add_argument("--no-legend", action="store_true",
                        help="Omit the legend printed after the tree")
    return parser.parse_args()


def format_freq_hz(hz: Optional[int]) -> str:
    if hz is None:
        return ""

    if hz % 1_000_000 == 0:
        return f"{hz // 1_000_000:>7} MHz"
    if hz % 1_000 == 0:
        return f"{hz // 1_000:>7} kHz"
    return f"{hz:>7} Hz"


def format_node_row(node: TreeNode, clock_width: int) -> str:
    """Clock name beside tree branch; mux/rate/div at fixed columns."""
    clock_width = max(clock_width, len(node.clock_name))
    rate = format_freq_hz(node.rate_hz)
    div = f"div={node.div}" if node.div is not None else ""

    if node.primary_source:
        clock_col = f"{Fore.RED}{node.clock_name}{Style.RESET_ALL}"
        clock_col += " " * (clock_width - len(node.clock_name))
        if rate:
            rate = f"{Fore.RED}{rate}{Style.RESET_ALL}"
    else:
        clock_col = f"{node.clock_name:<{clock_width}}"

    line = (
        f"{clock_col}"
        f"{node.mux_name:<{LAYOUT.col_mux}}"
        f"{rate:>{LAYOUT.col_rate}}"
        f"{div:>{LAYOUT.col_div}}"
    )
    if node.note:
        line = f"{line}  ({node.note})"
    return line.rstrip()


def tree_max_depth(nodes: list[TreeNode], depth: int = 0) -> int:
    deepest = depth
    for node in nodes:
        if node.children:
            deepest = max(deepest, tree_max_depth(node.children, depth + 1))
    return deepest


def data_col_start(max_depth: int) -> int:
    """Column where mux/rate/div always begin (after the clock name field)."""
    return max_depth * LAYOUT.tree_step + LAYOUT.branch_len + LAYOUT.col_clock


def print_column_header(data_start: int) -> None:
    top_gutter = LAYOUT.branch_len
    clock_width = data_start - top_gutter
    header = (
        f"{'clock':<{clock_width}}"
        f"{'mux':<{LAYOUT.col_mux}}"
        f"{'rate':>{LAYOUT.col_rate}}"
        f"{'div':>{LAYOUT.col_div}}"
    )
    rule = (
        f"{'-' * clock_width}{'-' * LAYOUT.col_mux}"
        f"{'-' * LAYOUT.col_rate}{'-' * LAYOUT.col_div}"
    )
    print(f"{' ' * top_gutter}{header}")
    print(f"{' ' * top_gutter}{rule}")


def node_label(node: edtlib.Node) -> str:
    if node.labels:
        return node.labels[0]
    return node.name


def node_frequency(node: edtlib.Node) -> Optional[int]:
    prop = node.props.get("clock-frequency")
    return prop.val if prop is not None else None


def clock_parent(node: edtlib.Node) -> Optional[edtlib.Node]:
    prop = node.props.get("clocks")
    if prop is None:
        return None
    return prop.val[0].controller


def clock_divider(node: edtlib.Node) -> Optional[int]:
    prop = node.props.get("clock-div")
    return prop.val if prop is not None else None


def source_sort_key(node: edtlib.Node) -> tuple[int, str]:
    label = node_label(node)
    if label in SOURCE_ORDER:
        return (SOURCE_ORDER.index(label), label)
    return (len(SOURCE_ORDER), label)


def find_clocks_root(edt: edtlib.EDT) -> Optional[edtlib.Node]:
    for node in edt.nodes:
        if node.path == "/clocks":
            return node
    return None


def find_clock_managers(edt: edtlib.EDT) -> list[edtlib.Node]:
    managers = []
    for node in edt.nodes:
        if node.status != "okay":
            continue
        compat = node.matching_compat
        if compat in CLOCK_MANAGER_COMPAT:
            managers.append(node)
    return sorted(managers, key=lambda n: CLOCK_MANAGER_COMPAT[n.matching_compat])


def collect_route_nodes(managers: list[edtlib.Node]) -> list[edtlib.Node]:
    routes = []
    for manager in managers:
        for child in manager.children.values():
            if child.status != "okay":
                continue
            if child.matching_compat in (ROUTE_COMPAT, PLL_COMPAT):
                routes.append(child)
    return routes


def route_tree_node(node: edtlib.Node) -> TreeNode:
    label = node_label(node)
    return TreeNode(
        clock_name=DT_LABEL_TO_CLOCK_NAME.get(label, ""),
        mux_name=label,
        rate_hz=node_frequency(node),
        div=clock_divider(node),
    )


def source_tree_node(node: edtlib.Node) -> TreeNode:
    label = node_label(node)
    return TreeNode(
        clock_name=DT_LABEL_TO_CLOCK_NAME.get(label, ""),
        mux_name=label,
        rate_hz=node_frequency(node),
        primary_source=label in PRIMARY_SOURCES,
    )


def gate_only_node(clock_name: str) -> TreeNode:
    return TreeNode(clock_name=clock_name, note=NOTE_GATE_ONLY)


def hardlink_node(clock_name: str, mux_name: str) -> TreeNode:
    return TreeNode(clock_name=clock_name, mux_name=mux_name, note=NOTE_HARDLINK)


def sleeptimer_managed_node(clock_name: str, mux_name: str) -> TreeNode:
    return TreeNode(clock_name=clock_name, mux_name=mux_name, note=NOTE_SLEEPTIMER_HAL)


def fixed_lf_mux_node(clock_name: str, mux_name: str) -> TreeNode:
    return TreeNode(clock_name=clock_name, mux_name=mux_name, note=NOTE_FIXED_LF_MUX)


def build_route_subtree(parent: edtlib.Node, routes: list[edtlib.Node]) -> list[TreeNode]:
    children = []
    for route in routes:
        if clock_parent(route) != parent:
            continue

        node = route_tree_node(route)
        node.children = build_route_subtree(route, routes)
        children.append(node)

    children.sort(key=lambda entry: entry.mux_name.lower())
    return children


def apply_xtal_khz_supplemental_nodes(node: TreeNode) -> None:
    clock_name, mux_name = XTAL_KHZ_SYSRTC
    node.children.insert(0, sleeptimer_managed_node(clock_name, mux_name))

    for child in node.children:
        if child.mux_name != "aon_ref_lf_mux":
            continue

        for clock_name, mux_name, note in XTAL_KHZ_AON_REF_LF_CHILDREN:
            if note == NOTE_HARDLINK:
                child.children.append(hardlink_node(clock_name, mux_name))
            else:
                child.children.append(fixed_lf_mux_node(clock_name, mux_name))

        child.children.sort(key=lambda entry: entry.mux_name.lower())
        break


def build_source_subtree(source: edtlib.Node, routes: list[edtlib.Node]) -> TreeNode:
    node = source_tree_node(source)
    node.children = build_route_subtree(source, routes)

    if node_label(source) == "xtal_khz":
        apply_xtal_khz_supplemental_nodes(node)

    return node


def find_tree_node(nodes: list[TreeNode], mux_prefix: str) -> Optional[TreeNode]:
    for node in nodes:
        if node.mux_name.startswith(mux_prefix):
            return node

        found = find_tree_node(node.children, mux_prefix)
        if found is not None:
            return found

    return None


def append_gate_only_children(proc_node: TreeNode, gate_names: list[str]) -> None:
    for name in gate_names:
        proc_node.children.append(gate_only_node(name))
    proc_node.children.sort(key=lambda entry: entry.clock_name.lower())


def build_static_clock_gates() -> TreeNode:
    folder = TreeNode(mux_name="static_clock_gates",
                      note="external reference, not under CPU")
    for clock_name, note in STATIC_CLOCK_GATES:
        folder.children.append(TreeNode(clock_name=clock_name, note=note))
    return folder


def validate_route_mappings(routes: list[edtlib.Node]) -> list[str]:
    """Return warnings for clock-route nodes missing from DT_LABEL_TO_CLOCK_NAME."""
    warnings = []
    for route in routes:
        label = node_label(route)
        if label not in DT_LABEL_TO_CLOCK_NAME:
            warnings.append(f"unmapped clock-route nodelabel: {label}")
    return warnings


def print_legend() -> None:
    print()
    print("Legend:")
    print("  clock  - SIWX91X_CLK_* name (see siwx91x-clock.h / silabs clocks shell)")
    print("  mux    - devicetree nodelabel for a clock-route or PLL node")
    print("  div    - devicetree clock-div property (divider semantics per mux in siwg917.dtsi)")
    print("  gate-only      - peripheral gate on CPU/ULP_REF_CPU, no dedicated mux in DT")
    print("  hardlink       - fixed HW connection not modeled as clock-route child")
    print("  sleeptimer HAL - SYSRTC source mux in sl_sleeptimer HAL (RC_KHZ or XTAL_KHZ);")
    print("                   devicetree clocks on sysrtc0 is informative only")
    print("  fixed LF mux   - RTC/watchdog LF source fixed to REF_AON_LF in hardware")
    print("  Primary sources (xtal/rc) are highlighted when color output is enabled.")


def render_tree(children: list[TreeNode], data_start: int, prefix: str = "") -> None:
    for idx, child in enumerate(children):
        is_last = idx == len(children) - 1
        branch = "└── " if is_last else "├── "
        extension = "    " if is_last else "│   "

        gutter = prefix + branch
        clock_width = data_start - len(gutter)
        print(f"{gutter}{format_node_row(child, clock_width)}")

        if child.children:
            render_tree(child.children, data_start, prefix + extension)


def main() -> int:
    args = parse_args()
    setup_devicetree_path(resolve_zephyr_base(args.zephyr_base))
    init_color(args.no_color)

    from devicetree import edtlib  # noqa: E402,F401

    with safe_open_edt_pickle(args.edt_pickle) as edt_file:
        edt = pickle.load(edt_file)

    managers = find_clock_managers(edt)
    if not managers:
        print("No SiWx91x clock managers in devicetree (skipped).")
        return 0

    clocks_root = find_clocks_root(edt)
    routes = collect_route_nodes(managers)

    for warning in validate_route_mappings(routes):
        print(f"warning: {warning}", file=sys.stderr)

    top_level: list[TreeNode] = []

    if clocks_root is not None:
        sources = sorted(
            [child for child in clocks_root.children.values() if child.status == "okay"],
            key=source_sort_key,
        )
        for source in sources:
            top_level.append(build_source_subtree(source, routes))

    hp_cpu = find_tree_node(top_level, "cpu_mux")
    if hp_cpu is not None:
        append_gate_only_children(hp_cpu, HP_CPU_GATE_ONLY)

    ulp_cpu = find_tree_node(top_level, "ulp_ref_cpu_mux")
    if ulp_cpu is not None:
        append_gate_only_children(ulp_cpu, ULP_CPU_GATE_ONLY)

    top_level.append(build_static_clock_gates())

    max_depth = tree_max_depth(top_level)
    data_start = data_col_start(max_depth)

    print("SiWx91x clock tree (devicetree boot routes + hardware links):")
    print_column_header(data_start)
    print("/")
    render_tree(top_level, data_start)

    if not args.no_legend:
        print_legend()

    return 0


if __name__ == "__main__":
    sys.exit(main())
