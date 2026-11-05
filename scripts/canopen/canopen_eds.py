# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""CiA 306 EDS reader and writer and the communication area of the Zephyr CANopen stack."""

import datetime
import math
import os
import re
import struct
from dataclasses import dataclass, field

VAR = 0x7
ARRAY = 0x8
RECORD = 0x9

BOOLEAN = 0x01
INTEGER8 = 0x02
INTEGER16 = 0x03
INTEGER32 = 0x04
UNSIGNED8 = 0x05
UNSIGNED16 = 0x06
UNSIGNED32 = 0x07
REAL32 = 0x08
VISIBLE_STRING = 0x09
OCTET_STRING = 0x0A
UNICODE_STRING = 0x0B
TIME_OF_DAY = 0x0C
TIME_DIFFERENCE = 0x0D
DOMAIN = 0x0F
INTEGER24 = 0x10
REAL64 = 0x11
INTEGER40 = 0x12
INTEGER48 = 0x13
INTEGER56 = 0x14
INTEGER64 = 0x15
UNSIGNED24 = 0x16
UNSIGNED40 = 0x18
UNSIGNED48 = 0x19
UNSIGNED56 = 0x1A
UNSIGNED64 = 0x1B

INT_SIZES = {
    BOOLEAN: 1,
    INTEGER8: 1,
    INTEGER16: 2,
    INTEGER24: 3,
    INTEGER32: 4,
    INTEGER40: 5,
    INTEGER48: 6,
    INTEGER56: 7,
    INTEGER64: 8,
    UNSIGNED8: 1,
    UNSIGNED16: 2,
    UNSIGNED24: 3,
    UNSIGNED32: 4,
    UNSIGNED40: 5,
    UNSIGNED48: 6,
    UNSIGNED56: 7,
    UNSIGNED64: 8,
}
SIGNED = {INTEGER8, INTEGER16, INTEGER24, INTEGER32, INTEGER40, INTEGER48, INTEGER56, INTEGER64}
FLOATS = {REAL32: 4, REAL64: 8}
STRINGS = {VISIBLE_STRING, OCTET_STRING, UNICODE_STRING}
SUPPORTED_TYPES = set(INT_SIZES) | set(FLOATS) | STRINGS | {DOMAIN}

ATTR_READ = 0x01
ATTR_WRITE = 0x02
ATTR_CONST = 0x04
ATTR_TPDO = 0x08
ATTR_RPDO = 0x10
ATTR_PERSIST = 0x20

ACCESS_ATTR = {
    "ro": ATTR_READ,
    "wo": ATTR_WRITE,
    "rw": ATTR_READ | ATTR_WRITE,
    "rwr": ATTR_READ | ATTR_WRITE,
    "rww": ATTR_READ | ATTR_WRITE,
    "const": ATTR_READ | ATTR_CONST,
}

COB_ID_INVALID = 0x80000000
COB_ID_NO_RTR = 0x40000000
PDO_PREDEFINED = 4
PDO_TYPE_EVENT = 0xFE


class EdsError(Exception):
    pass


@dataclass
class Value:
    value: int | float | str | bytes | None
    relative: bool = False


@dataclass
class Sub:
    sub: int
    name: str
    data_type: int
    attr: int
    default: Value = field(default_factory=lambda: Value(None))
    index: int = 0


@dataclass
class Obj:
    index: int
    name: str
    code: int
    subs: dict[int, Sub] = field(default_factory=dict)


def parse_int(text):
    text = text.strip().replace(" ", "")
    relative = False
    if text.upper().startswith("$NODEID"):
        relative = True
        text = text[len("$NODEID") :]
        offset = 0
        for term in text.split("+")[1:]:
            offset += parse_int(term).value
        if text and not text.startswith("+"):
            raise EdsError(f"invalid formula '$NODEID{text}'")
        return Value(offset, True)
    if re.fullmatch(r"-?0[xX][0-9a-fA-F]+", text):
        return Value(int(text, 16), relative)
    if re.fullmatch(r"-?0[0-7]+", text):
        return Value(int(text, 8), relative)
    if re.fullmatch(r"-?[0-9]+", text):
        return Value(int(text, 10), relative)
    raise EdsError(f"invalid number '{text}'")


