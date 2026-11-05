#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""Generate the CiA 306 EDS of a Zephyr CANopen node from the linked image.

The application object dictionary, the PDO and heartbeat consumer configuration and the
serial number are read from the node configuration in the ELF file, the communication
objects are derived from the Kconfig options of the build.
"""

import argparse
import bisect
import re
import struct
import sys

import canopen_eds as eds
from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


class Image:
    def __init__(self, path):
        self.file = open(path, "rb")  # noqa: SIM115
        self.elf = ELFFile(self.file)
        if self.elf["e_type"] != "ET_EXEC":
            raise eds.EdsError(f"{path}: not a linked executable")
        self.little = self.elf.little_endian
        self.ptr = self.elf.elfclass // 8
        self.symbols = {}
        objects = []
        for section in self.elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue
            for sym in section.iter_symbols():
                if sym["st_info"]["type"] != "STT_OBJECT" or sym["st_size"] == 0:
                    continue
                name = re.sub(r"\.(lto_priv\.)?\d+$", "", sym.name)
                self.symbols.setdefault(name, []).append(sym)
                objects.append((sym["st_value"], sym["st_size"], name))
        objects.sort()
        self.objects = objects
        self.starts = [o[0] for o in objects]

    def symbol(self, name):
        syms = self.symbols.get(name, [])
        if len(syms) != 1:
            raise eds.EdsError(f"symbol '{name}' found {len(syms)} times, expected once")
        return syms[0]["st_value"], syms[0]["st_size"]

    def owner(self, addr):
        for i in range(bisect.bisect_right(self.starts, addr) - 1, -1, -1):
            start, size, name = self.objects[i]
            if start <= addr < start + size:
                return start, size, name
        return None

    def read(self, addr, size):
        for section in self.elf.iter_sections():
            if not section["sh_flags"] & SH_FLAGS.SHF_ALLOC:
                continue
            start = section["sh_addr"]
            if start <= addr and addr + size <= start + section["sh_size"]:
                if section["sh_type"] == "SHT_NOBITS":
                    return bytes(size)
                offset = addr - start
                return section.data()[offset : offset + size]
        raise eds.EdsError(f"address 0x{addr:x} is not in the image")


class Layout:
    def __init__(self, image, fields):
        self.image = image
        self.offsets = {}
        offset = 0
        align = 1
        for name, kind in fields:
            size = image.ptr if kind == "p" else struct.calcsize(kind)
            offset = (offset + size - 1) // size * size
            self.offsets[name] = (offset, kind, size)
            offset += size
            align = max(align, size)
        self.size = (offset + align - 1) // align * align

    def read(self, addr):
        raw = self.image.read(addr, self.size)
        values = {}
        for name, (offset, kind, size) in self.offsets.items():
            fmt = ("<" if self.image.little else ">") + (
                {4: "I", 8: "Q"}[size] if kind == "p" else kind
            )
            values[name] = struct.unpack_from(fmt, raw, offset)[0]
        return values


def layouts(image):
    return {
        "node": Layout(
            image,
            [
                ("can", "p"),
                ("od", "p"),
                ("nmt_state_cb", "p"),
                ("reset_app_cb", "p"),
                ("rpdo_cb", "p"),
                ("sync_cb", "p"),
                ("hb_cb", "p"),
                ("time_cb", "p"),
                ("hb_consumer", "p"),
                ("tpdo", "p"),
                ("rpdo", "p"),
                ("tpdo_count", "H"),
                ("rpdo_count", "H"),
                ("hb_consumer_count", "B"),
                ("led_run", "p"),
                ("led_err", "p"),
                ("led_bicolor", "B"),
                ("user_data", "p"),
                ("serial_number", "I"),
                ("node_id", "B"),
            ],
        ),
        "od": Layout(image, [("entries", "p"), ("count", "p")]),
        "entry": Layout(
            image,
            [
                ("subs", "p"),
                ("write_hook", "p"),
                ("user_data", "p"),
                ("index", "H"),
                ("code", "B"),
                ("count", "B"),
            ],
        ),
        "sub": Layout(
            image,
            [("data", "p"), ("size", "H"), ("sub", "B"), ("type", "B"), ("attr", "B")],
        ),
        "pdo": Layout(
            image,
            [
                ("map", "p"),
                ("cob_id", "I"),
                ("inhibit_time", "H"),
                ("event_timer", "H"),
                ("transmission_type", "B"),
                ("sync_start", "B"),
                ("map_count", "B"),
            ],
        ),
    }


def read_u32_array(image, addr, count):
    raw = image.read(addr, 4 * count)
    return list(struct.unpack(("<" if image.little else ">") + "I" * count, raw))


def read_pdos(image, lay, addr, count):
    pdos = []
    for n in range(count):
        p = lay["pdo"].read(addr + n * lay["pdo"].size)
        mapping = read_u32_array(image, p["map"], p["map_count"]) if p["map_count"] else []
        pdos.append(
            eds.PdoConfig(
                p["cob_id"],
                p["transmission_type"],
                p["inhibit_time"],
                p["event_timer"],
                p["sync_start"],
                mapping,
            )
        )
    return pdos


def sub_default(image, sub, addr):
    if sub["type"] not in eds.SUPPORTED_TYPES:
        raise eds.EdsError(f"data type 0x{sub['type']:02X} not supported")
    if sub["type"] == eds.DOMAIN:
        return eds.Value(None)
    raw = image.read(addr, sub["size"])
    return eds.Value(eds.unpack_native(sub["type"], raw, image.little))


def app_objects(image, lay, od_addr):
    od = lay["od"].read(od_addr)
    objects = {}
    for i in range(od["count"]):
        entry = lay["entry"].read(od["entries"] + i * lay["entry"].size)
        index = entry["index"]
        subs = [
            lay["sub"].read(entry["subs"] + j * lay["sub"].size)
            for j in range(1 if entry["code"] == eds.ARRAY else entry["count"])
        ]
        owner = image.owner(subs[0]["data"])
        name = owner[2] if owner else f"Object {index:04X}h"
        obj = eds.Obj(index, name, entry["code"])
        if entry["code"] == eds.VAR:
            s = subs[0]
            obj.subs[0] = eds.Sub(0, name, s["type"], s["attr"], sub_default(image, s, s["data"]))
        elif entry["code"] == eds.ARRAY:
            s = subs[0]
            obj.subs[0] = eds.Sub(
                0,
                "Highest sub-index supported",
                eds.UNSIGNED8,
                eds.CONST,
                eds.Value(entry["count"]),
            )
            for n in range(1, entry["count"] + 1):
                addr = s["data"] + (n - 1) * s["size"]
                obj.subs[n] = eds.Sub(
                    n, f"{name}{n}", s["type"], s["attr"], sub_default(image, s, addr)
                )
        else:
            shared = all(owner and image.owner(s["data"]) == owner for s in subs)
            if not shared:
                obj.name = f"Object {index:04X}h"
            if subs[0]["sub"] != 0:
                obj.subs[0] = eds.Sub(
                    0,
                    "Highest sub-index supported",
                    eds.UNSIGNED8,
                    eds.CONST,
                    eds.Value(subs[-1]["sub"]),
                )
            for s in subs:
                sub_owner = image.owner(s["data"])
                if not shared and sub_owner and sub_owner[0] == s["data"]:
                    sub_name = sub_owner[2]
                else:
                    sub_name = f"{obj.name}{s['sub']}"
                obj.subs[s["sub"]] = eds.Sub(
                    s["sub"], sub_name, s["type"], s["attr"], sub_default(image, s, s["data"])
                )
        objects[index] = obj
    return objects


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--elf", required=True, help="linked image")
    parser.add_argument("--config", required=True, help=".config of the build")
    parser.add_argument(
        "--node-config", required=True, help="name of the struct canopen_node_config"
    )
    parser.add_argument("--vendor-name", default="", help="VendorName of the EDS")
    parser.add_argument("--bitrates", default="", help="supported bit rates in kbit/s")
    parser.add_argument("output", help="EDS file to write")
    args = parser.parse_args()

    try:
        config = eds.read_kconfig(args.config)
        image = Image(args.elf)
        lay = layouts(image)
        addr, size = image.symbol(args.node_config)
        if size != lay["node"].size:
            raise eds.EdsError(
                f"'{args.node_config}' has {size} bytes, struct canopen_node_config has "
                f"{lay['node'].size}"
            )
        node_cfg = lay["node"].read(addr)
        node = eds.NodeValues(
            read_pdos(image, lay, node_cfg["tpdo"], node_cfg["tpdo_count"]),
            read_pdos(image, lay, node_cfg["rpdo"], node_cfg["rpdo_count"]),
            read_u32_array(image, node_cfg["hb_consumer"], node_cfg["hb_consumer_count"])
            if node_cfg["hb_consumer_count"]
            else [],
            node_cfg["serial_number"],
        )
        objects = eds.comm_objects(config, node)
        objects.update(app_objects(image, lay, node_cfg["od"]))
        bitrates = [int(rate) for rate in re.split(r"[\s,]+", args.bitrates.strip()) if rate]
        eds.write_eds(
            args.output,
            objects,
            {"Description": config.get("CONFIG_CANOPEN_DEVICE_NAME", ""), "CreatedBy": "Zephyr"},
            eds.device_info(config, args.vendor_name, bitrates),
        )
    except (eds.EdsError, OSError) as e:
        sys.exit(f"od2eds: {e}")


if __name__ == "__main__":
    main()
