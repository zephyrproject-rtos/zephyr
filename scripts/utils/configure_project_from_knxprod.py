#!/usr/bin/env python3

# Copyright (c) 2026 Fabien Proriol
# SPDX-License-Identifier: Apache-2.0

"""
Configure a KnxDaughterBoard's prj.conf from its ETS .knxprod file
####################################################################

Usage::

    zephyr/scripts/utils/configure_project_from_knxprod.py \\
        application_w1/ets/SondeTemperatureW1.knxprod application_w1/src/prj.conf

Sets, in the given prj.conf, the five Kconfig symbols that must match the
application program described by the .knxprod:

    CONFIG_KNX_APPLICATION_PROGRAM_ID
    CONFIG_KNX_APPLICATION_PROGRAM_VERSION
    CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE
    CONFIG_KNX_GROUP_OBJECT_COUNT
    CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE

If a symbol is already assigned (`CONFIG_FOO=...`) or explicitly unset
(`# CONFIG_FOO is not set`), that line is rewritten in place. Otherwise the
symbol is appended at the end of the file under a short comment.

This script shares the .knxprod archive-walking and Group-Object-sizing
code with generate_header_from_knxprod.py (loaded from the same directory)
instead of reimplementing it, so the sizes it writes to prj.conf always
match the struct layout that script would generate into knx_app_data.h —
including its DPT-guessing heuristics for CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE.
Run that script too (or check its warnings) when a Group Object's guessed
type looks wrong; this script inherits the same "TODO" warnings on stderr.

CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE is set from the .knxprod's own
declared <RelativeSegment Size=...>, not from the computed struct size —
same choice generate_header_from_knxprod.py's Kconfig comment makes — so a
mismatch is reported as a warning rather than silently overridden by a
number the struct-generator itself flags as suspect.
"""

import argparse
import importlib.util
import re
import sys
from pathlib import Path


def _load_header_generator():
    """Load generate_header_from_knxprod.py as a module, from the same
    directory as this script, so both tools parse a .knxprod identically."""
    here = Path(__file__).resolve().parent
    src = here / 'generate_header_from_knxprod.py'
    spec = importlib.util.spec_from_file_location('generate_header_from_knxprod', src)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


ghfk = _load_header_generator()


# ---------------------------------------------------------------------------
# .knxprod -> the five Kconfig values.
# ---------------------------------------------------------------------------


def compute_kconfig_values(knxprod_path, program_ref=None):
    import zipfile

    with zipfile.ZipFile(knxprod_path) as zf:
        hardware_member = ghfk.find_hardware_member(zf)
        hw_root = ghfk.parse_xml_member(zf, hardware_member)

        hw2p = ghfk.pick_hardware2program(hw_root, program_ref)
        app_ref_elem = ghfk.find_by_localname(hw2p, 'ApplicationProgramRef')
        if app_ref_elem is None:
            sys.exit(f"error: Hardware2Program {hw2p.get('Id')!r} has no ApplicationProgramRef")
        app_ref = app_ref_elem.get('RefId')

        ap_member_path = ghfk.find_application_program_member(zf, hardware_member, app_ref)
        app_root_doc = ghfk.parse_xml_member(zf, ap_member_path)

    app_program = ghfk.find_by_localname(app_root_doc, 'ApplicationProgram')
    if app_program is None:
        sys.exit(f"error: {ap_member_path} has no ApplicationProgram element")

    app_info = dict(app_program.attrib)
    app_number = int(app_info['ApplicationNumber'])
    app_version = int(app_info['ApplicationVersion'])

    code_segment, other_segments = ghfk.pick_code_segment(app_program)
    if code_segment is None:
        sys.exit("error: no <RelativeSegment> (Static/Code) found in the .knxprod")
    for s in other_segments:
        print(
            f"warning: extra RelativeSegment {s.get('Name')!r} (Size={s.get('Size')}) "
            f"ignored, only {code_segment.get('Name')!r} is used",
            file=sys.stderr,
        )

    ap_size_decl = int(code_segment.get('Size', '0'))
    ap_members = ghfk.build_application_program_members(app_program, code_segment)
    ap_size_computed = ghfk.struct_size(ap_members)
    if ap_size_computed != ap_size_decl:
        print(
            f"warning: sizeof(struct application_program_data) would be {ap_size_computed} "
            f"but the .knxprod RelativeSegment declares Size={ap_size_decl} — writing the "
            f"declared {ap_size_decl} "
            "(CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE only needs to be >=)",
            file=sys.stderr,
        )

    go_members = ghfk.build_group_object_members(app_program)
    go_size_computed = ghfk.struct_size(go_members)

    for m in ap_members + go_members:
        if 'TODO' in m.comment:
            macro_suffix = f" ({m.macro})" if m.macro else ""
            print(f"warning: {m.decl}{macro_suffix}: {m.comment}", file=sys.stderr)

    return {
        'CONFIG_KNX_APPLICATION_PROGRAM_ID': f'0x{app_number:04X}',
        'CONFIG_KNX_APPLICATION_PROGRAM_VERSION': f'0x{app_version:02X}',
        'CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE': str(ap_size_decl),
        'CONFIG_KNX_GROUP_OBJECT_COUNT': str(len(go_members)),
        'CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE': str(go_size_computed),
    }


# ---------------------------------------------------------------------------
# prj.conf editing: replace an existing assignment/"is not set" line in
# place, or append a new one, one symbol at a time.
# ---------------------------------------------------------------------------


def update_prj_conf(text, values, knxprod_path):
    lines = text.splitlines()
    remaining = dict(values)
    report = []

    for i, line in enumerate(lines):
        m = re.match(r'^(CONFIG_[A-Z0-9_]+)=(.*)$', line)
        name, old_value = (m.group(1), m.group(2)) if m else (None, None)
        if name is None:
            m = re.match(r'^# (CONFIG_[A-Z0-9_]+) is not set\s*$', line)
            if m:
                name, old_value = m.group(1), '(not set)'
        if name is None or name not in remaining:
            continue

        new_value = remaining.pop(name)
        lines[i] = f"{name}={new_value}"
        if old_value != new_value:
            report.append(f"changed {name}: {old_value} -> {new_value}")
        else:
            report.append(f"unchanged {name}={new_value}")

    if remaining:
        if lines and lines[-1] != '':
            lines.append('')
        lines.append(f"# Application identity/sizing generated from {knxprod_path}")
        lines.append("# by zephyr/scripts/utils/configure_project_from_knxprod.py")
        for name, new_value in remaining.items():
            lines.append(f"{name}={new_value}")
            report.append(f"added {name}={new_value}")

    return '\n'.join(lines) + '\n', report


# ---------------------------------------------------------------------------
# Main.
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument('knxprod', help="path to the .knxprod file")
    parser.add_argument('prj_conf', help="path to the prj.conf to update in place")
    parser.add_argument(
        '--program-ref',
        metavar='ID',
        help="Hardware2Program Id to use, if Hardware.xml declares "
        "more than one (e.g. several bus-medium variants)",
    )
    args = parser.parse_args()

    values = compute_kconfig_values(args.knxprod, args.program_ref)

    prj_conf_path = Path(args.prj_conf)
    old_text = prj_conf_path.read_text() if prj_conf_path.exists() else ''
    new_text, report = update_prj_conf(old_text, values, args.knxprod)

    prj_conf_path.write_text(new_text)

    for line in report:
        print(line)


if __name__ == '__main__':
    main()
