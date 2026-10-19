#!/usr/bin/env python3
# Copyright (c) 2026 Texas Instruments Incorporated
# SPDX-License-Identifier: Apache-2.0

"""Convert a SmartRF Studio / SysConfig RadioConfig export to a devicetree PHY.

Input is the ti_radio_config.c/.h pair exported for a CC23x0 device. Output is
a self-contained .dtsi with one ti,cc23x0-radio-config node that carries the
complete LRF_Config payload (microcode image names, every register segment,
software configs, TX shapes, TX power table), plus one operating mode child
per selectable mode.

This script stands in for a native SmartRF Studio ".dtsi" export template.
Both must emit the same binding. Semantic properties of each mode (datarate,
deviation, modulation) are derived from the register data by evaluating the
feature-gated segments, never from comments in the export.

Example:
    ti_radio_config_to_dtsi.py ti_radio_config.c -o ble_gen.dtsi \
        --mode 1-mbps:0x0000 --mode 2-mbps:0x0001 \
        --mode coded-s8:0x0002:125000 --mode coded-s2:0x0006:500000
"""

import argparse
import re
import sys
from pathlib import Path

# Pointer-slot tokens. Must match include/zephyr/dt-bindings/basic_radio/ti-cc23x0-radio-config.h.
PTR_TOKENS = {
    "TI_LRF_PTR_TX_POWER_TABLE": 0xA1F00001,
    "TI_LRF_PTR_FCFG_TRIMS": 0xA1F00002,
    "TI_LRF_PTR_SW_CONFIG_0": 0xA1F00010,
    "TI_LRF_PTR_SW_CONFIG_1": 0xA1F00011,
    "TI_LRF_PTR_SW_CONFIG_2": 0xA1F00012,
    "TI_LRF_PTR_SW_CONFIG_3": 0xA1F00013,
}
MAX_SW_CONFIGS = 4
MAX_TX_SHAPES = 4
NO_TX_SHAPE = 255

# CC23x0 modem reference clock used by LRFDMDM.BAUD/BAUDPRE.
MDM_REF_HZ = 48_000_000

# LRF_RegionOperation (hal_ti rcl/LRF.h): words of payload per element count.
REGION_HW_CLEAR = 0
REGION_HW_16 = 1
REGION_HW_32 = 2
REGION_HW_SPARSE = 3
REGION_SW_CLEAR = 4
REGION_SW_16 = 5
REGION_SW_32 = 6
REGION_SW_SPARSE = 7
REGION_PAR_CLEAR = 8
REGION_PAR_REF_32 = 9
REGION_PAR_32 = 10
REGION_HW_MASKED = 11

REG_MDM_BAUD = 0x20D4
REG_MDM_BAUDPRE = 0x20D8
REG_MDM_MODMAIN = 0x20DC
RAM_PBE_NUMCRCBITS = 0x2026
RAM_PBE_LENCFG = 0x2034


class ExportError(Exception):
    pass


def region_payload_words(rtype, count):
    if rtype in (REGION_HW_CLEAR, REGION_SW_CLEAR, REGION_PAR_CLEAR):
        return 0
    if rtype in (REGION_HW_16, REGION_SW_16):
        return (count + 1) // 2
    if rtype in (REGION_HW_32, REGION_HW_SPARSE, REGION_SW_32, REGION_SW_SPARSE,
                 REGION_PAR_REF_32, REGION_PAR_32, REGION_HW_MASKED):
        return count
    raise ExportError(f"unknown LRF region type {rtype}")


def hw32_to_reg16(addr):
    """32-bit alias address to the first 16-bit register it covers."""
    return (addr & 0xF000) + ((addr & 0x0FFF) - 0x400) * 2


def segment_applies(header, features):
    mask = header >> 16
    inverted = (header >> 14) & 1
    if mask == 0:
        return True
    if inverted:
        return (mask & features) == 0
    return (mask & features) != 0


def describe_segment(header):
    mask = header >> 16
    length = header & 0x3FFF
    if mask == 0:
        return f"segment: base (all modes), {length} words"
    cond = "clear" if (header >> 14) & 1 else "set"
    return f"segment: phyFeatures bits 0x{mask:04X} {cond}, {length} words"


