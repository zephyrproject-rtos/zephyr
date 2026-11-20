#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
# SPDX-License-Identifier: Apache-2.0

import argparse
import collections
import os
import re
import sys

import pdfplumber

SOC = "cyt4dn"
SOC_PATH = "cat1c/traveo/cyt4dn"
PINCTRL_ADDR = 0x40300000
ALT_TITLE = r"Pin selector and alternate pin functions|Alternate pin functions in active power mode"

HSIOM_ACT = [8, 9, 10, 11, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27]
HSIOM_DS = [12, 13, 14, 15, 28, 29, 30, 31]
HSIOM_NAME = {v: f"HSIOM_SEL_ACT_{i}" for i, v in enumerate(HSIOM_ACT)}
HSIOM_NAME.update({v: f"HSIOM_SEL_DS_{i}" for i, v in enumerate(HSIOM_DS)})

PIN_RE = re.compile(r"^P(\d+)\.(\d+)(?:/\S+)?$")
PKG_RE = re.compile(r"BGA", re.I)
DASHES = ("", "-", "\u2013", "\u2014")

MATRIX_RULES = [
    (r"^scb(\d+)_(rx|tx|rts|cts)$", ["scb{0}_uart_{1}"]),
    (r"^scb(\d+)_(scl|sda)$", ["scb{0}_i2c_{1}"]),
    (r"^scb(\d+)_(clk|miso|mosi)$", ["scb{0}_spi_m_{1}", "scb{0}_spi_s_{1}"]),
    (r"^scb(\d+)_sel(\d+)$", ["scb{0}_spi_m_select{1}", "scb{0}_spi_s_select{1}"]),
    (r"^sdhc(\d*)_(card_dat_\dto\d)_\d$", ["sdhc{0}_{1}"]),
    (r"^sdhc(\d*)_(\w+)$", ["sdhc{0}_{1}"]),
    (r"^pwm\d+_m_(\d+)(?:_n)?$", ["pwm1_{0}"]),
    (r"^pwm\d+_h_(\d+)(?:_n)?$", ["pwm2_{0}"]),
    (r"^pwm(\d+)_(\d+)(?:_n)?$", ["pwm{0}_{1}"]),
]

PDL_RULES = [
    (r"^scb\[(\d+)\]\.uart_(\w+)$", ["scb{0}_uart_{1}"]),
    (r"^scb\[(\d+)\]\.i2c_(\w+)$", ["scb{0}_i2c_{1}"]),
    (r"^scb\[(\d+)\]\.spi_select(\d+)$", ["scb{0}_spi_m_select{1}", "scb{0}_spi_s_select{1}"]),
    (r"^scb\[(\d+)\]\.spi_(\w+)$", ["scb{0}_spi_m_{1}", "scb{0}_spi_s_{1}"]),
]


def generic_name(sig):
    sig = re.sub(r"\[(\d+)\]", r"\1", sig.lower())
    sig = re.sub(r"[^a-z0-9]+", "_", sig)
    return sig.strip("_")


def map_name(sig, rules):
    for pat, fmts in rules:
        m = re.match(pat, sig)
        if m:
            groups = [g if g else "0" for g in m.groups()]
            return [f.format(*groups) for f in fmts]
    return [generic_name(sig)]


def matrix_sig(sig):
    return re.sub(r"\(\d+(?:,\d+)*\)$", "", sig.replace("-", "")).lower()


def matrix_compl(sig):
    return re.match(r"^pwm\d+_(?:[mh]_)?\d+_n$", matrix_sig(sig)) is not None


def matrix_names(sig):
    return map_name(matrix_sig(sig), MATRIX_RULES)


def pdl_compl(sig):
    return re.match(r"^tcpwm\[\d+\]\.line_compl\[", sig) is not None


def pdl_names(sig):
    m = re.match(r"^tcpwm\[\d+\]\.line(?:_compl)?\[(\d+)\]$", sig)
    if m:
        line = int(m.group(1))
        return [f"pwm{line // 256}_{line % 256}"]
    for pat, fmts in PDL_RULES:
        m = re.match(pat, sig)
        if m:
            return [f.format(*m.groups()) for f in fmts]
    return [generic_name(re.sub(r"^\w+(?:\[\d+\])?\.", "", sig))]


def route_value(kind, num):
    return (HSIOM_ACT if kind == "ACT" else HSIOM_DS)[int(num)]


def clean_cell(cell):
    return (cell or "").replace("\n", "").replace(" ", "")


def drop_superscripts(page):
    sizes = collections.Counter(round(c["size"], 1) for c in page.chars)
    body = sizes.most_common(1)[0][0] if sizes else 0
    return page.filter(lambda o: o.get("object_type") != "char" or o["size"] >= 0.9 * body)


