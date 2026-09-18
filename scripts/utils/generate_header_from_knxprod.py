#!/usr/bin/env python3

# Copyright (c) 2026 Fabien Proriol
# SPDX-License-Identifier: Apache-2.0

"""
Generate a KnxDaughterBoard's knx_app_data.h from its ETS .knxprod file
########################################################################

Usage::

    zephyr/scripts/utils/generate_header_from_knxprod.py \\
        application_w1/ets/knxboard-w1.knxprod > application_w1/src/knx_app_data.h

A .knxprod file is a ZIP archive of XML files. The chain this script walks
to reach the application's parameter/Group-Object layout:

    <archive root>/<Manufacturer>/Hardware.xml
        -> <Hardware><Hardware2Program ...>
             <ApplicationProgramRef RefId="..."/>            (picks the file below)
    <archive root>/<Manufacturer>/<ApplicationProgramRef RefId>.xml
        -> <ApplicationProgram>
             <Static><Code><RelativeSegment Size="..."/></Code>   (= application
                                                                     program data size)
                     <Parameters><Parameter Name=... Memory Offset=.../></Parameters>
                     <ComObjectTable><ComObject Number=... Name=...
                                                 ObjectSize=.../></ComObjectTable>

The manufacturer folder name (e.g. "M-00FA") is read straight off the
archive's own directory layout rather than cross-referenced against
knx_master.xml — that file is ETS's global, ~300-manufacturer catalogue
shared by every .knxprod in existence, not project-specific data, so
nothing here actually depends on it.

What this script cannot know from the .knxprod alone
------------------------------------------------------

- **A Group Object's exact Datapoint Type.** The stripped-down ComObject
  element in this archive format carries only a byte/bit ObjectSize, not a
  DPT number. This script guesses the KNX stack's C cache type
  (``Type_DPT_Value_Temp``, ``Type_DPT_Value_Volt``, ``Type_DPT_Enable``,
  see zephyr/include/zephyr/knx/dpt.h) from keywords in the object's name
  ("temperature", "voltage") and falls back to a same-width raw integer type
  otherwise — every guess is marked with a ``/* TODO ... */`` comment in the
  generated struct. Verify against the ETS product database by hand.
- **DEVICE_PRODUCT_ID.** The hand-written knx_app_data.h headers this
  project used to carry a ``DEVICE_PRODUCT_ID`` byte array — grepping the
  tree shows nothing in zephyr/subsys/knx/ or any application/ ever reads
  it, and it does not correspond to anything in the .knxprod (it looks like
  a carrier-board hardware-revision marker, e.g. "KNXBOARD V3", not
  application/ETS data at all). This script does not regenerate it; add it
  back by hand in application_*/src/knx_app_data.h if some future code
  needs it.
- **Sub-byte (BitOffset != 0) Parameters.** Two ETS Parameters can share one
  byte via BitOffset when they are small enough (e.g. packed flag bits).
  This script does not pack them into a C bitfield — each still gets its
  own struct field, flagged with a comment — because none of the KnxBoard
  applications use packed parameters today. Fix by hand if a future
  .knxprod does.

"""

import argparse
import re
import sys
import xml.etree.ElementTree as ET
import zipfile
from dataclasses import dataclass

# ---------------------------------------------------------------------------
# XML helpers.
#
# The .knxprod XML carries a versioned default namespace
# (xmlns="http://knx.org/xml/project/11"). Stripping it before comparing
# tag names means this script keeps working if ETS bumps that version
# number, instead of hardcoding "11" everywhere.
# ---------------------------------------------------------------------------


def localname(tag):
    return tag.split('}', 1)[-1] if '}' in tag else tag


def iter_by_localname(elem, name):
    for e in elem.iter():
        if localname(e.tag) == name:
            yield e


def find_by_localname(elem, name):
    return next(iter_by_localname(elem, name), None)