def strip_c_comments(text):
    return re.sub(r"//[^\n]*", "", text)


def parse_int(text):
    text = text.strip()
    return int(text, 0)


class Word:
    def __init__(self, value=None, token=None, comment=""):
        self.value = value
        self.token = token
        self.comment = comment.strip()


class Segment:
    def __init__(self, cname, title, words):
        self.cname = cname
        self.title = title
        self.words = words


def parse_word(expr, sw_index):
    expr = expr.strip()
    m = re.fullmatch(r"\(uint32_t\)\s*&\s*\(?\s*([\w\->.]+)\s*\)?", expr)
    if m:
        target = m.group(1)
        if target == "LRF_txPowerTable":
            return Word(token="TI_LRF_PTR_TX_POWER_TABLE")
        if target == "fcfg->appTrims":
            return Word(token="TI_LRF_PTR_FCFG_TRIMS")
        if target in sw_index:
            return Word(token=f"TI_LRF_PTR_SW_CONFIG_{sw_index[target]}")
        raise ExportError(f"unsupported pointer slot '{expr}'")
    return Word(value=parse_int(expr))


def parse_array_body(body, sw_index):
    words = []
    for line in body.splitlines():
        code, _, comment = line.partition("//")
        code = code.strip().rstrip(",").strip()
        if not code:
            continue
        word = parse_word(code, sw_index)
        word.comment = comment.strip()
        words.append(word)
    return words


def parse_struct_fields(body):
    fields = {}
    for m in re.finditer(r"\.(\w+(?:\.\w+)*)\s*=\s*([^,\n]+)", body):
        fields[m.group(1)] = m.group(2).strip()
    return fields


def slug(text):
    text = re.sub(r"\(.*?\)", "", text)
    text = re.sub(r"[^a-z0-9]+", "-", text.lower())
    return text.strip("-")