def parse_value(data_type, text):
    if data_type == DOMAIN:
        return Value(None)
    if text is None or (text.strip() == "" and data_type not in STRINGS):
        return Value(None)
    if data_type == VISIBLE_STRING:
        return Value(text)
    if data_type in (OCTET_STRING, UNICODE_STRING):
        return Value(bytes.fromhex(text.strip()))
    if data_type in FLOATS:
        value = float(text)
        if not math.isfinite(value):
            raise EdsError(f"invalid number '{text}'")
        return Value(value)
    return parse_int(text)


def format_value(data_type, value):
    if value.value is None:
        return ""
    if value.relative:
        return f"$NODEID+0x{value.value:X}"
    if data_type == VISIBLE_STRING:
        return value.value
    if data_type in (OCTET_STRING, UNICODE_STRING):
        return value.value.hex()
    if data_type in FLOATS:
        return repr(float(value.value))
    if data_type in SIGNED:
        return str(value.value)
    return f"0x{value.value:X}"


def access_name(attr):
    if attr & ATTR_CONST:
        return "const"
    if (attr & (ATTR_READ | ATTR_WRITE)) == ATTR_READ:
        return "ro"
    if (attr & (ATTR_READ | ATTR_WRITE)) == ATTR_WRITE:
        return "wo"
    if (attr & (ATTR_TPDO | ATTR_RPDO)) == ATTR_TPDO:
        return "rwr"
    if (attr & (ATTR_TPDO | ATTR_RPDO)) == ATTR_RPDO:
        return "rww"
    return "rw"


def access_attr(access, mappable):
    access = access.strip().lower()
    if access not in ACCESS_ATTR:
        raise EdsError(f"invalid AccessType '{access}'")
    attr = ACCESS_ATTR[access]
    if mappable:
        if access == "rwr" or (attr & ATTR_WRITE) == 0:
            attr |= ATTR_TPDO
        elif access == "rww" or (attr & ATTR_READ) == 0:
            attr |= ATTR_RPDO
        else:
            attr |= ATTR_TPDO | ATTR_RPDO
    return attr


class EdsFile:
    def __init__(self, path):
        self.sections = {}
        current = None
        with open(path, encoding="ascii", errors="strict") as f:
            for number, line in enumerate(f, 1):
                line = line.rstrip("\r\n")
                if line.startswith(";") or line.strip() == "":
                    continue
                if line.startswith("["):
                    if not line.rstrip().endswith("]"):
                        raise EdsError(f"{path}:{number}: invalid section '{line}'")
                    current = self.sections.setdefault(line.strip()[1:-1].strip().lower(), {})
                    continue
                if current is None or "=" not in line:
                    raise EdsError(f"{path}:{number}: invalid line '{line}'")
                key, value = line.split("=", 1)
                current[key.strip().lower()] = value.strip()

    def get(self, section, key, fallback=None):
        return self.sections.get(section.lower(), {}).get(key.lower(), fallback)

    def has(self, section):
        return section.lower() in self.sections


def _index_sections(eds):
    found = {}
    for name in eds.sections:
        if re.fullmatch(r"[0-9a-f]{1,4}", name):
            found[int(name, 16)] = name
    return found


def _sub_sections(eds, index):
    found = {}
    for name in eds.sections:
        match = re.fullmatch(r"([0-9a-f]{1,4})sub([0-9a-f]{1,2})", name)
        if match and int(match.group(1), 16) == index:
            found[int(match.group(2), 16)] = name
    return found


def _read_sub(eds, section, index, sub, values):
    name = eds.get(section, "ParameterName")
    if name is None:
        raise EdsError(f"[{section}]: ParameterName missing")
    data_type = parse_int(eds.get(section, "DataType", "")).value
    if data_type not in SUPPORTED_TYPES:
        raise EdsError(f"{index:04X}h sub {sub}: data type 0x{data_type:04X} not supported")
    mappable = parse_int(eds.get(section, "PDOMapping", "0") or "0").value != 0
    attr = access_attr(eds.get(section, "AccessType", "rw"), mappable)
    if "PERSIST" in (eds.get(section, "StorageLocation", "") or "").upper():
        attr |= ATTR_PERSIST
    text = values.get(sub, eds.get(section, "ParameterValue", eds.get(section, "DefaultValue")))
    return Sub(sub, name, data_type, attr, parse_value(data_type, text), index)