def parse_matrix(pdf):
    title_re = re.compile(ALT_TITLE)
    result = []
    avail = collections.defaultdict(set)
    for page in pdf.pages:
        if not title_re.search(page.extract_text() or ""):
            continue
        page = drop_superscripts(page)
        pins = [w for w in page.extract_words() if re.match(r"^P\d+\.\d+", w["text"])]
        for table in page.find_tables():
            hcon = None
            pkgcols = {}
            for ri, row in enumerate(table.extract()):
                cells = [clean_cell(c) for c in row]
                heads = [re.sub(r"\[[\d,]+\]", "", c) for c in cells]
                if any(re.match(r"^HCon#\d+$", c) for c in heads):
                    hcon = [int(c[5:]) if re.match(r"^HCon#\d+$", c) else None for c in heads]
                    pkgcols = {i: c for i, c in enumerate(heads) if PKG_RE.search(c)}
                    continue
                if hcon is None:
                    continue
                m = PIN_RE.match(re.sub(r"\[[\d,]+\]$", "", cells[0]))
                if m is None:
                    y0, y1 = table.rows[ri].bbox[1], table.rows[ri].bbox[3]
                    hit = [
                        w["text"] for w in pins if y0 - 1 <= (w["top"] + w["bottom"]) / 2 <= y1 + 1
                    ]
                    if len(hit) != 1:
                        continue
                    m = PIN_RE.match(re.sub(r"\[[\d,]+\]$", "", hit[0]))
                    if m is None:
                        continue
                pin = (int(m.group(1)), int(m.group(2)))
                for i, name in pkgcols.items():
                    if cells[i] not in DASHES:
                        avail[name].add(pin)
                for i, cell in enumerate(cells):
                    if hcon[i] in HSIOM_NAME and cell not in DASHES and i not in pkgcols:
                        result.append((pin, cell, hcon[i]))
    return result, avail


def parse_route_tables(pdf):
    head_re = re.compile(r"^PIN( (ACT|DS) ?#\d+)+$", re.M)
    result = []
    for page in pdf.pages:
        if not head_re.search(page.extract_text() or ""):
            continue
        for rows in drop_superscripts(page).extract_tables():
            heads = [clean_cell(c) for c in rows[0]]
            if heads[0] != "PIN":
                continue
            routes = [re.match(r"^(ACT|DS)#(\d+)$", c) for c in heads]
            if not all(routes[1:]):
                continue
            for row in rows[1:]:
                cells = [clean_cell(c) for c in row]
                m = PIN_RE.match(cells[0])
                if m is None:
                    continue
                pin = (int(m.group(1)), int(m.group(2)))
                for r, cell in zip(routes[1:], cells[1:], strict=True):
                    if cell not in DASHES:
                        result.append((pin, cell, route_value(r[1], r[2])))
    return result


def package_key(name):
    name = name.lower()
    return "".join(re.findall(r"[a-z]+", name)), "".join(re.findall(r"\d+", name))


def add_node(seen, pin, name, hsiom, compl=False):
    base = f"p{pin[0]}_{pin[1]}_{name}"
    if any(v["base"] == base and v["hsiom"] == hsiom for v in seen.values()):
        return
    label, n = base, 1
    while label in seen:
        label = f"{base}_{n}"
        n += 1
    seen[label] = {
        "pin": pin,
        "hsiom": hsiom,
        "name": name,
        "base": base,
        "node": f"{label}_compl" if compl else label,
    }


def dedupe(signals):
    seen = {}
    for pin, sig, hsiom in sorted(set(signals)):
        for name in matrix_names(sig):
            add_node(seen, pin, name, hsiom, matrix_compl(sig))
    return seen


def add_pdl(nodes, pins, pdl):
    have = {(n["pin"], n["hsiom"]) for n in nodes.values()}
    nodes = dict(nodes)
    added = []
    for (pin, hsiom), sig in sorted(pdl.items()):
        if pin in pins and (pin, hsiom) not in have:
            for name in pdl_names(sig):
                add_node(nodes, pin, name, hsiom, pdl_compl(sig))
            added.append((pin, hsiom, sig))
    return nodes, added


def group_of(name):
    return re.sub(r"_+", "_", re.sub(r"\d+", "", name)).strip("_")


def read_soc_gpio(zephyr_base):
    path = os.path.join(zephyr_base, "dts", "arm", "infineon", SOC_PATH, f"{SOC}.dtsi")
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as f:
        text = f.read()
    ports = {}
    for m in re.finditer(r"gpio_prt(\d+):\s*gpio@[0-9a-f]+\s*\{(.*?)\};", text, re.S):
        ng = re.search(r"ngpios\s*=\s*<(\d+)>", m.group(2))
        ports[int(m.group(1))] = int(ng.group(1)) if ng else 8
    return ports