class Export:
    def __init__(self, c_path, h_path):
        self.c_text = c_path.read_text()
        self.h_text = h_path.read_text()
        self._parse_header()
        self._parse_tables()
        self._parse_segments()
        self._parse_lrf_config()
        self._resolve_lencfg()

    def _search(self, pattern, text=None, required=True):
        m = re.search(pattern, self.c_text if text is None else text, re.S)
        if m is None and required:
            raise ExportError(f"pattern not found in export: {pattern}")
        return m

    def _parse_header(self):
        def first_match(patterns):
            for pat, flags in patterns:
                m = re.search(pat, self.c_text, flags)
                if m:
                    return m
            return None

        m = first_match([(r"generated for the (\S+)", 0),
                         (r"^// Device\s+(\S+)", re.M)])
        if m is None:
            raise ExportError("cannot find device name in export")
        self.device = m.group(1)

        m = first_match([(r"Tool Version:\s*(\S+)", 0),
                         (r"^// Tool version\s+(\S+)", re.M | re.I)])
        if m is None:
            raise ExportError("cannot find tool version in export")
        self.tool = m.group(1)

        m = first_match([(r"PHY name:[ \t]*([^\n]+)", 0),
                         (r"^// PHY\s{2,}(.+)", re.M)])
        if m is None:
            raise ExportError("cannot find PHY name in export")
        self.phy_name = m.group(1).strip()

        m = re.search(r"PHY abbreviation/ID:\s*(\S+)", self.c_text)
        self.phy_id = m.group(1) if m else re.sub(r"[^a-z0-9]+", "_", self.phy_name.lower()).strip("_")

        m = re.search(r"PHY definition file:\s*(\S+)", self.c_text)
        self.phy_file = m.group(1) if m else ""

        self.features = []
        for m in re.finditer(r"#define RCL_PHY_FEATURE_(\w+)\s+(0x[0-9A-Fa-f]+)", self.h_text):
            self.features.append((m.group(1), int(m.group(2), 16)))

        def h_define_optional(name):
            m = re.search(rf"#define {name}\s+(0x[0-9A-Fa-f]+|\d+)", self.h_text)
            return int(m.group(1), 0) if m else None

        self.len_pos = h_define_optional("RCL_REGISTER_FIELD_PBE_GENERIC_RAM_LENCFG_LENPOS")
        self.num_len_bits = h_define_optional("RCL_REGISTER_FIELD_PBE_GENERIC_RAM_LENCFG_NUMLENBITS")

    def _parse_tables(self):
        self.tx_shapes = []
        shape_index = {}
        for m in re.finditer(r"const LRF_TxShape (\w+)\s*=\s*\{\s*\.scale\s*=\s*(\w+),\s*"
                             r"\.numCoeff\s*=\s*(\w+),\s*\.coeff\s*=\s*\{([^}]*)\}", self.c_text):
            coeffs = [parse_int(c) for c in m.group(4).split(",") if c.strip()]
            if len(coeffs) != parse_int(m.group(3)):
                raise ExportError(f"{m.group(1)}: numCoeff does not match coefficient list")
            shape_index[m.group(1)] = len(self.tx_shapes)
            self.tx_shapes.append((m.group(1), parse_int(m.group(2)), coeffs))
        if len(self.tx_shapes) > MAX_TX_SHAPES:
            raise ExportError("too many TX shapes for the binding")

        self.sw_configs = []
        self.sw_index = {}
        for m in re.finditer(r"const LRF_SwConfig (\w+)\s*=\s*\{(.*?)\};", self.c_text, re.S):
            f = parse_struct_fields(m.group(2))
            shape = f["txShape"].lstrip("&").strip()
            shape_val = re.sub(r'^\(\s*\w[\w\s*]*\*\s*\)\s*', '', shape)
            shape_idx = NO_TX_SHAPE if shape_val in ("0", "NULL") else shape_index[shape]
            cfg = [parse_int(f["rxIntFrequency"]), parse_int(f["rxFrequencyOffset"]),
                   parse_int(f["txFrequencyOffset"]), parse_int(f["modFrequencyDeviation"]),
                   shape_idx, parse_int(f["bwIndex"]), parse_int(f["bwIndexDither"])]
            self.sw_index[m.group(1)] = len(self.sw_configs)
            self.sw_configs.append((m.group(1), cfg))
        if len(self.sw_configs) > MAX_SW_CONFIGS:
            raise ExportError("too many software configs for the binding")

        self.power_table = []
        for m in re.finditer(r"\.power\s*=\s*\{\s*\.fraction\s*=\s*(\d+),\s*\.dBm\s*=\s*(-?\d+)\s*\},\s*"
                             r"\.tempCoeff\s*=\s*(\d+),\s*\.value\s*=\s*\{\s*\.reserved\s*=\s*(\d+),\s*"
                             r"\.ib\s*=\s*(\d+),\s*\.gain\s*=\s*(\d+),\s*\.mode\s*=\s*(\d+),\s*"
                             r"\.noIfampRfLdoBypass\s*=\s*(\d+)", self.c_text):
            frac, dbm, temp, res, ib, gain, mode, bypass = (int(g) for g in m.groups())
            if frac or res or bypass:
                raise ExportError("TX power entry uses fields not covered by the binding")
            self.power_table.append((dbm, temp, ib, gain, mode))
        if not self.power_table:
            raise ExportError("no TX power table found")

    def _parse_segments(self):
        arrays = {}
        for m in re.finditer(r"(?://\s*Configuration:\s*([^\n]*)\n)?static const uint32_t (\w+)\[\]\s*=\s*"
                             r"\{(.*?)\};", self.c_text, re.S):
            title, cname, body = m.group(1) or m.group(2), m.group(2), m.group(3)
            arrays[cname] = Segment(cname, title.strip(), parse_array_body(body, self.sw_index))
        self.arrays = arrays

    def _resolve_lencfg(self):
        if self.len_pos is not None:
            return
        _, ram, _ = self.evaluate(0)
        lencfg = ram.get(RAM_PBE_LENCFG, 0)
        self.len_pos = lencfg & 0x1F
        self.num_len_bits = (lencfg >> 5) & 0x1F

    def _parse_lrf_config(self):
        m = self._search(r"const LRF_Config LRF_config\s*=\s*\{(.*?)\};", self.c_text)
        f = parse_struct_fields(m.group(1))
        self.images = {}
        for kind in ("pbe", "mce", "rfe"):
            img = re.search(rf"LRF_{kind.upper()}_binary_(\w+)", f[f"{kind}Image"])
            if img is None:
                raise ExportError(f"cannot resolve {kind}Image")
            self.images[kind] = img.group(1)
        list_name = f["regConfigList"].lstrip("&").strip()
        lm = self._search(rf"LRF_RegConfigList {list_name}\s*=\s*\{{.*?\.entries\s*=\s*\{{(.*?)\}}", self.c_text)
        names = [re.sub(r"\(LRF_ConfigWord\s*\*\)", "", e).strip() for e in lm.group(1).split(",")]
        self.segments = [self.arrays[n] for n in names if n]
        tokens = set(PTR_TOKENS.values())
        for seg in self.segments:
            for w in seg.words:
                if w.value is not None and w.value in tokens:
                    raise ExportError(f"{seg.cname}: data word collides with a pointer token")

    def evaluate(self, features):
        """Apply all segments for a phyFeatures word; return HW/RAM regs and swConfig."""
        hw, ram, par = {}, {}, {}
        for seg in self.segments:
            words = seg.words
            header = words[0].value
            if header is None or header & (1 << 15):
                raise ExportError(f"{seg.cname}: compound segments are not supported")
            if (header & 0x3FFF) != len(words) - 1:
                raise ExportError(f"{seg.cname}: segment length mismatch")
            if not segment_applies(header, features):
                continue
            i = 1
            while i < len(words):
                r = words[i].value
                rtype = (r >> 12) & 0xF
                count = (r & 0xFFF) + 1
                addr = r >> 16
                n = region_payload_words(rtype, count)
                payload = words[i + 1:i + 1 + n]
                self._apply_region(rtype, addr, payload, hw, ram, par)
                i += 1 + n
        return hw, ram, par

    @staticmethod
    def _apply_region(rtype, addr, payload, hw, ram, par):
        if rtype in (REGION_HW_16, REGION_SW_16):
            dst, step = (hw, 4) if rtype == REGION_HW_16 else (ram, 2)
            for k, w in enumerate(payload):
                dst[addr + 2 * k * step] = w.value & 0xFFFF
                dst[addr + (2 * k + 1) * step] = w.value >> 16
        elif rtype == REGION_HW_32:
            base = hw32_to_reg16(addr)
            for k, w in enumerate(payload):
                hw[base + 8 * k] = w.value & 0xFFFF
                hw[base + 8 * k + 4] = w.value >> 16
        elif rtype == REGION_SW_32:
            for k, w in enumerate(payload):
                ram[addr + 4 * k] = w.value & 0xFFFF
                ram[addr + 4 * k + 2] = w.value >> 16
        elif rtype in (REGION_HW_SPARSE, REGION_SW_SPARSE):
            dst = hw if rtype == REGION_HW_SPARSE else ram
            for w in payload:
                dst[w.value >> 16] = w.value & 0xFFFF
        elif rtype == REGION_PAR_32:
            for k, w in enumerate(payload):
                par[addr + 4 * k] = w.token if w.token else w.value


