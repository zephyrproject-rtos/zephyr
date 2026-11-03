#!/usr/bin/env python3
#
# Copyright 2022-2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Validate and complete the daisy (select-input) column of generated i.MX RT
pinctrl definitions.

The signal configuration file is not a complete source for the daisy column: it
carries a DAISY assignment for some pad/ALT pairs and none for others, so the
generated pinmux entries would leave those input paths unconfigured. Two sources
can fill the gap:

  * the IOMUXC select-input candidate lists in the configuration data pack's
    register data. A select-input register multiplexes several pads onto one
    signal, so its definition has to enumerate the candidate pads - which makes
    it a complete, self-consistent (pad, ALT) -> (register, value) map. This is
    the primary source.
  * fsl_iomuxc.h from the device data. Used as a cross-check, not as the
    authority: it is generated from the same upstream database as the signal
    configuration file, so it reproduces that file's mistakes.

The candidate list is also a reverse map, and that gives a check neither source
can give on its own: the (register, value) pair a pinmux entry writes must map
back to that entry's own pad and ALT mode. A pair that maps back to a different
pad would silently re-route another pad's input, so it is treated as fatal
rather than repaired.
"""

import argparse
import re

iomuxc_re = re.compile(
    r'#define (IOMUXC_[\w_]+)\s+([0x]*[\dA-F]+)[U]*, ([0x]*[\dA-F]+)[U]*,'
    r' ([0x]*[\dA-F]+)[U]*, ([0x]*[\dA-F]+)[U]*, ([0x]*[\dA-F]+)[U]*\n'
)

# i.MX RT266x (RT2660 family) fsl_iomuxc.h macros are 4-value tuples:
# (mux_reg, mux_mode, input_reg, input_daisy). The pad configuration register is
# the same as the mux register (the PIO<port>_<pin> register holds both MUX_MODE
# and the pad config), so there is no separate 5th config-register value.
iomuxc_rt2_re = re.compile(
    r'#define (IOMUXC_[\w_]+)\s+(0[xX][\dA-Fa-f]+)[U]*, '
    r'(0[xX][\dA-Fa-f]+)[U]*, (0[xX][\dA-Fa-f]+)[U]*, (0[xX][\dA-Fa-f]+)[U]*\n'
)

pinctrl_line1_re = re.compile(r'\t/omit-if-no-ref/ [\w_]+: ([\w_]+) {\n')
pinctrl_line2_re = re.compile(
    r'\t\tpinmux = <(0x[\da-f]+) '
    r'(\d+) (0x[\da-f]+) (\d+) (0x[\da-f]+)>;'
)


def parse_iomuxc_ground_truth(iomuxc_path):
    """
    Parse an fsl_iomuxc.h file and build a ground truth mapping.
    @param iomuxc_path: path to fsl_iomuxc.h file
    @return dict mapping (mux_reg, mux) to (name, daisy_reg, daisy, cfg_reg)
    """
    ground_truth = {}
    with open(iomuxc_path, encoding='utf8') as iomux_file:
        while True:
            line = iomux_file.readline()
            if line == '':
                break
            match = iomuxc_re.match(line)
            if match:
                mux_reg = int(match.group(2), 0)
                mux = int(match.group(3), 0)
                daisy_reg = int(match.group(4), 0)
                daisy = int(match.group(5), 0)
                cfg_reg = int(match.group(6), 0)
                if (mux_reg, mux, cfg_reg) in ground_truth:
                    raise RuntimeError("Duplicate mux_reg and mux value pairings")
                ground_truth[(mux_reg, mux)] = (match.group(1), daisy_reg, daisy, cfg_reg)
                continue
            # i.MX RT266x: 4-value tuple, config register == mux register
            match = iomuxc_rt2_re.match(line)
            if match:
                mux_reg = int(match.group(2), 0)
                mux = int(match.group(3), 0)
                daisy_reg = int(match.group(4), 0)
                daisy = int(match.group(5), 0)
                cfg_reg = mux_reg
                ground_truth[(mux_reg, mux)] = (match.group(1), daisy_reg, daisy, cfg_reg)
    return ground_truth


def _resolve_daisy(key, daisy_maps, ground_truth, report):
    """
    Pick the daisy (register, value) a pinmux entry should carry.
    @param key: (mux_reg, mux_mode) of the entry
    @return (daisy_reg, daisy_val) tuple
    """
    pack = daisy_maps['forward'].get(key) if daisy_maps else None
    truth = ground_truth.get(key) if ground_truth else None
    if pack is not None:
        if truth is not None and truth[1] != 0 and (truth[1], truth[2]) != (pack.reg, pack.value):
            report['conflict'].append((key, (pack.reg, pack.value), (truth[1], truth[2]), truth[0]))
        return pack.reg, pack.value
    if truth is not None and truth[1] != 0:
        report['header_only'].append((key, (truth[1], truth[2]), truth[0]))
        return truth[1], truth[2]
    return 0, 0


def _check_round_trip(name, key, daisy, daisy_maps, report):
    """
    A daisy (register, value) pair must select the pad and ALT mode of the entry
    that writes it. Anything else re-routes a different pad's input.
    """
    if daisy == (0, 0) or not daisy_maps:
        return
    entry = daisy_maps['reverse'].get(daisy)
    if entry is None:
        report['orphan'].append((name, key, daisy))
    elif (entry.mux_reg, entry.mux_mode) != key:
        report['hijack'].append((name, key, daisy, (entry.pad, entry.mux_mode, entry.signal)))


def validate_pinctrl_file(pinctrl_path, daisy_maps=None, ground_truth=None, check=False):
    """
    Validate a generated pinctrl dtsi file and, unless check is set, rewrite its
    daisy columns from the resolved source.
    @param pinctrl_path: path to the pinctrl dtsi file
    @param daisy_maps: result of NXPSdkUtil.build_daisy_maps(), or None
    @param ground_truth: result of parse_iomuxc_ground_truth(), or None
    @param check: if True, only report, do not modify the file
    @return report dict. 'conflict', 'hijack' and 'orphan' are fatal; the
        caller decides what to do with the advisory lists.
    """
    report = {
        'fixed': [],
        'conflict': [],
        'hijack': [],
        'orphan': [],
        'header_only': [],
        'cfg_mismatch': [],
        'header_unused': [],
    }
    seen = set()
    with open(pinctrl_path, encoding='utf8') as pinctrl_file:
        lines = pinctrl_file.readlines()
    out = []
    index = 0
    while index < len(lines):
        line = lines[index]
        out.append(line)
        index += 1
        match = pinctrl_line1_re.match(line)
        if not match:
            continue
        if index >= len(lines):
            break
        name = match.group(1)
        value_line = lines[index]
        match2 = pinctrl_line2_re.match(value_line)
        index += 1
        if not match2:
            out.append(value_line)
            continue
        mux_reg = int(match2.group(1), 0)
        mux = int(match2.group(2), 0)
        cfg_reg = int(match2.group(5), 0)
        key = (mux_reg, mux)
        seen.add(key)
        daisy = _resolve_daisy(key, daisy_maps, ground_truth, report)
        _check_round_trip(name, key, daisy, daisy_maps, report)
        truth = ground_truth.get(key) if ground_truth else None
        if truth is not None and truth[3] != cfg_reg:
            report['cfg_mismatch'].append((name, cfg_reg, truth[3]))
        current = (int(match2.group(3), 0), int(match2.group(4), 0))
        if current != daisy:
            report['fixed'].append((name, current, daisy))
            out.append(
                f"\t\tpinmux = <0x{mux_reg:x} {mux} 0x{daisy[0]:x} {daisy[1]} 0x{cfg_reg:x}>;\n"
            )
        else:
            out.append(value_line)
    if ground_truth:
        for key, truth in sorted(ground_truth.items()):
            if key not in seen:
                report['header_unused'].append((truth[0], key))
    if not check:
        with open(pinctrl_path, 'w', encoding='utf8', newline='\n') as out_file:
            out_file.writelines(out)
    return report


def format_report(report, pinctrl_path):
    """
    Render a report from validate_pinctrl_file() as a list of lines.
    """
    lines = [f"Daisy validation for {pinctrl_path}:"]
    if report['fixed']:
        lines.append(
            f"  {len(report['fixed'])} entr(ies) completed from the select-input candidate lists"
        )
    if report['header_only']:
        lines.append(
            f"  {len(report['header_only'])} entr(ies) taken from "
            "fsl_iomuxc.h (no candidate list covers them)"
        )
        for key, daisy, macro in report['header_only']:
            lines.append(
                f"    {macro}: mux=(0x{key[0]:x}, {key[1]}) daisy=(0x{daisy[0]:x}, {daisy[1]})"
            )
    if report['header_unused']:
        lines.append(
            f"  WARNING: {len(report['header_unused'])} fsl_iomuxc.h "
            "mux option(s) have no pinctrl node; the configuration "
            "data does not describe them, so they cannot be generated"
        )
        for macro, key in report['header_unused']:
            lines.append(f"    {macro}: mux=(0x{key[0]:x}, {key[1]})")
    for name, current, wanted in report['cfg_mismatch']:
        lines.append(
            f"  WARNING: {name} config register 0x{current:x} "
            f"disagrees with fsl_iomuxc.h 0x{wanted:x}"
        )
    for key, pack, truth, macro in report['conflict']:
        lines.append(
            f"  ERROR: mux=(0x{key[0]:x}, {key[1]}) daisy conflict - "
            f"candidate list says (0x{pack[0]:x}, {pack[1]}), "
            f"{macro} says (0x{truth[0]:x}, {truth[1]})"
        )
    for name, _key, daisy in report['orphan']:
        lines.append(
            f"  ERROR: {name} writes daisy (0x{daisy[0]:x}, "
            f"{daisy[1]}) which no candidate list defines"
        )
    for name, _key, daisy, target in report['hijack']:
        lines.append(
            f"  ERROR: {name} writes daisy (0x{daisy[0]:x}, "
            f"{daisy[1]}) on {target[2]}, but that value selects "
            f"{target[0]}/ALT{target[1]}"
        )
    return lines


def report_is_fatal(report):
    """
    True when a report contains a problem that must stop generation.
    """
    return bool(report['conflict'] or report['hijack'] or report['orphan'])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        'iomuxc_file', metavar='IOMUXC', type=str, help='fsl_iomuxc.h file to cross-check against'
    )
    parser.add_argument('pinctrl_file', metavar='pinctrl', type=str, help='pinctrl file to check')
    parser.add_argument(
        '--check', action='store_true', help='do not edit pinctrl file, just report'
    )

    args = parser.parse_args()

    truth_map = parse_iomuxc_ground_truth(args.iomuxc_file)
    result = validate_pinctrl_file(args.pinctrl_file, ground_truth=truth_map, check=args.check)
    print('\n'.join(format_report(result, args.pinctrl_file)))
    raise SystemExit(1 if report_is_fatal(result) else 0)