def _compact_list(eds, section):
    entries = {}
    if eds.has(section):
        for key, value in eds.sections[section.lower()].items():
            if key != "nrofentries":
                entries[int(key, 10)] = value
    return entries


def read_eds(path):
    eds = EdsFile(path)
    objects = {}
    for index, section in sorted(_index_sections(eds).items()):
        name = eds.get(section, "ParameterName")
        if name is None:
            raise EdsError(f"[{section}]: ParameterName missing")
        code = parse_int(eds.get(section, "ObjectType", "0x7") or "0x7").value
        obj = Obj(index, name, code)
        if code == VAR:
            obj.subs[0] = _read_sub(eds, section, index, 0, {})
        elif code in (ARRAY, RECORD):
            compact = parse_int(eds.get(section, "CompactSubObj", "0") or "0").value
            if compact != 0:
                if code != ARRAY:
                    raise EdsError(f"{index:04X}h: CompactSubObj is only allowed for arrays")
                names = _compact_list(eds, f"{index:X}Name")
                values = _compact_list(eds, f"{index:X}Value")
                obj.subs[0] = Sub(0, "NrOfObjects", UNSIGNED8, ATTR_READ, Value(compact), index)
                for sub in range(1, compact + 1):
                    s = _read_sub(eds, section, index, sub, values)
                    s.name = names.get(sub, f"{name}{sub}")
                    obj.subs[sub] = s
            else:
                for sub, sub_section in sorted(_sub_sections(eds, index).items()):
                    obj.subs[sub] = _read_sub(eds, sub_section, index, sub, {})
                if 0 not in obj.subs:
                    raise EdsError(f"{index:04X}h: sub-index 0 missing")
        else:
            raise EdsError(f"{index:04X}h: object type 0x{code:X} not supported")
        objects[index] = obj
    return objects


def _ascii(text, limit):
    text = str(text)
    if not text.isascii() or "\n" in text or len(text) > limit:
        raise EdsError(f"'{text}' is not a valid EDS string")
    return text


def write_eds(path, objects, file_info, device_info):
    stamp = datetime.datetime.fromtimestamp(
        int(os.environ.get("SOURCE_DATE_EPOCH", datetime.datetime.now().timestamp())),
        datetime.UTC,
    )
    lines = []

    def section(name, entries):
        lines.append(f"[{name}]")
        lines.extend(f"{key}={value}" for key, value in entries)
        lines.append("")

    time = stamp.strftime("%I:%M%p")
    date = stamp.strftime("%m-%d-%Y")
    section(
        "FileInfo",
        [
            ("FileName", os.path.basename(path)),
            ("FileVersion", file_info.get("FileVersion", 1)),
            ("FileRevision", file_info.get("FileRevision", 0)),
            ("EDSVersion", "4.0"),
            ("Description", _ascii(file_info.get("Description", ""), 243)),
            ("CreationTime", time),
            ("CreationDate", date),
            ("CreatedBy", _ascii(file_info.get("CreatedBy", ""), 245)),
            ("ModificationTime", time),
            ("ModificationDate", date),
            ("ModifiedBy", _ascii(file_info.get("CreatedBy", ""), 244)),
        ],
    )
    section("DeviceInfo", list(device_info.items()))
    section("DummyUsage", [(f"Dummy{t:04X}", 1) for t in range(BOOLEAN, UNSIGNED32 + 1)])

    groups = {"MandatoryObjects": [], "OptionalObjects": [], "ManufacturerObjects": []}
    for index in sorted(objects):
        if index in (0x1000, 0x1001, 0x1018):
            groups["MandatoryObjects"].append(index)
        elif 0x2000 <= index <= 0x5FFF:
            groups["ManufacturerObjects"].append(index)
        else:
            groups["OptionalObjects"].append(index)

    for group, indices in groups.items():
        entries = [("SupportedObjects", len(indices))]
        entries += [(str(n), f"0x{index:04X}") for n, index in enumerate(indices, 1)]
        section(group, entries)
        for index in indices:
            _write_object(section, objects[index])

    with open(path, "w", encoding="ascii", newline="\r\n") as f:
        f.write("\n".join(lines))