def mode_semantics(export, features):
    hw, ram, par = export.evaluate(features)
    for reg in (REG_MDM_BAUD, REG_MDM_BAUDPRE):
        if reg not in hw:
            raise ExportError(f"mode 0x{features:04X}: LRFDMDM register 0x{reg:04X} not configured")
    prescaler = hw[REG_MDM_BAUDPRE] & 0xFF
    symbol_rate = round(MDM_REF_HZ / prescaler * hw[REG_MDM_BAUD] / 65536)
    modmain = hw.get(REG_MDM_MODMAIN, 0)
    sw_token = par.get(0)
    if not sw_token or not sw_token.startswith("TI_LRF_PTR_SW_CONFIG_"):
        raise ExportError(f"mode 0x{features:04X}: no swConfig selected")
    sw_idx = int(sw_token.rsplit("_", 1)[1])
    sw = export.sw_configs[sw_idx][1]
    if modmain == 4:  # O-QPSK for IEEE 802.15.4
        modulation = "oqpsk"
    else:
        levels = 4 if (modmain & 0x3) == 1 else 2
        shaped = sw[4] != NO_TX_SHAPE
        modulation = f"{levels}-{'gfsk' if shaped else 'fsk'}"
    crc = ram.get(RAM_PBE_NUMCRCBITS, 0) != 0
    return symbol_rate, sw[3], modulation, crc, sw_idx


