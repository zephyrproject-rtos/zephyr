#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""Generate a Zephyr CANopen object dictionary from a CiA 306 EDS or DCF.

The objects outside the communication area become variables and a CANOPEN_OD_DEFINE()
table. The PDO and heartbeat consumer defaults become configuration arrays for
struct canopen_node_config. The communication objects of the EDS are checked against
the objects the stack provides with the Kconfig options of the build.
"""

import argparse
import os
import re
import sys

import canopen_eds as eds

COMM_FIRST = 0x1000
COMM_LAST = 0x1FFF
DUMMY_LAST = 0x001B

C_TYPES = {
    eds.BOOLEAN: "bool",
    eds.INTEGER8: "int8_t",
    eds.INTEGER16: "int16_t",
    eds.INTEGER24: "int32_t",
    eds.INTEGER32: "int32_t",
    eds.INTEGER40: "int64_t",
    eds.INTEGER48: "int64_t",
    eds.INTEGER56: "int64_t",
    eds.INTEGER64: "int64_t",
    eds.UNSIGNED8: "uint8_t",
    eds.UNSIGNED16: "uint16_t",
    eds.UNSIGNED24: "uint32_t",
    eds.UNSIGNED32: "uint32_t",
    eds.UNSIGNED40: "uint64_t",
    eds.UNSIGNED48: "uint64_t",
    eds.UNSIGNED56: "uint64_t",
    eds.UNSIGNED64: "uint64_t",
    eds.REAL32: "float",
    eds.REAL64: "double",
    eds.VISIBLE_STRING: "char",
    eds.OCTET_STRING: "uint8_t",
    eds.UNICODE_STRING: "uint8_t",
}

TYPE_NAMES = {
    eds.BOOLEAN: "BOOLEAN",
    eds.INTEGER8: "INTEGER8",
    eds.INTEGER16: "INTEGER16",
    eds.INTEGER24: "INTEGER24",
    eds.INTEGER32: "INTEGER32",
    eds.INTEGER40: "INTEGER40",
    eds.INTEGER48: "INTEGER48",
    eds.INTEGER56: "INTEGER56",
    eds.INTEGER64: "INTEGER64",
    eds.UNSIGNED8: "UNSIGNED8",
    eds.UNSIGNED16: "UNSIGNED16",
    eds.UNSIGNED24: "UNSIGNED24",
    eds.UNSIGNED32: "UNSIGNED32",
    eds.UNSIGNED40: "UNSIGNED40",
    eds.UNSIGNED48: "UNSIGNED48",
    eds.UNSIGNED56: "UNSIGNED56",
    eds.UNSIGNED64: "UNSIGNED64",
    eds.REAL32: "REAL32",
    eds.REAL64: "REAL64",
    eds.VISIBLE_STRING: "VISIBLE_STRING",
    eds.OCTET_STRING: "OCTET_STRING",
    eds.UNICODE_STRING: "UNICODE_STRING",
    eds.DOMAIN: "DOMAIN",
}


def where(index, sub=None):
    return f"{index:04X}h" + ("" if sub is None else f" sub-index {sub}")


def same_access(a, b):
    readonly = {"ro", "const"}
    a = eds.access_name(a)
    b = eds.access_name(b)
    return a == b or {a, b} <= readonly


def same_value(expected, found):
    if found.value is None:
        return True
    return expected.value == found.value and expected.relative == found.relative


def compare_comm(model, objects, errors):
    for index in sorted(set(model) | {i for i in objects if COMM_FIRST <= i <= COMM_LAST}):
        if index not in objects:
            errors.append(f"{where(index)}: provided by the stack but missing in the EDS")
            continue
        if index not in model:
            errors.append(f"{where(index)}: not provided by the stack with this configuration")
            continue
        m, o = model[index], objects[index]
        if m.code != o.code:
            errors.append(f"{where(index)}: object type 0x{o.code:X}, stack has 0x{m.code:X}")
            continue
        for sub in sorted(set(m.subs) | set(o.subs)):
            if sub not in o.subs:
                errors.append(f"{where(index, sub)}: provided by the stack but missing")
            elif sub not in m.subs:
                errors.append(f"{where(index, sub)}: not provided by the stack")
            else:
                ms, os_ = m.subs[sub], o.subs[sub]
                if ms.data_type != os_.data_type:
                    errors.append(
                        f"{where(index, sub)}: data type 0x{os_.data_type:04X}, "
                        f"stack has 0x{ms.data_type:04X}"
                    )
                if not same_access(ms.attr, os_.attr):
                    errors.append(
                        f"{where(index, sub)}: AccessType "
                        f"{eds.access_name(os_.attr)}, stack has "
                        f"{eds.access_name(ms.attr)}"
                    )
                if (ms.attr ^ os_.attr) & (eds.ATTR_TPDO | eds.ATTR_RPDO):
                    errors.append(f"{where(index, sub)}: PDOMapping differs from the stack")
                if not same_value(ms.default, os_.default):
                    errors.append(
                        f"{where(index, sub)}: value "
                        f"{eds.format_value(os_.data_type, os_.default)}, stack has "
                        f"{eds.format_value(ms.data_type, ms.default)}"
                    )


def default_int(sub):
    if sub.default.value is None:
        return 0
    if sub.default.relative:
        raise eds.EdsError(f"{where(sub.index, sub.sub)}: $NODEID is not supported here")
    return sub.default.value


def pdo_config(objects, n, tx, errors):
    comm = objects.get((0x1800 if tx else 0x1400) + n)
    mapping = objects.get((0x1A00 if tx else 0x1600) + n)
    if comm is None or mapping is None:
        return None
    cfg = eds.PdoConfig()
    cob = comm.subs.get(1)
    if cob is not None and cob.default.value is not None:
        value = cob.default.value
        if cob.default.relative:
            predefined = (0x180 if tx else 0x200) + n * 0x100
            if n >= eds.PDO_PREDEFINED or (value & 0x7FF) != predefined:
                errors.append(
                    f"{where(comm.index, 1)}: only the pre-defined $NODEID COB-ID is supported"
                )
            value &= ~0x7FF
        if tx and not value & eds.COB_ID_NO_RTR:
            errors.append(f"{where(comm.index, 1)}: bit 30 must be set, RTR is not supported")
        cfg.cob_id = value
    for sub, attr in (
        (2, "transmission_type"),
        (3, "inhibit_time"),
        (5, "event_timer"),
        (6, "sync_start"),
    ):
        if sub in comm.subs and comm.subs[sub].default.value is not None:
            setattr(cfg, attr, default_int(comm.subs[sub]))
    count = default_int(mapping.subs[0]) if 0 in mapping.subs else 0
    for sub in range(1, count + 1):
        if sub not in mapping.subs:
            errors.append(f"{where(mapping.index, sub)}: mapping entry missing")
            break
        cfg.mapping.append(default_int(mapping.subs[sub]))
    return cfg


def node_values(objects, config, errors):
    node = eds.NodeValues()
    for tx, option, target in (
        (True, "CONFIG_CANOPEN_TPDO_COUNT", node.tpdo),
        (False, "CONFIG_CANOPEN_RPDO_COUNT", node.rpdo),
    ):
        configs = []
        for n in range(eds.kconfig_int(config, option)):
            cfg = pdo_config(objects, n, tx, errors)
            configs.append(cfg)
        none = eds.PdoConfig(cob_id=eds.COB_ID_INVALID | (eds.COB_ID_NO_RTR if tx else 0))
        while configs and (configs[-1] is None or configs[-1] == none):
            configs.pop()
        target.extend(cfg or none for cfg in configs)
    hb = objects.get(0x1016)
    if hb is not None:
        node.hb_consumer = [default_int(s) for sub, s in sorted(hb.subs.items()) if sub > 0]
        while node.hb_consumer and node.hb_consumer[-1] == 0:
            node.hb_consumer.pop()
    identity = objects.get(0x1018)
    if identity is not None and 4 in identity.subs:
        node.serial_number = default_int(identity.subs[4])
    return node


def c_name(prefix, name, used):
    base = re.sub(r"[^0-9a-z]+", "_", name.lower()).strip("_")
    ident = prefix + (f"_{base}" if base else "")
    candidate = ident
    n = 2
    while candidate in used:
        candidate = f"{ident}_{n}"
        n += 1
    used.add(candidate)
    return candidate


def c_value(sub):
    t = sub.data_type
    if t == eds.VISIBLE_STRING:
        text = sub.default.value or ""
        return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if t in (eds.OCTET_STRING, eds.UNICODE_STRING):
        return "{" + ", ".join(f"0x{b:02X}U" for b in sub.default.value) + "}"
    if t in eds.FLOATS:
        value = float(sub.default.value or 0)
        return repr(value) + ("f" if t == eds.REAL32 else "")
    value = default_int(sub)
    if t == eds.BOOLEAN:
        return "true" if value else "false"
    bits = eds.INT_SIZES[t] * 8
    if t in eds.SIGNED:
        if not -(1 << (bits - 1)) <= value < (1 << (bits - 1)):
            raise eds.EdsError(f"{where(sub.index, sub.sub)}: value {value} out of range")
        if value == -(1 << 63):
            return "INT64_MIN"
        return f"{value}" + ("LL" if bits > 32 else "")
    if not 0 <= value < (1 << bits):
        raise eds.EdsError(f"{where(sub.index, sub.sub)}: value {value} out of range")
    return f"0x{value:X}U" + ("LL" if bits > 32 else "")


def c_attr(attr):
    names = {
        eds.ATTR_READ | eds.ATTR_CONST: "CANOPEN_OD_CONST",
        eds.ATTR_READ: "CANOPEN_OD_RO",
        eds.ATTR_WRITE: "CANOPEN_OD_WO",
        eds.ATTR_READ | eds.ATTR_WRITE: "CANOPEN_OD_RW",
    }
    parts = [names[attr & (eds.ATTR_READ | eds.ATTR_WRITE | eds.ATTR_CONST)]]
    for flag, name in (
        (eds.ATTR_TPDO, "CANOPEN_OD_ATTR_TPDO"),
        (eds.ATTR_RPDO, "CANOPEN_OD_ATTR_RPDO"),
        (eds.ATTR_PERSIST, "CANOPEN_OD_ATTR_PERSIST"),
    ):
        if attr & flag:
            parts.append(name)
    return " | ".join(parts)


def c_decl(sub, ident):
    if sub.data_type == eds.VISIBLE_STRING:
        if not sub.default.value:
            raise eds.EdsError(
                f"{where(sub.index, sub.sub)}: a string needs a DefaultValue, "
                f"its length sets the size"
            )
        return f"char {ident}[{len(sub.default.value) + 1}]"
    if sub.data_type in (eds.OCTET_STRING, eds.UNICODE_STRING):
        if not sub.default.value:
            raise eds.EdsError(
                f"{where(sub.index, sub.sub)}: a string needs a DefaultValue, "
                f"its length sets the size"
            )
        return f"uint8_t {ident}[{len(sub.default.value)}]"
    return f"{C_TYPES[sub.data_type]} {ident}"


def check_mapping(objects, cfg, tx, label, errors):
    bits = 0
    for entry in cfg.mapping:
        index, sub, length = entry >> 16, (entry >> 8) & 0xFF, entry & 0xFF
        bits += length
        if index <= DUMMY_LAST and sub == 0:
            if tx or eds.INT_SIZES.get(index, 0) * 8 != length:
                errors.append(f"{label}: dummy entry 0x{entry:08X} not valid")
            continue
        target = objects.get(index)
        s = target.subs.get(sub) if target is not None else None
        if target is not None and target.code == eds.VAR and sub != 0:
            s = None
        if s is None or not s.attr & (eds.ATTR_TPDO if tx else eds.ATTR_RPDO):
            errors.append(f"{label}: 0x{entry:08X} does not name a mappable object")
        elif length % 8 or (
            s.data_type in eds.INT_SIZES and eds.INT_SIZES[s.data_type] * 8 != length
        ):
            errors.append(f"{label}: 0x{entry:08X} has the wrong length")
    if bits > 64:
        errors.append(f"{label}: mapping exceeds 8 bytes")


def generate(objects, name, config, source):
    errors = []
    node = node_values(objects, config, errors)
    compare_comm(eds.comm_objects(config, node), objects, errors)
    app = [objects[i] for i in sorted(objects) if not COMM_FIRST <= i <= COMM_LAST]
    for index in sorted(objects):
        if index < COMM_FIRST:
            errors.append(f"{where(index)}: data type area objects are not supported")

    used = set()
    decls = []
    entries = []
    for obj in app:
        if obj.index < COMM_FIRST:
            continue
        ident = c_name(f"x{obj.index:04X}", obj.name, used)
        if obj.code == eds.VAR and obj.subs[0].data_type == eds.DOMAIN:
            s = obj.subs[0]
            if s.attr & (eds.ATTR_TPDO | eds.ATTR_RPDO | eds.ATTR_PERSIST):
                errors.append(f"{where(obj.index)}: a DOMAIN cannot be mapped or stored")
            decls.append(f"extern const struct canopen_od_domain {ident};")
            entries.append(f"CANOPEN_OD_DOMAIN(0x{obj.index:04X}, {c_attr(s.attr)}, {ident}, NULL)")
            continue
        if obj.code == eds.VAR:
            s = obj.subs[0]
            decls.append(f"static {c_decl(s, ident)} = {c_value(s)};")
            entries.append(
                f"CANOPEN_OD_VAR(0x{obj.index:04X}, CANOPEN_OD_TYPE_"
                f"{TYPE_NAMES[s.data_type]}, {c_attr(s.attr)}, {ident})"
            )
            continue
        subs = [s for sub, s in sorted(obj.subs.items()) if sub > 0]
        if any(s.data_type == eds.DOMAIN for s in subs):
            errors.append(f"{where(obj.index)}: a DOMAIN is only supported as VAR")
            continue
        if not subs:
            errors.append(f"{where(obj.index)}: no sub-index besides 0")
            continue
        if obj.code == eds.ARRAY:
            highest = obj.subs[0]
            first = subs[0]
            if [s.sub for s in subs] != list(range(1, len(subs) + 1)):
                errors.append(f"{where(obj.index)}: array sub-indices must be 1 to n")
            elif any(s.data_type != first.data_type or s.attr != first.attr for s in subs):
                errors.append(f"{where(obj.index)}: array entries differ in type or access")
            elif first.data_type in eds.STRINGS:
                errors.append(f"{where(obj.index)}: arrays of strings are not supported")
            if highest.default.value not in (None, len(subs)):
                errors.append(f"{where(obj.index, 0)}: must be {len(subs)}")
            if errors:
                continue
            values = ", ".join(c_value(s) for s in subs)
            decls.append(f"static {C_TYPES[first.data_type]} {ident}[{len(subs)}] = {{{values}}};")
            entries.append(
                f"CANOPEN_OD_ARRAY(0x{obj.index:04X}, CANOPEN_OD_TYPE_"
                f"{TYPE_NAMES[first.data_type]}, {c_attr(first.attr)}, {ident})"
            )
            continue
        if obj.subs[0].default.value not in (None, subs[-1].sub):
            errors.append(f"{where(obj.index, 0)}: must be {subs[-1].sub}")
            continue
        members = []
        init = []
        refs = []
        member_names = set()
        for s in subs:
            member = c_name(f"sub{s.sub}", s.name, member_names)
            members.append(f"\t{c_decl(s, member)};")
            init.append(f"\t.{member} = {c_value(s)},")
            refs.append(
                f"\t\tCANOPEN_OD_SUB({s.sub}U, CANOPEN_OD_TYPE_{TYPE_NAMES[s.data_type]}, "
                f"{c_attr(s.attr)}, {ident}.{member})"
            )
        decls.append(
            "static struct {\n"
            + "\n".join(members)
            + f"\n}} {ident} = {{\n"
            + "\n".join(init)
            + "\n};"
        )
        entries.append(f"CANOPEN_OD_RECORD(0x{obj.index:04X},\n" + ",\n".join(refs) + ")")

    for tx, kind, cfgs in ((True, "tpdo", node.tpdo), (False, "rpdo", node.rpdo)):
        for n, cfg in enumerate(cfgs):
            check_mapping(objects, cfg, tx, f"{kind.upper()}{n + 1}", errors)

    if errors:
        raise eds.EdsError(f"{source}:\n  " + "\n  ".join(errors))

    upper = name.upper()
    lines = [
        f"/* Generated by eds2od.py from {os.path.basename(source)}, do not edit. */",
        "",
        f"#ifndef {upper}_H_",
        f"#define {upper}_H_",
        "",
        "#include <stdbool.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/canbus/canopen.h>",
        "#include <zephyr/toolchain.h>",
        "",
    ]
    lines += decls
    if entries:
        lines += ["", f"CANOPEN_OD_DEFINE({name}" + "".join(f",\n\t{e}" for e in entries) + ");"]
    else:
        lines += [f"static const struct canopen_od {name};"]
    lines.append("")
    for kind, cfgs in (("tpdo", node.tpdo), ("rpdo", node.rpdo)):
        for n, cfg in enumerate(cfgs):
            if cfg.mapping:
                values = ", ".join(f"0x{m:08X}U" for m in cfg.mapping)
                lines.append(f"static const uint32_t {name}_{kind}{n + 1}_map[] = {{{values}}};")
        if cfgs:
            lines.append(
                f"static const struct canopen_pdo_config {name}_{kind}[] __maybe_unused = {{"
            )
            for n, cfg in enumerate(cfgs):
                mapping = (
                    f"\t\t.map = {name}_{kind}{n + 1}_map,\n"
                    f"\t\t.map_count = ARRAY_SIZE({name}_{kind}{n + 1}_map),\n"
                    if cfg.mapping
                    else ""
                )
                lines.append(
                    "\t{\n"
                    + mapping
                    + f"\t\t.cob_id = 0x{cfg.cob_id:08X}U,\n"
                    + f"\t\t.inhibit_time = {cfg.inhibit_time}U,\n"
                    + f"\t\t.event_timer = {cfg.event_timer}U,\n"
                    + f"\t\t.transmission_type = 0x{cfg.transmission_type:02X}U,\n"
                    + f"\t\t.sync_start = {cfg.sync_start}U,\n\t}},"
                )
            lines.append("};")
            lines.append(f"#define {upper}_{kind.upper()} {name}_{kind}")
            lines.append(f"#define {upper}_{kind.upper()}_COUNT ARRAY_SIZE({name}_{kind})")
        else:
            lines.append(f"#define {upper}_{kind.upper()} NULL")
            lines.append(f"#define {upper}_{kind.upper()}_COUNT 0U")
        lines.append("")
    if node.hb_consumer:
        values = ", ".join(f"0x{v:08X}U" for v in node.hb_consumer)
        lines.append(f"static const uint32_t {name}_hb_consumer[] __maybe_unused = {{{values}}};")
        lines.append(f"#define {upper}_HB_CONSUMER {name}_hb_consumer")
        lines.append(f"#define {upper}_HB_CONSUMER_COUNT ARRAY_SIZE({name}_hb_consumer)")
    else:
        lines.append(f"#define {upper}_HB_CONSUMER NULL")
        lines.append(f"#define {upper}_HB_CONSUMER_COUNT 0U")
    lines.append(f"#define {upper}_SERIAL_NUMBER 0x{node.serial_number:08X}U")
    lines += ["", f"#endif /* {upper}_H_ */", ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--config", required=True, help=".config of the build")
    parser.add_argument("--name", required=True, help="name of the generated object dictionary")
    parser.add_argument("eds", help="EDS or DCF file")
    parser.add_argument("output", help="header file to write")
    args = parser.parse_args()

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.name):
        sys.exit(f"eds2od: '{args.name}' is not a C identifier")

    try:
        text = generate(eds.read_eds(args.eds), args.name, eds.read_kconfig(args.config), args.eds)
    except (eds.EdsError, OSError, ValueError) as e:
        sys.exit(f"eds2od: {e}")

    with open(args.output, "w", encoding="utf-8") as f:
        f.write(text)


if __name__ == "__main__":
    main()