def _sub_entries(sub):
    entries = [
        ("ParameterName", _ascii(sub.name, 241)),
        ("ObjectType", "0x7"),
        ("DataType", f"0x{sub.data_type:04X}"),
        ("AccessType", access_name(sub.attr)),
        ("DefaultValue", format_value(sub.data_type, sub.default)),
        ("PDOMapping", 1 if sub.attr & (ATTR_TPDO | ATTR_RPDO) else 0),
    ]
    if sub.attr & ATTR_PERSIST:
        entries.append(("StorageLocation", "PERSIST"))
    return entries


def _write_object(section, obj):
    if obj.code == VAR:
        entries = _sub_entries(obj.subs[0])
        entries[0] = ("ParameterName", _ascii(obj.name, 241))
        section(f"{obj.index:X}", entries)
        return
    section(
        f"{obj.index:X}",
        [
            ("ParameterName", _ascii(obj.name, 241)),
            ("ObjectType", f"0x{obj.code:X}"),
            ("SubNumber", len(obj.subs)),
        ],
    )
    for sub in sorted(obj.subs):
        section(f"{obj.index:X}sub{sub:X}", _sub_entries(obj.subs[sub]))


def read_kconfig(path):
    config = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            match = re.match(r"(CONFIG_\w+)=(.*)$", line.strip())
            if match is None:
                continue
            value = match.group(2)
            if value.startswith('"'):
                value = re.sub(r"\\(.)", r"\1", value[1:-1])
            config[match.group(1)] = value
    return config


def kconfig_int(config, name, default=0):
    return int(config.get(name, str(default)), 0)


def kconfig_bool(config, name):
    return config.get(name) == "y"


@dataclass
class PdoConfig:
    cob_id: int = 0
    transmission_type: int = PDO_TYPE_EVENT
    inhibit_time: int = 0
    event_timer: int = 0
    sync_start: int = 0
    mapping: list[int] = field(default_factory=list)


@dataclass
class NodeValues:
    tpdo: list[PdoConfig] = field(default_factory=list)
    rpdo: list[PdoConfig] = field(default_factory=list)
    hb_consumer: list[int] = field(default_factory=list)
    serial_number: int = 0


RO = ATTR_READ
RW = ATTR_READ | ATTR_WRITE
CONST = ATTR_READ | ATTR_CONST


def _var(index, name, data_type, attr, value, relative=False):
    return Obj(index, name, VAR, {0: Sub(0, name, data_type, attr, Value(value, relative))})


def _subs(index, name, code, entries):
    obj = Obj(index, name, code)
    for sub, (sub_name, data_type, attr, value, *relative) in enumerate(entries):
        if sub_name is not None:
            obj.subs[sub] = Sub(sub, sub_name, data_type, attr, Value(value, bool(relative)))
    return obj


def _pdo_objects(n, tx, cfg, mapped_max):
    base = 0x1800 if tx else 0x1400
    kind = "TPDO" if tx else "RPDO"
    cob = COB_ID_INVALID
    relative = n < PDO_PREDEFINED
    if relative:
        cob |= (0x180 if tx else 0x200) + n * 0x100
    if cfg is not None:
        if (cfg.cob_id & 0x7FF) != 0 or not relative:
            cob = cfg.cob_id
            relative = False
        else:
            cob = (cob & 0x7FF) | cfg.cob_id
    if tx:
        cob |= COB_ID_NO_RTR
    cfg = cfg or PdoConfig()
    comm = [
        ("highest sub-index supported", UNSIGNED8, CONST, 6 if tx else 5),
        (f"COB-ID used by {kind}", UNSIGNED32, RW, cob, *([True] if relative else [])),
        ("transmission type", UNSIGNED8, RW, cfg.transmission_type),
        ("inhibit time", UNSIGNED16, RW, cfg.inhibit_time),
        (None, None, None, None),
        ("event timer" if tx else "event-timer", UNSIGNED16, RW, cfg.event_timer),
    ]
    if tx:
        comm.append(("SYNC start value", UNSIGNED8, RW, cfg.sync_start))
    mapping = [
        (
            f"number of mapped application objects in {'TPDO' if tx else 'PDO'}",
            UNSIGNED8,
            RW,
            len(cfg.mapping),
        )
    ]
    for i in range(mapped_max):
        value = cfg.mapping[i] if i < len(cfg.mapping) else 0
        mapping.append((f"application object {i + 1}", UNSIGNED32, RW, value))
    return [
        _subs(base + n, f"{kind} communication parameter", RECORD, comm),
        _subs(
            base + 0x200 + n,
            "TPDO mapping" if tx else "RPDO mapping parameter",
            RECORD,
            mapping,
        ),
    ]