def fmt_hex(v):
    return f"0x{v:08X}"


def fmt_int(v):
    return f"({v})" if v < 0 else str(v)


def emit(export, modes, label, freq_range, out):
    o = []
    w = o.append
    w("/*")
    w(" * Copyright (c) 2026 Texas Instruments Incorporated")
    w(" * SPDX-License-Identifier: Apache-2.0")
    w(" *")
    w(" * GENERATED FILE - DO NOT EDIT. Regenerate from SmartRF Studio instead.")
    w(f" *   PHY name:            {export.phy_name}")
    w(f" *   PHY abbreviation/ID: {export.phy_id}")
    if export.phy_file:
        w(f" *   PHY definition file: {export.phy_file}")
    w(f" *   Device:              {export.device}")
    w(f" *   RadioConfig version: {export.tool}")
    w(" *")
    w(" * Usage: include this file from an overlay and reference one of the")
    w(" * operating mode nodes below from a ti,cc23x0-prop-rf-phy child:")
    w(f" *   ti,radio-mode = <&{label}_{slug(modes[0][0]).replace('-', '_')}>;")
    w(" */")
    w("")
    w("#include <dt-bindings/basic_radio/ti-cc23x0-radio-config.h>")
    w("")
    w("/ {")
    w("\tradio_config {")
    w(f"\t\t{label}: {slug(export.phy_id)} {{")
    w('\t\t\tcompatible = "ti,cc23x0-radio-config";')
    w("")
    w("\t\t\t/* Microcode images (rf_patches/lrf_<core>_binary_<name>.h) */")
    for kind in ("pbe", "mce", "rfe"):
        w(f'\t\t\tti,{kind}-image = "{export.images[kind]}";')
    w("")
    w("\t\t\t/* PBE_GENERIC_RAM.LENCFG: length field position and size */")
    w(f"\t\t\tti,len-pos = <{export.len_pos}>;")
    w(f"\t\t\tti,num-len-bits = <{export.num_len_bits}>;")
    w("")
    w("\t\t\t/* LRF_txPowerTable: TI_LRF_TX_POWER(dBm, tempCoeff, ib, gain, mode) */")
    rows = [f"TI_LRF_TX_POWER({', '.join(fmt_int(v) for v in row)})" for row in export.power_table]
    w(f"\t\t\tti,tx-power-table = <{rows[0]}")
    for row in rows[1:]:
        w(f"\t\t\t\t\t     {row}")
    o[-1] += ">;"
    for i, (cname, scale, coeffs) in enumerate(export.tx_shapes):
        w("")
        w(f"\t\t\t/* {cname} */")
        w(f"\t\t\tti,tx-shape-{i}-scale = <0x{scale:05X}>;")
        w(f"\t\t\tti,tx-shape-{i} = [{' '.join(f'{c:02X}' for c in coeffs)}];")
    for i, (cname, cfg) in enumerate(export.sw_configs):
        w("")
        w(f"\t\t\t/* {cname}: rxIntFrequency rxFrequencyOffset txFrequencyOffset")
        w("\t\t\t * modFrequencyDeviation txShape bwIndex bwIndexDither")
        w("\t\t\t */")
        w(f"\t\t\tti,sw-config-{i} = <{' '.join(fmt_int(v) for v in cfg)}>;")
    w("")
    w("\t\t\t/* LRF register configuration list, applied in this order */")
    w("\t\t\treg-config {")
    for seg in export.segments:
        applies = [n for n, f, _ in modes if segment_applies(seg.words[0].value, f)]
        w(f"\t\t\t\t/* {seg.title} ({seg.cname}); modes: {', '.join(applies) or 'none'} */")
        w(f"\t\t\t\t{slug(seg.title)} {{")
        w("\t\t\t\t\tti,words = <")
        for k, word in enumerate(seg.words):
            text = word.token if word.token else fmt_hex(word.value)
            comment = describe_segment(word.value) if k == 0 else word.comment
            w(f"\t\t\t\t\t\t{text:<26} /* {comment} */" if comment else f"\t\t\t\t\t\t{text}")
        w("\t\t\t\t\t>;")
        w("\t\t\t\t};")
    w("\t\t\t};")
    for name, features, datarate in modes:
        symbol_rate, deviation, modulation, crc, sw_idx = mode_semantics(export, features)
        if not datarate:
            print(f"note: mode {name}: datarate-bps set to the symbol rate ({symbol_rate}); "
                  f"pass --mode {name}:0x{features:04X}:DATARATE if the mode uses FEC/coding",
                  file=sys.stderr)
        node = slug(name)
        label_suffix = node.replace("-", "_")
        w("")
        w(f"\t\t\t/* Operating mode {name}: phyFeatures 0x{features:04X}, "
          f"{export.sw_configs[sw_idx][0]}, symbol rate {symbol_rate} sps.")
        w("\t\t\t */")
        w(f"\t\t\t{label}_{label_suffix}: mode-{node} {{")
        w(f"\t\t\t\tti,phy-feature = <0x{features:04X}>;")
        w(f'\t\t\t\tmodulation = "{modulation}";')
        w(f"\t\t\t\tdatarate-bps = <{datarate if datarate else symbol_rate}>;")
        w(f"\t\t\t\tdeviation-hz = <{deviation}>;")
        w(f"\t\t\t\tfrequency-min-hz = <{freq_range[0]}>;")
        w(f"\t\t\t\tfrequency-max-hz = <{freq_range[1]}>;")
        if crc:
            w("\t\t\t\tcrc-enable;")
        if export.num_len_bits:
            w("\t\t\t\tvariable-length;")
        w("\t\t\t};")
    w("\t\t};")
    w("\t};")
    w("};")
    out.write("\n".join(o) + "\n")