def render(nodes, pins, soc_gpio):
    out = [
        "/*",
        " * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG",
        " * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz",
        " * SPDX-License-Identifier: Apache-2.0",
        " */",
        "",
        "#include <zephyr/dt-bindings/gpio/gpio.h>",
        "#include <zephyr/dt-bindings/pinctrl/ifx_cat1-pinctrl.h>",
        f'#include "{SOC}.dtsi"',
        "",
        "/ {",
        "\tsoc {",
        f"\t\tpinctrl: pinctrl@{PINCTRL_ADDR:x} {{",
    ]
    groups = collections.defaultdict(list)
    for label, n in nodes.items():
        if n["pin"] in pins:
            groups[group_of(n["name"])].append((n["pin"], n["hsiom"], label, n["node"]))
    first = True
    for group in sorted(groups):
        if not first:
            out.append("")
        first = False
        out.append(f"\t\t\t/* {group} */")
        for pin, hsiom, label, node in sorted(groups[group]):
            out += [
                f"\t\t\t/omit-if-no-ref/ {label}: {node} {{",
                f"\t\t\t\tpinmux = <DT_CAT1_PINMUX({pin[0]}, {pin[1]}, {HSIOM_NAME[hsiom]})>;",
                "\t\t\t};",
            ]
    out += ["\t\t};", "\t};", "};"]
    if soc_gpio:
        bonded = collections.defaultdict(list)
        for port, num in pins:
            bonded[port].append(num)
        for port in sorted(soc_gpio):
            if port not in bonded:
                out += ["", f"/delete-node/ &gpio_prt{port};"]
            elif max(bonded[port]) + 1 != soc_gpio[port]:
                out += ["", f"&gpio_prt{port} {{", f"\tngpios = <{max(bonded[port]) + 1}>;", "};"]
    return "\n".join(out) + "\n"


def read_pdl(path):
    entries = {}
    bonded = set()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"^#define\s+P(\d+)_(\d+)_(?:[A-Z]+_)*PORT\b", line)
            if m:
                bonded.add((int(m.group(1)), int(m.group(2))))
                continue
            m = re.match(r"^\s*P(\d+)_(\d+)_\w+\s*=\s*(\d+)\b.*?- (\S+?)(?::\d+)?\s*\*/", line)
            if m and int(m.group(3)) in HSIOM_NAME:
                pin = (int(m.group(1)), int(m.group(2)))
                entries[(pin, int(m.group(3)))] = m.group(4)
    return entries, bonded


def pdl_package(path):
    stem = os.path.basename(path)
    stem = re.sub(r"^gpio_[a-z0-9]+_", "", re.sub(r"\.h$", "", stem))
    return package_key(stem)


def cross_check(name, nodes, pins, entries, bonded):
    ds = {(n["pin"], n["hsiom"]) for n in nodes.values() if n["pin"] in pins}
    pdl = set(entries)
    match, only_pdl, only_ds = len(ds & pdl), len(pdl - ds), len(ds - pdl)
    print(f"  pdl check {name}: {match} match, {only_pdl} only in PDL, {only_ds} only in datasheet")
    for pin in sorted(bonded - pins):
        print(f"    bonded only in PDL       P{pin[0]}.{pin[1]}")
    for pin in sorted(pins - bonded):
        print(f"    bonded only in datasheet P{pin[0]}.{pin[1]}")
    for pin, hsiom in sorted(ds - pdl):
        print(f"    only datasheet P{pin[0]}.{pin[1]} {HSIOM_NAME[hsiom]}")


def main():
    ap = argparse.ArgumentParser(
        description="Generate the CYT4DN pinctrl dtsi from the datasheet",
        allow_abbrev=False,
    )
    ap.add_argument("--datasheet", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument(
        "--pdl",
        action="append",
        default=[],
        help="PDL gpio_<device>_<package>.h: cross-check and add entries not in the datasheet",
    )
    ap.add_argument("--zephyr-base", default=os.environ.get("ZEPHYR_BASE"))
    args = ap.parse_args()

    with pdfplumber.open(args.datasheet) as pdf:
        signals, avail = parse_matrix(pdf)
        signals += parse_route_tables(pdf)
    if not signals or not avail:
        sys.exit("no alternate functions or packages found in the datasheet")

    nodes = dedupe(signals)
    soc_gpio = read_soc_gpio(args.zephyr_base) if args.zephyr_base else None
    pdl = {pdl_package(p): (p,) + read_pdl(p) for p in args.pdl}
    os.makedirs(args.out, exist_ok=True)
    for pkg in sorted(avail):
        pins = avail[pkg]
        pkg_nodes = nodes
        if package_key(pkg) in pdl:
            path, entries, bonded = pdl.pop(package_key(pkg))
            cross_check(os.path.basename(path), nodes, pins, entries, bonded)
            pins = pins | bonded
            pkg_nodes, added = add_pdl(nodes, pins, entries)
            for pin, hsiom, sig in added:
                print(f"    added from PDL P{pin[0]}.{pin[1]} {HSIOM_NAME[hsiom]} {sig}")
        fname = f"{SOC}.{pkg.lower()}.dtsi"
        with open(os.path.join(args.out, fname), "w", encoding="ascii") as f:
            f.write(render(pkg_nodes, pins, soc_gpio))
        count = sum(1 for n in pkg_nodes.values() if n["pin"] in pins)
        print(f"{fname}: {len(pins)} pins, {count} nodes")
    for path, _, _ in pdl.values():
        print(f"  pdl check: no datasheet package matches {path}")


if __name__ == "__main__":
    main()