# ---------------------------------------------------------------------------
# Archive navigation: .knxprod -> Hardware.xml -> ApplicationProgram xml.
# ---------------------------------------------------------------------------


def parse_xml_member(zf, member):
    with zf.open(member) as f:
        return ET.parse(f).getroot()


def find_hardware_member(zf):
    candidates = sorted(n for n in zf.namelist() if n.endswith('/Hardware.xml'))
    if not candidates:
        sys.exit("error: no '<Manufacturer>/Hardware.xml' member in this .knxprod")
    if len(candidates) > 1:
        sys.exit(
            "error: more than one manufacturer folder has a Hardware.xml ({}) — "
            "not supported by this script".format(', '.join(candidates))
        )
    return candidates[0]


def pick_hardware2program(hw_root, program_ref=None):
    programs = list(iter_by_localname(hw_root, 'Hardware2Program'))
    if not programs:
        sys.exit("error: Hardware.xml has no Hardware2Program entries")
    if program_ref:
        for p in programs:
            if p.get('Id') == program_ref:
                return p
        sys.exit(
            "error: --program-ref {!r} not among: {}".format(
                program_ref, ', '.join(p.get('Id') for p in programs)
            )
        )
    if len(programs) > 1:
        listing = '\n'.join(
            "  {} -> ApplicationProgramRef={}".format(
                p.get('Id'), find_by_localname(p, 'ApplicationProgramRef').get('RefId')
            )
            for p in programs
        )
        sys.exit(
            "error: Hardware.xml has more than one Hardware2Program, pass "
            "--program-ref to pick one:\n" + listing
        )
    return programs[0]


def find_application_program_member(zf, hardware_member, app_ref):
    mfr_dir = hardware_member.rsplit('/', 1)[0]
    candidate = f"{mfr_dir}/{app_ref}.xml"
    if candidate in zf.namelist():
        return candidate
    matches = [n for n in zf.namelist() if n.endswith(f'/{app_ref}.xml')]
    if matches:
        return matches[0]
    sys.exit(f"error: no XML member for application program {app_ref!r} in the archive")


# ---------------------------------------------------------------------------
# Identifier sanitising — ETS free-text Names become C identifiers.
# ---------------------------------------------------------------------------


def sanitize(text, upper=False):
    s = re.sub(r'[^0-9A-Za-z]+', '_', text.strip())
    s = re.sub(r'_+', '_', s).strip('_')
    if not s:
        s = 'UNNAMED'
    if s[0].isdigit():
        s = '_' + s
    return s.upper() if upper else s


def dedupe(name, seen):
    """Suffix a name with _2, _3, ... if it collides with one already used."""
    base = name
    n = 2
    while name in seen:
        name = f"{base}_{n}"
        n += 1
    seen.add(name)
    return name


# ---------------------------------------------------------------------------
# A single generated struct member.
# ---------------------------------------------------------------------------


@dataclass
class Member:
    decl: str  # e.g. "Scale1" or "raw9[6]"
    ctype: str  # e.g. "float", "Type_DPT_Value_Temp", "uint8_t"
    size: int  # sizeof(ctype) in bytes (or sizeof the whole array)
    align: int
    comment: str = ""
    macro: str = ""  # KNX_GO_* name, only set for Group Objects


