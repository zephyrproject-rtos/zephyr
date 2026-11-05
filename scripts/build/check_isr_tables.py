#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Check linked ISR placement against the generated layout manifest.

The manifest describes storage and aliases, not the identity of ISR targets.
Runtime tests or explicit target expectations must check handler/argument values.
Only ELF-visible section/symbol names can identify unselected placeholder storage.
"""

import argparse
import json
import sys
from contextlib import ExitStack
from pathlib import Path

from elftools.elf.elffile import ELFFile

MANIFEST_VERSION = 1
DIAGNOSTIC_LIMIT = 5
DIAGNOSTIC_VALUE_CHARACTERS = 200
SHF_ALLOC = 0x2
SHF_WRITE = 0x1
SHF_EXECINSTR = 0x4
PUBLIC_BINDINGS = {"STB_GLOBAL", "STB_WEAK"}
ADDRESS_SENSITIVE_TYPES = {"STT_FUNC", "STT_OBJECT"}
ISR_LINKONCE_SECTIONS = (
    ".gnu.linkonce.sw_isr_table",
    ".gnu.linkonce.shared_sw_isr_table",
    ".gnu.linkonce.irq_vector_table",
)
PLACEHOLDER_SECTION_PREFIXES = (
    ".isr_pre0",
    ".text.isr_pre0",
    ".isr_placeholder",
    ".isr_table_placeholder",
    ".isr_vector_owners",
    ".isr_vector_refs",
)
PLACEHOLDER_SYMBOL_PREFIXES = (
    "z_isr_pre0_",
    "__isr_pre0",
    "__isr_placeholder",
    "__isr_table_placeholder",
)
METADATA_SECTION_PREFIXES = (".intList", "intList", ".irq_info", "isr_declarations")
METADATA_POLICIES = {"absent", "nonallocated"}
CAPACITY_BOUNDARIES = (
    ("_kobject_rodata_area_start", "_kobject_rodata_area_end"),
    ("z_priv_stacks_ram_start", "z_priv_stacks_ram_end"),
)
RESERVED_SECTIONS = ("kobject_data", ".kobject_data", "priv_stacks_noinit")
# kernel/userspace/userspace.c places these weak intermediate implementations
# in .kobject_data.text.dummies; final gperf code occupies the reserved area.
KOBJECT_LOOKUP_STUBS = {
    "z_object_gperf_find",
    "z_object_gperf_wordlist_foreach",
    "k_object_find",
    "k_object_wordlist_foreach",
}


class InputError(ValueError):
    """The ELF or manifest cannot be interpreted."""


def integer(value, name):
    if type(value) is not int or value < 0:
        raise InputError(f"{name} must be a nonnegative integer")


def alignment(value, name):
    integer(value, name)
    if value == 0 or value & (value - 1):
        raise InputError(f"{name} must be a positive power of two")


def name(value, field):
    if not isinstance(value, str) or not value or "\0" in value:
        raise InputError(f"{field} must be a nonempty name")


def validate_manifest(manifest):
    if (
        not isinstance(manifest, dict)
        or type(manifest.get("version")) is not int
        or manifest["version"] != MANIFEST_VERSION
    ):
        raise InputError(f"expected layout version {MANIFEST_VERSION}")
    if not isinstance(manifest.get("tables"), list):
        raise InputError("tables must be an array")
    if manifest.get("metadata_policy", "absent") not in METADATA_POLICIES:
        raise InputError("unsupported ISR metadata policy")
    table_names = set()
    input_names = set()
    for table in manifest["tables"]:
        if not isinstance(table, dict):
            raise InputError("table must be an object")
        for field in ("section", "symbol", "end_symbol"):
            name(table.get(field), field)
        if "elf_section" in table:
            name(table["elf_section"], "elf_section")
        if table["symbol"] in table_names:
            raise InputError(f"duplicate table: {table['symbol']}")
        table_names.add(table["symbol"])
        integer(table.get("size"), "table size")
        alignment(table.get("alignment"), "table alignment")
        if "writable" in table and type(table["writable"]) is not bool:
            raise InputError("table writable must be a boolean")
        if not isinstance(table.get("inputs"), list):
            raise InputError("table inputs must be an array")
        intervals = []
        for item in table["inputs"]:
            if not isinstance(item, dict):
                raise InputError("table input must be an object")
            for field in ("name", "symbol"):
                name(item.get(field), f"input {field}")
            if item["name"] in input_names:
                raise InputError(f"input selected more than once: {item['name']}")
            input_names.add(item["name"])
            integer(item.get("offset"), "input offset")
            integer(item.get("size"), "input size")
            alignment(item.get("alignment"), "input alignment")
            intervals.append((item["offset"], item["offset"] + item["size"]))
        intervals.sort()
        if any(left[1] > right[0] for left, right in zip(intervals, intervals[1:], strict=False)):
            raise InputError(f"overlapping inputs in {table['symbol']}")
    manual_vectors = manifest.get("manual_vectors", [])
    if not isinstance(manual_vectors, list):
        raise InputError("manual_vectors must be an array")
    for table in manual_vectors:
        if not isinstance(table, dict):
            raise InputError("manual vector table must be an object")
        name(table.get("symbol"), "manual vector symbol")
        if table["symbol"] in table_names:
            raise InputError(f"duplicate table: {table['symbol']}")
        table_names.add(table["symbol"])
        integer(table.get("size"), "manual vector size")
        if table["size"] == 0:
            raise InputError("manual vector size must be positive")
        alignment(table.get("alignment"), "manual vector alignment")
        if type(table.get("writable")) is not bool:
            raise InputError("manual vector writable must be a boolean")


class Image:
    def __init__(self, stream):
        self.elf = ELFFile(stream)
        self.sections = {section.name: section for section in self.elf.iter_sections()}
        self.storage_sections = [
            section
            for section in self.elf.iter_sections()
            if section["sh_flags"] & SHF_ALLOC and section["sh_size"]
        ]
        symtab = self.elf.get_section_by_name(".symtab")
        if symtab is None:
            raise InputError("ELF has no .symtab; preserve symbols for ISR validation")
        self.symbols = {}
        for symbol in symtab.iter_symbols():
            if symbol.name and symbol["st_shndx"] != "SHN_UNDEF":
                record = {
                    "value": symbol["st_value"],
                    "size": symbol["st_size"],
                    "type": symbol["st_info"]["type"],
                    "binding": symbol["st_info"]["bind"],
                    "section": symbol["st_shndx"],
                }
                if record not in self.symbols.setdefault(symbol.name, []):
                    self.symbols[symbol.name].append(record)

    def unique(self, symbol):
        records = self.symbols.get(symbol, [])
        return records[0] if len(records) == 1 else None

    def public_objects(self):
        result = {}
        for symbol, records in self.symbols.items():
            public = [record for record in records if record["binding"] in PUBLIC_BINDINGS]
            if (
                len(public) == 1
                and public[0]["type"] in ADDRESS_SENSITIVE_TYPES
                and isinstance(public[0]["section"], int)
            ):
                result[symbol] = public[0]
        return result


class Checks:
    def __init__(self, report, prefix=""):
        self.report = report
        self.prefix = prefix

    def equal(self, label, actual, expected):
        passed = actual == expected
        self.report["checks"].append(
            {"name": self.prefix + label, "passed": passed, "actual": actual, "expected": expected}
        )
        return passed

    def symbol(self, image, symbol, label):
        result = image.unique(symbol)
        if not self.equal(f"{label}.unique_symbol", result is not None, True):
            count = len(image.symbols.get(symbol, []))
            self.report["errors"].append(
                {"code": "symbol_resolution", "message": f"{symbol}: {count} definitions"}
            )
        if result is not None:
            section = result["section"]
            if not self.equal(
                f"{label}.defined_section",
                section == "SHN_ABS"
                or isinstance(section, int)
                and 0 < section < image.elf.num_sections(),
                True,
            ):
                return None
        return result


def check_storage(image, address, size, label, checks, writable=None):
    """Check a logical array across the ELF sections covering its address range."""
    limit = address + size
    fragments = sorted(
        (
            (
                max(address, section["sh_addr"]),
                min(limit, section["sh_addr"] + section["sh_size"]),
                section,
            )
            for section in image.storage_sections
            if section["sh_addr"] < limit and address < section["sh_addr"] + section["sh_size"]
        ),
        key=lambda item: item[0],
    )
    checks.equal(label + ".allocated", bool(fragments), True)
    cursor = address
    for index, (begin, end, section) in enumerate(fragments):
        part = f"{label}.fragment.{index}"
        checks.equal(part + ".contiguous", begin, cursor)
        cursor = max(cursor, end)
        if writable is not None:
            checks.equal(part + ".writable", bool(section["sh_flags"] & SHF_WRITE), writable)
        checks.equal(part + ".file_backed", section["sh_type"] != "SHT_NOBITS", True)
        if section["sh_type"] != "SHT_NOBITS":
            offset = begin - section["sh_addr"]
            checks.equal(
                part + ".file_bytes",
                len(section.data()[offset : offset + end - begin]),
                end - begin,
            )
    checks.equal(label + ".storage_end", cursor, limit)


def check_symbol_section(image, symbol, size, label, checks, expected=None, address=None):
    """Absolute linker aliases have no section; real section bindings must be valid."""
    if not isinstance(symbol["section"], int):
        checks.equal(label + ".absolute_symbol", symbol["section"], "SHN_ABS")
        return
    section = image.elf.get_section(symbol["section"])
    if address is None:
        address = symbol["value"]
    if expected is not None:
        checks.equal(label + ".symbol_section", section.name, expected)
    else:
        checks.equal(
            label + ".symbol_section",
            bool(section["sh_flags"] & SHF_ALLOC)
            and section["sh_addr"] <= address
            and address + size <= section["sh_addr"] + section["sh_size"],
            True,
        )


def check_table(image, table, checks):
    label = table["symbol"]
    start = checks.symbol(image, label, label)
    end = checks.symbol(image, table["end_symbol"], label + ".end")
    if start is None or end is None:
        return
    address = start["value"]
    checks.equal(label + ".size", end["value"] - address, table["size"])
    checks.equal(label + ".alignment", address % table["alignment"], 0)
    if start["size"] != 0:
        checks.equal(label + ".object_size", start["size"], table["size"])
    expected = table.get("elf_section")
    if expected is not None:
        section = image.sections.get(expected)
        if not checks.equal(label + ".output_section_exists", section is not None, True):
            return
        checks.equal(
            label + ".within_section",
            section["sh_addr"]
            <= address
            <= end["value"]
            <= section["sh_addr"] + section["sh_size"],
            True,
        )
    # A zero-size logical table needs no allocated storage.
    if table["size"] == 0:
        checks.equal(label + ".empty_inputs", table["inputs"], [])
        return
    check_symbol_section(image, start, start["size"], label, checks, expected)
    check_storage(image, address, table["size"], label, checks, table.get("writable"))
    for item in table["inputs"]:
        item_label = f"{label}.input.{item['name']}"
        item_address = address + item["offset"]
        checks.equal(
            item_label + ".within_table", item["offset"] + item["size"] <= table["size"], True
        )
        checks.equal(item_label + ".alignment", item_address % item["alignment"], 0)
        alias = checks.symbol(image, item["symbol"], item_label)
        if alias is None:
            continue
        checks.equal(item_label + ".address", alias["value"], item_address)
        used_end = checks.symbol(image, item["symbol"] + "_end", item_label + ".end")
        if used_end is not None:
            checks.equal(
                item_label + ".used_size", used_end["value"] - alias["value"], item["size"]
            )
        if alias["size"] != 0:
            checks.equal(item_label + ".object_size", alias["size"], item["size"])
        check_symbol_section(image, alias, item["size"], item_label, checks, expected, item_address)
        check_storage(image, item_address, item["size"], item_label, checks, table.get("writable"))


def read_allocated_bytes(image, address, size):
    """Read initialized target bytes, including arrays split across output sections."""
    result = bytearray()
    cursor = address
    for section in sorted(image.storage_sections, key=lambda item: item["sh_addr"]):
        begin, end = section["sh_addr"], section["sh_addr"] + section["sh_size"]
        if end <= cursor or begin >= address + size:
            continue
        if begin > cursor or section["sh_type"] == "SHT_NOBITS":
            raise InputError("Shared ISR clients have no initialized storage")
        limit = min(end, address + size)
        data = section.data()[cursor - begin : limit - begin]
        if len(data) != limit - cursor:
            raise InputError("Truncated shared ISR client storage")
        result.extend(data)
        cursor = limit
    if cursor != address + size:
        raise InputError("Incomplete shared ISR client storage")
    return bytes(result)


def check_shared_clients(image, manifest, checks):
    """Retain the legacy prohibition on duplicate ISR/argument registrations."""
    tables = [table for table in manifest["tables"] if table["section"] == "shared_sw_isr_table"]
    if not tables:
        return
    metadata = manifest.get("metadata", {})
    fields = (
        "entry_size",
        "arg_offset",
        "arg_size",
        "isr_offset",
        "isr_size",
        "shared_size",
        "shared_count_offset",
        "shared_count_size",
    )
    for field in fields:
        integer(metadata.get(field), "shared metadata " + field)
    entry_size, shared_size = metadata["entry_size"], metadata["shared_size"]
    count_offset, count_size = metadata["shared_count_offset"], metadata["shared_count_size"]
    if not entry_size or not shared_size or not count_size:
        raise InputError("Invalid shared ISR structure extent")
    if count_offset + count_size > shared_size:
        raise InputError("Shared ISR count exceeds its structure")
    for field in ("arg", "isr"):
        if not metadata[field + "_size"] or (
            metadata[field + "_offset"] + metadata[field + "_size"] > entry_size
        ):
            raise InputError("Invalid shared ISR " + field + " extent")
    order = "little" if image.elf.little_endian else "big"
    for table in tables:
        start = checks.symbol(image, table["symbol"], "shared." + table["symbol"])
        if start is None:
            continue
        if table["size"] % shared_size:
            raise InputError("Shared ISR table has partial slots")
        for index in range(table["size"] // shared_size):
            label = f"shared.{table['symbol']}.slot.{index}"
            slot = read_allocated_bytes(image, start["value"] + index * shared_size, shared_size)
            count = int.from_bytes(slot[count_offset : count_offset + count_size], order)
            if not checks.equal(label + ".count_fits", count * entry_size <= count_offset, True):
                continue
            clients = set()
            for client in range(count):
                base = client * entry_size
                pair = tuple(
                    slot[
                        base + metadata[field + "_offset"] : base
                        + metadata[field + "_offset"]
                        + metadata[field + "_size"]
                    ]
                    for field in ("isr", "arg")
                )
                checks.equal(f"{label}.client.{client}.unique_pair", pair not in clients, True)
                clients.add(pair)


def check_residue(image, checks):
    sections = [
        section
        for section in image.sections
        if section.startswith(ISR_LINKONCE_SECTIONS + PLACEHOLDER_SECTION_PREFIXES)
    ]
    symbols = [symbol for symbol in image.symbols if symbol.startswith(PLACEHOLDER_SYMBOL_PREFIXES)]
    checks.equal("final.no_isr_linkonce_or_placeholder_sections", sorted(sections), [])
    checks.equal("final.no_placeholder_symbols", sorted(symbols), [])


def check_metadata(image, manifest, checks):
    sections = [
        section
        for name, section in image.sections.items()
        if name.startswith(METADATA_SECTION_PREFIXES)
    ]
    if manifest.get("metadata_policy", "absent") == "absent":
        checks.equal("metadata.absent", sorted(section.name for section in sections), [])
        return
    for section in sections:
        label = "metadata." + section.name
        checks.equal(label + ".nonallocated", section["sh_flags"] & SHF_ALLOC, 0)
        checks.equal(label + ".nonexecutable", section["sh_flags"] & SHF_EXECINSTR, 0)
        begin = section["sh_offset"]
        end = begin + section["sh_size"]
        loaded = any(
            segment["p_type"] == "PT_LOAD"
            and (
                segment.section_in_segment(section)
                or (
                    section["sh_size"] > 0
                    and begin < segment["p_offset"] + segment["p_filesz"]
                    and segment["p_offset"] < end
                )
            )
            for segment in image.elf.iter_segments()
        )
        checks.equal(label + ".outside_load_segments", loaded, False)


def check_manual_vectors(image, tables, checks):
    for table in tables:
        label = "manual." + table["symbol"]
        symbol = checks.symbol(image, table["symbol"], label)
        if symbol is None:
            continue
        checks.equal(label + ".object_type", symbol["type"], "STT_OBJECT")
        checks.equal(label + ".size", symbol["size"], table["size"])
        checks.equal(label + ".alignment", symbol["value"] % table["alignment"], 0)
        section = (
            image.elf.get_section(symbol["section"]) if isinstance(symbol["section"], int) else None
        )
        if not checks.equal(label + ".section_exists", section is not None, True):
            continue
        checks.equal(label + ".allocated", bool(section["sh_flags"] & SHF_ALLOC), True)
        checks.equal(label + ".writable", bool(section["sh_flags"] & SHF_WRITE), table["writable"])
        offset = symbol["value"] - section["sh_addr"]
        checks.equal(
            label + ".within_section",
            offset >= 0 and offset + symbol["size"] <= section["sh_size"],
            True,
        )
        checks.equal(label + ".file_backed", section["sh_type"] != "SHT_NOBITS", True)
        if section["sh_type"] != "SHT_NOBITS" and offset >= 0:
            checks.equal(
                label + ".file_bytes",
                len(section.data()[offset : offset + symbol["size"]]),
                table["size"],
            )


def check_capacities(prelinked, final, checks):
    for start_name, used_end_name in CAPACITY_BOUNDARIES:
        start = prelinked.unique(start_name)
        reserved_end = prelinked.unique(used_end_name)
        if start is None or reserved_end is None:
            continue
        final_start = checks.symbol(final, start_name, "capacity." + start_name)
        final_used_end = checks.symbol(final, used_end_name, "capacity." + used_end_name)
        if final_start is None or final_used_end is None:
            continue
        checks.equal("capacity." + start_name + ".start", final_start["value"], start["value"])
        checks.equal(
            "capacity." + start_name + ".used_fits",
            start["value"] <= final_used_end["value"] <= reserved_end["value"],
            True,
        )
    for section_name in RESERVED_SECTIONS:
        before = prelinked.sections.get(section_name)
        if before is None:
            continue
        after = final.sections.get(section_name)
        if checks.equal("capacity." + section_name + ".exists", after is not None, True):
            checks.equal("capacity." + section_name + ".start", after["sh_addr"], before["sh_addr"])
            checks.equal(
                "capacity." + section_name + ".reserved_size",
                after["sh_size"],
                before["sh_size"],
            )
            if section_name in ("kobject_data", ".kobject_data"):
                used_start = final.unique("_kobject_data_area_start")
                used_end = final.unique("_kobject_data_area_end")
                if used_start is not None and used_end is not None:
                    checks.equal(
                        "capacity.kobject_data.used_fits",
                        before["sh_addr"]
                        <= used_start["value"]
                        <= used_end["value"]
                        <= before["sh_addr"] + before["sh_size"],
                        True,
                    )


def check_stages(prelinked, final, manifest, checks):
    checks.equal("stage.elf_class", final.elf.elfclass, prelinked.elf.elfclass)
    checks.equal("stage.endianness", final.elf.little_endian, prelinked.elf.little_endian)
    before = prelinked.public_objects()
    after = final.public_objects()
    common = sorted(before.keys() & after.keys())
    for symbol in common:
        if (
            symbol in KOBJECT_LOOKUP_STUBS
            and before[symbol]["binding"] == "STB_WEAK"
            and after[symbol]["binding"] == "STB_GLOBAL"
            and before[symbol]["type"] == after[symbol]["type"] == "STT_FUNC"
        ):
            check_lookup_replacement(
                prelinked,
                final,
                symbol,
                before[symbol],
                after[symbol],
                checks,
            )
            continue
        for field in ("type", "value", "size"):
            checks.equal(f"stage.{symbol}.{field}", after[symbol][field], before[symbol][field])
    checks.report["stage"] = {
        "public_symbols_compared": len(common),
        "symbols_only_prelinked": sorted(before.keys() - after.keys()),
        "symbols_only_final": sorted(after.keys() - before.keys()),
    }
    # Deliberately do not compare every STT_NOTYPE end/used marker: kobject
    # used ends may move inside fixed reservations, and gperf adds new symbols.
    for table in manifest["tables"]:
        for field in ("symbol", "end_symbol"):
            symbol = table[field]
            earlier = checks.symbol(prelinked, symbol, "stage.prelinked." + symbol)
            later = checks.symbol(final, symbol, "stage.final." + symbol)
            if earlier is not None and later is not None:
                checks.equal("stage.boundary." + symbol, later["value"], earlier["value"])
    check_capacities(prelinked, final, checks)


def check_lookup_replacement(prelinked, final, symbol, before, after, checks):
    label = f"stage.{symbol}.generated_lookup"
    areas = []
    for phase, image, function in (("prelinked", prelinked, before), ("final", final, after)):
        phase_label = f"{label}.{phase}"
        start = checks.symbol(image, "_kobject_text_area_start", phase_label + ".start")
        used_end = checks.symbol(image, "_kobject_text_area_end", phase_label + ".used_end")
        budget = checks.symbol(image, "CONFIG_KOBJECT_TEXT_AREA", phase_label + ".budget")
        if start is None or used_end is None or budget is None:
            return
        base, capacity = start["value"], budget["value"]
        checks.equal(phase_label + ".positive_budget", capacity > 0, True)
        checks.equal(phase_label + ".used_fits", base <= used_end["value"] <= base + capacity, True)
        # ARM function symbols encode Thumb state in bit zero, not in storage.
        address = function["value"]
        if image.elf["e_machine"] == "EM_ARM":
            address &= ~1
        checks.equal(
            phase_label + ".function_fits",
            function["size"] > 0
            and base <= address
            and address + function["size"] <= used_end["value"],
            True,
        )
        section = image.elf.get_section(function["section"])
        checks.equal(
            phase_label + ".allocated_executable",
            section["sh_flags"] & (SHF_ALLOC | SHF_EXECINSTR),
            SHF_ALLOC | SHF_EXECINSTR,
        )
        checks.equal(
            phase_label + ".reservation_within_section",
            section["sh_addr"] <= base
            and base + capacity <= section["sh_addr"] + section["sh_size"],
            True,
        )
        areas.append((base, capacity))
    checks.equal(label + ".reservation_stable", areas[1], areas[0])


def run(elf_path, manifest_path, prelinked_path=None):
    """Return (exit status, report); 0 passes, 1 violates layout, 2 is invalid input."""
    report = {
        "version": 1,
        "elf": str(elf_path),
        "manifest": str(manifest_path),
        "prelinked": None if prelinked_path is None else str(prelinked_path),
        "status": "input_error",
        "checks": [],
        "errors": [],
    }
    try:
        manifest = json.loads(Path(manifest_path).read_text(encoding="utf-8"))
        validate_manifest(manifest)
        with ExitStack() as stack:
            final = Image(stack.enter_context(Path(elf_path).open("rb")))
            checks = Checks(report)
            checks.equal("final.executable", final.elf["e_type"], "ET_EXEC")
            report["elf_class"] = final.elf.elfclass
            report["byte_order"] = "little" if final.elf.little_endian else "big"
            for table in manifest["tables"]:
                check_table(final, table, checks)
            check_shared_clients(final, manifest, checks)
            check_residue(final, checks)
            check_metadata(final, manifest, checks)
            check_manual_vectors(final, manifest.get("manual_vectors", []), checks)
            if prelinked_path is not None:
                prelinked = Image(stack.enter_context(Path(prelinked_path).open("rb")))
                for table in manifest["tables"]:
                    # The same manifest must describe the real-table prelink too.
                    check_table(prelinked, table, Checks(report, "prelinked."))
                check_shared_clients(prelinked, manifest, Checks(report, "prelinked."))
                check_metadata(prelinked, manifest, Checks(report, "prelinked."))
                check_manual_vectors(
                    prelinked, manifest.get("manual_vectors", []), Checks(report, "prelinked.")
                )
                check_stages(prelinked, final, manifest, checks)
    except Exception as error:
        report["errors"].append({"code": "invalid_input", "message": str(error)})
        return 2, report
    passed = all(check["passed"] for check in report["checks"]) and not report["errors"]
    report["status"] = "pass" if passed else "fail"
    return (0 if passed else 1), report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--manifest", "--layout", required=True, type=Path)
    parser.add_argument("--prelinked", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args(argv)
    result, report = run(args.elf, args.manifest, args.prelinked)
    output = json.dumps(report, indent=2) + "\n"
    if args.report is None:
        print(output, end="")
    else:
        try:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(output, encoding="utf-8")
        except OSError as error:
            print(f"cannot write ISR report: {error}", file=sys.stderr)
            return 2
    if result != 0:
        failed = [check for check in report["checks"] if not check["passed"]]
        print(
            f"ISR table validation failed: {len(failed)} checks, {len(report['errors'])} errors",
            file=sys.stderr,
        )
        for check in failed[:DIAGNOSTIC_LIMIT]:
            actual = json.dumps(check["actual"])[:DIAGNOSTIC_VALUE_CHARACTERS]
            expected = json.dumps(check["expected"])[:DIAGNOSTIC_VALUE_CHARACTERS]
            print(f"  {check['name']}: actual={actual}, expected={expected}", file=sys.stderr)
        for error in report["errors"][: max(0, DIAGNOSTIC_LIMIT - len(failed))]:
            print(f"  {error['code']}: {error['message']}", file=sys.stderr)
        print(
            f"JSON report: {args.report if args.report is not None else 'stdout'}",
            file=sys.stderr,
        )
    return result


if __name__ == "__main__":
    sys.exit(main())