def comm_objects(config, node):
    objects = [
        _var(
            0x1000,
            "Device type",
            UNSIGNED32,
            CONST,
            kconfig_int(config, "CONFIG_CANOPEN_DEVICE_TYPE"),
        ),
        _var(0x1001, "Error register", UNSIGNED8, RO, 0),
    ]
    history = kconfig_int(config, "CONFIG_CANOPEN_EMCY_HISTORY_SIZE")
    objects.append(
        _subs(
            0x1003,
            "Pre-defined error field",
            ARRAY,
            [("Number of errors", UNSIGNED8, RW, 0)]
            + [("Standard error field", UNSIGNED32, RO, 0)] * history,
        )
    )
    objects += [
        _var(0x1005, "COB-ID SYNC", UNSIGNED32, RW, 0x80),
        _var(0x1006, "Communication cycle period", UNSIGNED32, RW, 0),
        _var(0x1007, "Synchronous window length", UNSIGNED32, RW, 0),
    ]
    if kconfig_bool(config, "CONFIG_CANOPEN_NODE_GUARDING"):
        objects += [
            _var(0x100C, "Guard time", UNSIGNED16, RW, 0),
            _var(0x100D, "Life time factor", UNSIGNED8, RW, 0),
        ]
    if kconfig_bool(config, "CONFIG_CANOPEN_TIME"):
        objects.append(_var(0x1012, "COB-ID time stamp message", UNSIGNED32, RW, 0x80000100))
    for index, option, name in (
        (0x1008, "CONFIG_CANOPEN_DEVICE_NAME", "Manufacturer device name"),
        (0x1009, "CONFIG_CANOPEN_HW_VERSION", "Manufacturer hardware version"),
        (0x100A, "CONFIG_CANOPEN_SW_VERSION", "Manufacturer software version"),
    ):
        if config.get(option, ""):
            objects.append(_var(index, name, VISIBLE_STRING, CONST, config[option]))
    if kconfig_bool(config, "CONFIG_CANOPEN_STORAGE"):
        for index, name, verb in (
            (0x1010, "store parameters", "save"),
            (0x1011, "restore default parameters", "restore"),
        ):
            default = " default" if verb == "restore" else ""
            objects.append(
                _subs(
                    index,
                    name,
                    ARRAY,
                    [
                        ("highest sub-index supported", UNSIGNED8, CONST, 4),
                        (f"{verb} all{default} parameters", UNSIGNED32, RW, 1),
                        (f"{verb} communication{default} parameters", UNSIGNED32, RW, 1),
                        (f"{verb} application{default} parameters", UNSIGNED32, RW, 1),
                        (f"{verb} manufacturer defined{default} parameters", UNSIGNED32, RW, 1),
                    ],
                )
            )
    objects += [
        _var(0x1014, "COB-ID emergency message", UNSIGNED32, RW, 0x80, True),
        _var(0x1015, "inhibit time EMCY", UNSIGNED16, RW, 0),
    ]
    count = kconfig_int(config, "CONFIG_CANOPEN_HB_CONSUMER_COUNT")
    objects.append(
        _subs(
            0x1016,
            "Consumer heartbeat time",
            ARRAY,
            [("Highest sub-index supported", UNSIGNED8, CONST, count)]
            + [
                (
                    "Consumer heartbeat time",
                    UNSIGNED32,
                    RW,
                    node.hb_consumer[i] if i < len(node.hb_consumer) else 0,
                )
                for i in range(count)
            ],
        )
    )
    objects.append(
        _var(
            0x1017,
            "Producer heartbeat time",
            UNSIGNED16,
            RW,
            kconfig_int(config, "CONFIG_CANOPEN_HEARTBEAT_PRODUCER_MS"),
        )
    )
    objects.append(
        _subs(
            0x1018,
            "Identity object",
            RECORD,
            [
                ("Highest sub-index supported", UNSIGNED8, CONST, 4),
                ("Vendor-ID", UNSIGNED32, RO, kconfig_int(config, "CONFIG_CANOPEN_VENDOR_ID")),
                (
                    "Product code",
                    UNSIGNED32,
                    RO,
                    kconfig_int(config, "CONFIG_CANOPEN_PRODUCT_CODE"),
                ),
                (
                    "Revision number",
                    UNSIGNED32,
                    RO,
                    kconfig_int(config, "CONFIG_CANOPEN_REVISION_NUMBER"),
                ),
                ("Serial number", UNSIGNED32, RO, node.serial_number),
            ],
        )
    )
    objects += [
        _var(0x1019, "Synchronous counter overflow value", UNSIGNED8, RW, 0),
        _subs(
            0x1029,
            "Error behavior",
            ARRAY,
            [
                ("Highest sub-index supported", UNSIGNED8, CONST, 1),
                ("Communication error", UNSIGNED8, RW, 0),
            ],
        ),
        _subs(
            0x1200,
            "SDO server parameter",
            RECORD,
            [
                ("Highest sub-index supported", UNSIGNED8, CONST, 2),
                ("COB-ID client -> server (rx)", UNSIGNED32, CONST, 0x600, True),
                ("COB-ID server -> client (tx)", UNSIGNED32, CONST, 0x580, True),
            ],
        ),
    ]
    if kconfig_bool(config, "CONFIG_CANOPEN_MASTER"):
        objects.append(
            _subs(
                0x1280,
                "SDO client parameter",
                RECORD,
                [
                    ("Highest sub-index supported", UNSIGNED8, CONST, 3),
                    ("COB-ID client -> server (tx)", UNSIGNED32, RW, COB_ID_INVALID),
                    ("COB-ID server -> client (rx)", UNSIGNED32, RW, COB_ID_INVALID),
                    ("Node-ID of the SDO server", UNSIGNED8, RW, 1),
                ],
            )
        )
    mapped_max = kconfig_int(config, "CONFIG_CANOPEN_PDO_MAPPED_MAX")
    for tx, option, cfgs in (
        (False, "CONFIG_CANOPEN_RPDO_COUNT", node.rpdo),
        (True, "CONFIG_CANOPEN_TPDO_COUNT", node.tpdo),
    ):
        for n in range(kconfig_int(config, option)):
            objects += _pdo_objects(n, tx, cfgs[n] if n < len(cfgs) else None, mapped_max)
    return {obj.index: obj for obj in sorted(objects, key=lambda o: o.index)}