def parse_mode(text):
    parts = text.split(":")
    if len(parts) not in (2, 3):
        raise argparse.ArgumentTypeError("mode must be NAME:FEATURES[:DATARATE]")
    datarate = int(parts[2], 0) if len(parts) == 3 else 0
    return parts[0], int(parts[1], 0), datarate


def default_modes(export):
    """One mode per value of the first feature group, other groups at zero."""
    if not export.features:
        return [("default", 0, 0)]
    group = "_".join(export.features[0][0].split("_")[:2])
    return [(name[len(group) + 1:].lower().replace("_", "-"), value, 0)
            for name, value in export.features if name.startswith(group + "_")]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("export_c", type=Path, help="ti_radio_config.c from SmartRF Studio / SysConfig")
    ap.add_argument("--header", type=Path, help="matching .h (default: same name, .h)")
    ap.add_argument("-o", "--output", type=Path, default=None,
                    help="output .dtsi path (default: <script-dir>/<input-stem>_zephyr.dtsi)")
    ap.add_argument("--label", help="node label prefix (default: ti_phy_<PHY ID>)")
    ap.add_argument("--mode", action="append", type=parse_mode, default=[],
                    help="NAME:FEATURES[:DATARATE] operating mode; repeatable. "
                         "DATARATE overrides the value derived from LRFDMDM.BAUD "
                         "(needed for coded PHYs).")
    ap.add_argument("--freq-range", default="2400000000:2483500000",
                    help="MIN:MAX characterized carrier range in Hz")
    args = ap.parse_args()

    header = args.header or args.export_c.with_suffix(".h")
    try:
        export = Export(args.export_c, header)
        output = args.output or (Path(__file__).parent / (slug(export.phy_name) + ".dtsi"))
        modes = args.mode or default_modes(export)
        label = args.label or f"ti_phy_{export.phy_id}"
        freq_range = tuple(int(v) for v in args.freq_range.split(":"))
        with output.open("w") as out:
            emit(export, modes, label, freq_range, out)
    except (ExportError, KeyError) as e:
        sys.exit(f"error: {e}")


if __name__ == "__main__":
    main()