def round_up(x, a):
    return ((x + a - 1) // a) * a


def struct_size(members):
    offset = 0
    max_align = 1
    for m in members:
        offset = round_up(offset, m.align)
        offset += m.size
        max_align = max(max_align, m.align)
    return round_up(offset, max_align)


# ---------------------------------------------------------------------------
# application_program_data — from <Static><Code><RelativeSegment> +
# <Parameters>.
# ---------------------------------------------------------------------------


def pick_code_segment(app_root):
    segments = list(iter_by_localname(app_root, 'RelativeSegment'))
    if not segments:
        return None, []
    chosen = next((s for s in segments if s.get('Name') == 'Parameters'), segments[0])
    others = [s for s in segments if s is not chosen]
    return chosen, others


def resolve_parameter_type(app_root, type_ref, cache):
    """Map a <ParameterType> RefId to a (ctype, size, align, note) tuple, or
    None if this script doesn't recognise the underlying type kind."""
    if type_ref in cache:
        return cache[type_ref]

    result = None
    pt = next(
        (e for e in iter_by_localname(app_root, 'ParameterType') if e.get('Id') == type_ref), None
    )
    if pt is not None:
        for child in pt:
            lname = localname(child.tag)
            if lname == 'TypeFloat':
                enc = child.get('Encoding', '')
                if 'Double' in enc:
                    result = ('double', 8, 8, f"IEEE-754 double ({enc})")
                else:
                    result = ('float', 4, 4, f"IEEE-754 single ({enc})")
                break
            if lname == 'TypeNumber':
                bits = int(child.get('SizeInBit', '8'))
                kind = (child.get('Type') or '').lower()
                signed = 'signed' in kind and 'unsigned' not in kind
                nbytes = max(1, (bits + 7) // 8)
                width = {1: 8, 2: 16, 4: 32, 8: 64}.get(nbytes, nbytes * 8)
                ctype = f"{'' if signed else 'u'}int{width}_t"
                result = (
                    ctype,
                    nbytes,
                    nbytes,
                    f"TypeNumber {child.get('Type')}, {bits} bit",
                )
                break
            if lname == 'TypeRestriction':
                bits = int(child.get('SizeInBit', '8'))
                nbytes = max(1, (bits + 7) // 8)
                ctype = {1: 'uint8_t', 2: 'uint16_t', 4: 'uint32_t'}.get(nbytes, 'uint8_t')
                nbytes = {'uint8_t': 1, 'uint16_t': 2, 'uint32_t': 4}[ctype]
                result = (
                    ctype,
                    nbytes,
                    nbytes,
                    f"TypeRestriction (enum), {bits} bit — value/Text meaning not "
                    "reproduced, check the .knxprod by hand",
                )
                break
            # TypeText / TypeIPAddress / TypeColor / ... : not handled, the
            # caller falls back to gap-based sizing.
    cache[type_ref] = result
    return result


def build_application_program_members(app_root, code_segment):
    seg_id = code_segment.get('Id')
    entries = []
    for p in iter_by_localname(app_root, 'Parameter'):
        mem = find_by_localname(p, 'Memory')
        if mem is None or mem.get('CodeSegment') != seg_id:
            continue
        entries.append((int(mem.get('Offset', '0')), int(mem.get('BitOffset', '0')), p))
    entries.sort(key=lambda t: (t[0], t[1]))

    type_cache = {}
    members = []
    seen_names = set()
    for i, (offset, bit_offset, p) in enumerate(entries):
        name = p.get('Name') or p.get('Id')
        decl = dedupe(sanitize(name), seen_names)
        resolved = resolve_parameter_type(app_root, p.get('ParameterType'), type_cache)

        notes = []
        if bit_offset != 0:
            notes.append(
                f"BitOffset={bit_offset} (shares a byte with another parameter) — "
                "generated as its own field, pack by hand if that matters"
            )

        if resolved is None:
            nbytes = (entries[i + 1][0] - offset) if i + 1 < len(entries) else 1
            nbytes = max(1, nbytes)
            notes.append(
                f"unrecognized ParameterType {p.get('ParameterType')!r} for {name!r} — "
                f"defaulted to a raw {nbytes}-byte field, verify by hand"
            )
            if nbytes == 1:
                members.append(Member(decl, 'uint8_t', 1, 1, '; '.join(notes)))
            else:
                members.append(Member(f"{decl}[{nbytes}]", 'uint8_t', nbytes, 1, '; '.join(notes)))
        else:
            ctype, size, align, note = resolved
            notes.insert(0, note)
            members.append(Member(decl, ctype, size, align, '; '.join(notes)))

    return members


# ---------------------------------------------------------------------------
# group_objects_data — from <ComObjectTable>.
# ---------------------------------------------------------------------------

_OBJECT_SIZE_RE = re.compile(r'\s*(\d+)\s*(Bit|Byte)s?\s*$', re.IGNORECASE)

# Keyword -> (ctype, size, align). Extend this as more DPT-backed Group
# Object names show up in real .knxprod files; anything not matched here
# falls back to a same-width raw integer type (see guess_go_member()).
_GO_KEYWORDS = (
    (
        ('temperature', 'temp'),
        'Type_DPT_Value_Temp',
        4,
        4,
        "guessed from the name (\"temp\"): DPT 9.001 float cache",
    ),
    (
        ('voltage', 'volt'),
        'Type_DPT_Value_Volt',
        4,
        4,
        "guessed from the name (\"volt\"): DPT 9.020 float cache",
    ),
)

_RAW_INT_BY_SIZE = {1: 'uint8_t', 2: 'uint16_t', 4: 'uint32_t', 8: 'uint64_t'}


def parse_object_size_bits(text):
    m = _OBJECT_SIZE_RE.match(text or '')
    if not m:
        return None
    n = int(m.group(1))
    return n if m.group(2).lower() == 'bit' else n * 8


def guess_go_member(name, bits):
    if bits == 1:
        return 'Type_DPT_Enable', 1, 1, "ObjectSize=1 Bit -> DPT 1.x boolean cache"

    lname = name.lower()
    for keywords, ctype, size, align, note in _GO_KEYWORDS:
        if any(k in lname for k in keywords):
            return ctype, size, align, note

    nbytes = max(1, (bits + 7) // 8)
    ctype = _RAW_INT_BY_SIZE.get(nbytes)
    note = (
        f"TODO: no name keyword matched a known DPT — defaulted to a raw "
        f"{nbytes}-byte type for ObjectSize={bits} bit(s); check the real DPT in ETS "
        "and add a Type_DPT_* alias to dpt.h if it's reusable"
    )
    if ctype:
        return ctype, nbytes, nbytes, note
    return f'uint8_t[{nbytes}]', nbytes, 1, note


_FLAG_LETTERS = (
    ('CommunicationFlag', 'C'),
    ('ReadFlag', 'R'),
    ('WriteFlag', 'W'),
    ('TransmitFlag', 'T'),
    ('UpdateFlag', 'U'),
    ('ReadOnInitFlag', 'I'),
)


def flag_summary(comobject):
    return ''.join(letter for attr, letter in _FLAG_LETTERS if comobject.get(attr) == 'Enabled')


def build_group_object_members(app_root):
    comobjects = sorted(
        iter_by_localname(app_root, 'ComObject'), key=lambda c: int(c.get('Number', '0'))
    )

    members = []
    seen_names = set()
    seen_macros = set()
    for c in comobjects:
        number = int(c.get('Number'))
        name = c.get('Name') or c.get('Text') or f"GO{number}"
        decl = dedupe(sanitize(name), seen_names)
        macro = dedupe("KNX_GO_" + sanitize(name, upper=True), seen_macros)

        bits = parse_object_size_bits(c.get('ObjectSize'))
        if bits is None:
            ctype, size, align = 'uint8_t', 1, 1
            note = "TODO: unparsable ObjectSize={!r}, defaulted to 1 byte".format(
                c.get('ObjectSize')
            )
        else:
            ctype, size, align, note = guess_go_member(name, bits)

        comment = (
            f"ASAP {number}, flags={flag_summary(c) or '-'} "
            f"(ObjectSize={c.get('ObjectSize')}) — {note}"
        )

        if '[' in ctype:
            base_ctype, bracket = ctype.split('[', 1)
            members.append(Member(f"{decl}[{bracket}", base_ctype, size, align, comment, macro))
        else:
            members.append(Member(decl, ctype, size, align, comment, macro))

    return members


# ---------------------------------------------------------------------------
# Rendering.
# ---------------------------------------------------------------------------


def c_comment_safe(text):
    """ETS free-text fields (Names, Ids, ObjectSize, ...) end up inside C
    block comments below. Neutralise any literal "*/" in them so it can't
    terminate the comment early and spill free text into the declaration."""
    return str(text).replace('*/', '* /')


def render_struct(struct_name, members):
    lines = [f"struct {struct_name} {{"]
    for m in members:
        decl_line = f"\t{m.ctype} {m.decl};"
        if m.comment:
            lines.append(f"{decl_line:<40} /* {c_comment_safe(m.comment)} */")
        else:
            lines.append(decl_line)
    lines.append("};")
    return '\n'.join(lines)


def render_go_macros(members):
    lines = [
        "/*",
        " * Group Object ASAP numbers, as assigned by the ETS application program",
        " * (Group Object Table order — must stay in sync with struct",
        " * group_objects_data above). Use these with the knx_group_object_* API",
        " * (knx_group_object.h).",
        " */",
    ]
    width = max((len(m.macro) for m in members), default=0)
    for i, m in enumerate(members, start=1):
        lines.append(f"#define {m.macro:<{width}}  {i}")
    return '\n'.join(lines)


def render_header(
    app_info, ap_members, ap_size_decl, ap_size_computed, go_members, go_size_computed, knxprod_path
):
    app_number = int(app_info['ApplicationNumber'])
    app_version = int(app_info['ApplicationVersion'])

    header = []
    header.append("/*")
    header.append(" * Copyright (c) 2026 Fabien Proriol")
    header.append(" * SPDX-License-Identifier: Apache-2.0")
    header.append(" *")
    header.append(" * GENERATED FILE — do not edit by hand.")
    header.append(" * Produced by zephyr/scripts/utils/generate_header_from_knxprod.py")
    header.append(f" * from {c_comment_safe(knxprod_path)},")
    header.append(
        f" * application program {c_comment_safe(app_info['Id'])!r} "
        f"({c_comment_safe(app_info.get('Name', ''))!r}, ApplicationNumber={app_number}, "
        f"ApplicationVersion={app_version},"
    )
    header.append(
        " * MaskVersion={}, PeiType={}).".format(
            c_comment_safe(app_info.get('MaskVersion', '?')),
            c_comment_safe(app_info.get('PeiType', '?')),
        )
    )
    header.append(" *")
    header.append(" * Regenerate with:")
    header.append(" *     zephyr/scripts/utils/generate_header_from_knxprod.py \\")
    header.append(f" *         {c_comment_safe(knxprod_path)} > <this file>")
    header.append(" *")
    header.append(" * Kconfig values this application's prj.conf must set to match:")
    header.append(f" *     CONFIG_KNX_APPLICATION_PROGRAM_ID        = 0x{app_number:04X}")
    header.append(f" *     CONFIG_KNX_APPLICATION_PROGRAM_VERSION   = 0x{app_version:02X}")
    size_mismatch = (
        ""
        if ap_size_decl == ap_size_computed
        else (
            f"; sizeof(struct application_program_data) computed here = {ap_size_computed}"
            " -- MISMATCH, check the struct above"
        )
    )
    header.append(
        f" *     CONFIG_KNX_APPLICATION_PROGRAM_DATA_SIZE = {ap_size_decl}"
        f" (.knxprod RelativeSegment Size{size_mismatch})"
    )
    header.append(f" *     CONFIG_KNX_GROUP_OBJECT_COUNT            = {len(go_members)}")
    header.append(
        f" *     CONFIG_KNX_GROUP_OBJECTS_DATA_SIZE       = {go_size_computed}"
        " (sizeof(struct group_objects_data) computed here)"
    )
    header.append(" *")
    header.append(" * DEVICE_PRODUCT_ID from the previous hand-written header is not")
    header.append(" * reproduced: it isn't derived from anything in the .knxprod (it reads")
    header.append(" * like a carrier-board hardware-revision marker) and nothing in")
    header.append(
        " * zephyr/subsys/knx/ or any application_w1, application_helios, ... daughterboard"
    )
    header.append(" * actually reads it. Add it back")
    header.append(" * by hand if some future code needs it.")
    header.append(" */")
    header.append("")
    header.append("#ifndef KNX_APP_DATA_H_")
    header.append("#define KNX_APP_DATA_H_")
    header.append("")
    header.append("#include <stdint.h>")
    header.append("#include <zephyr/knx/dpt.h>")
    header.append("#include <zephyr/knx/knx_app_data.h>")
    header.append("")
    header.append(render_struct("application_program_data", ap_members))
    header.append("")
    header.append(render_struct("group_objects_data", go_members))
    header.append("")
    header.append(render_go_macros(go_members))
    header.append("")
    header.append("#endif /* KNX_APP_DATA_H_ */")
    return '\n'.join(header) + '\n'


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
    parser.add_argument(
        '--program-ref',
        metavar='ID',
        help="Hardware2Program Id to use, if Hardware.xml declares "
        "more than one (e.g. several bus-medium variants)",
    )
    parser.add_argument(
        '-o', '--output', metavar='FILE', help="write the header here instead of stdout"
    )
    args = parser.parse_args()

    with zipfile.ZipFile(args.knxprod) as zf:
        hardware_member = find_hardware_member(zf)
        hw_root = parse_xml_member(zf, hardware_member)

        hw2p = pick_hardware2program(hw_root, args.program_ref)
        app_ref_elem = find_by_localname(hw2p, 'ApplicationProgramRef')
        if app_ref_elem is None:
            sys.exit(
                "error: Hardware2Program {!r} has no ApplicationProgramRef".format(hw2p.get('Id'))
            )
        app_ref = app_ref_elem.get('RefId')

        ap_member_path = find_application_program_member(zf, hardware_member, app_ref)
        app_root_doc = parse_xml_member(zf, ap_member_path)

    app_program = find_by_localname(app_root_doc, 'ApplicationProgram')
    if app_program is None:
        sys.exit(f"error: {ap_member_path} has no ApplicationProgram element")

    app_info = dict(app_program.attrib)

    code_segment, other_segments = pick_code_segment(app_program)
    if code_segment is None:
        sys.exit(
            "error: no <RelativeSegment> (Static/Code) found — nothing to "
            "generate struct application_program_data from"
        )
    if other_segments:
        for s in other_segments:
            print(
                "warning: extra RelativeSegment {!r} (Size={}) ignored, only {!r} is used".format(
                    s.get('Name'), s.get('Size'), code_segment.get('Name')
                ),
                file=sys.stderr,
            )

    ap_members = build_application_program_members(app_program, code_segment)
    ap_size_decl = int(code_segment.get('Size', '0'))
    ap_size_computed = struct_size(ap_members)
    if ap_size_computed != ap_size_decl:
        print(
            f"warning: sizeof(struct application_program_data) = {ap_size_computed} but the "
            f".knxprod RelativeSegment declares Size={ap_size_decl}",
            file=sys.stderr,
        )

    go_members = build_group_object_members(app_program)
    go_size_computed = struct_size(go_members)

    for m in ap_members + go_members:
        if m.comment.startswith('TODO') or 'TODO' in m.comment:
            print(
                "warning: {}{}: {}".format(m.decl, (f" ({m.macro})") if m.macro else "", m.comment),
                file=sys.stderr,
            )

    text = render_header(
        app_info,
        ap_members,
        ap_size_decl,
        ap_size_computed,
        go_members,
        go_size_computed,
        args.knxprod,
    )

    if args.output:
        with open(args.output, 'w') as f:
            f.write(text)
    else:
        sys.stdout.write(text)


if __name__ == '__main__':
    main()