def device_info(config, vendor_name, bitrates):
    info = {
        "VendorName": _ascii(vendor_name, 244),
        "VendorNumber": f"0x{kconfig_int(config, 'CONFIG_CANOPEN_VENDOR_ID'):X}",
        "ProductName": _ascii(config.get("CONFIG_CANOPEN_DEVICE_NAME", ""), 243),
        "ProductNumber": f"0x{kconfig_int(config, 'CONFIG_CANOPEN_PRODUCT_CODE'):X}",
        "RevisionNumber": f"0x{kconfig_int(config, 'CONFIG_CANOPEN_REVISION_NUMBER'):X}",
        "OrderCode": "",
    }
    for rate in (10, 20, 50, 125, 250, 500, 800, 1000):
        info[f"BaudRate_{rate}"] = 1 if rate in bitrates else 0
    info.update(
        {
            "SimpleBootUpMaster": 0,
            "SimpleBootUpSlave": 1,
            "Granularity": 8,
            "DynamicChannelsSupported": 0,
            "GroupMessaging": 0,
            "NrOfRXPDO": kconfig_int(config, "CONFIG_CANOPEN_RPDO_COUNT"),
            "NrOfTXPDO": kconfig_int(config, "CONFIG_CANOPEN_TPDO_COUNT"),
            "LSS_Supported": 0,
        }
    )
    return info


def unpack_native(data_type, raw, little_endian=True):
    if data_type == VISIBLE_STRING:
        return raw.split(b"\0", 1)[0].decode("ascii")
    if data_type in (OCTET_STRING, UNICODE_STRING):
        return bytes(raw)
    if data_type in FLOATS:
        return struct.unpack(
            ("<" if little_endian else ">") + ("f" if len(raw) == 4 else "d"), raw
        )[0]
    value = int.from_bytes(raw, "little" if little_endian else "big", signed=data_type in SIGNED)
    if data_type in SIGNED:
        bits = INT_SIZES[data_type] * 8
        value = ((value + (1 << (bits - 1))) % (1 << bits)) - (1 << (bits - 1))
    else:
        value &= (1 << (INT_SIZES[data_type] * 8)) - 1
    return value
