#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Make armlink's dedicated ISR metadata overlay nonallocated.

armlink has no nonloaded scatter region. The overlay contains build records,
not executable data: detach only its program headers and section allocation
flags, retaining all payload bytes, symbols, addresses, and relocations.
"""

import argparse
import hashlib
import json
import os
import struct
import tempfile
from io import BytesIO
from pathlib import Path

from elftools.common.exceptions import ELFError
from elftools.elf.descriptions import describe_reloc_type
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from gen_isr_tables_parser_local import METADATA_FIELDS, METADATA_VERSION

METADATA_NAMES = frozenset((".intList", "intList", ".irq_info"))
POLICY_SYMBOL = "z_isr_metadata_nonallocated"
SHF_WRITE = 1
SHF_ALLOC = 2
SHF_EXECINSTR = 4
METADATA_HEADER_SIZE = struct.calcsize("<" + "I" * len(METADATA_FIELDS))
EF_ARM_BE8 = 0x00800000


def relocation_bytes(source, relocation, size):
    offset = relocation["r_offset"] - source["sh_addr"]
    if offset < 0 or offset + size > source["sh_size"]:
        raise ValueError("Runtime relocation exceeds its source section")
    return source.data()[offset : offset + size]


def arm_mov_immediate(elf, source, relocation, thumb):
    order = "little" if elf.little_endian or elf["e_flags"] & EF_ARM_BE8 else "big"
    data = relocation_bytes(source, relocation, 4)
    if thumb:
        first, second = int.from_bytes(data[:2], order), int.from_bytes(data[2:], order)
        value = ((first & 15) << 12) | (((first >> 10) & 1) << 11)
        value |= ((second >> 12) & 7) << 8 | (second & 255)
        return value, (second >> 8) & 15
    instruction = int.from_bytes(data, order)
    return (instruction & 0xFFF) | ((instruction >> 4) & 0xF000), (instruction >> 12) & 15


def signed(value, bits):
    value &= (1 << bits) - 1
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def arm_branch_target(elf, source, relocation, name):
    order = "little" if elf.little_endian or elf["e_flags"] & EF_ARM_BE8 else "big"
    place = relocation["r_offset"]
    if name.startswith("R_ARM_THM_"):
        first = int.from_bytes(relocation_bytes(source, relocation, 2), order)
        if first == 0xBF00 and name in {"R_ARM_THM_JUMP11", "R_ARM_THM_JUMP8"}:
            return (place + 2) & 0xFFFFFFFF
        if first == 0xF3AF and name in {"R_ARM_THM_CALL", "R_ARM_THM_JUMP24"}:
            second = int.from_bytes(relocation_bytes(source, relocation, 4)[2:], order)
            if second == 0x8000:
                return (place + 4) & 0xFFFFFFFF
    if name in {"R_ARM_PC24", "R_ARM_CALL", "R_ARM_JUMP24"}:
        instruction = int.from_bytes(relocation_bytes(source, relocation, 4), order)
        displacement = signed(instruction & 0xFFFFFF, 24) << 2
        if instruction >> 28 == 15:  # BLX immediate includes bit 1 in H.
            displacement |= (instruction >> 23) & 2
        return (place + 8 + displacement) & 0xFFFFFFFF
    size = 2 if name in {"R_ARM_THM_JUMP11", "R_ARM_THM_JUMP8"} else 4
    data = relocation_bytes(source, relocation, size)
    first = int.from_bytes(data[:2], order)
    if size == 2:
        bits = 11 if name == "R_ARM_THM_JUMP11" else 8
        return (place + 4 + (signed(first, bits) << 1)) & 0xFFFFFFFF
    second = int.from_bytes(data[2:], order)
    sign, j1, j2 = (first >> 10) & 1, (second >> 13) & 1, (second >> 11) & 1
    if name == "R_ARM_THM_JUMP19":
        immediate = (sign << 20) | (j2 << 19) | (j1 << 18)
        immediate |= (first & 0x3F) << 12 | (second & 0x7FF) << 1
        displacement = signed(immediate, 21)
    else:
        i1, i2 = 1 ^ (j1 ^ sign), 1 ^ (j2 ^ sign)
        immediate = (sign << 24) | (i1 << 23) | (i2 << 22)
        immediate |= (first & 0x3FF) << 12 | (second & 0x7FF) << 1
        displacement = signed(immediate, 25)
    pc = place + 4
    if name == "R_ARM_THM_CALL" and not second & 0x1000:
        pc &= ~3  # BLX uses Align(PC, 4).
    return (pc + displacement) & 0xFFFFFFFF


def relocation_target(elf, source, relocation, symbol):
    """Read an effective target, not a REL addend already rewritten by the linker."""
    name = describe_reloc_type(relocation["r_info_type"], elf)
    if relocation.is_RELA():
        if elf["e_machine"] != "EM_AARCH64" or name not in {
            "R_AARCH64_ABS64",
            "R_AARCH64_ABS32",
            "R_AARCH64_ABS16",
            "R_AARCH64_PREL64",
            "R_AARCH64_PREL32",
            "R_AARCH64_PREL16",
            "R_AARCH64_ADR_PREL_LO21",
            "R_AARCH64_ADR_PREL_PG_HI21",
            "R_AARCH64_ADR_PREL_PG_HI21_NC",
            "R_AARCH64_ADD_ABS_LO12_NC",
            "R_AARCH64_LDST8_ABS_LO12_NC",
            "R_AARCH64_LDST16_ABS_LO12_NC",
            "R_AARCH64_LDST32_ABS_LO12_NC",
            "R_AARCH64_LDST64_ABS_LO12_NC",
            "R_AARCH64_LDST128_ABS_LO12_NC",
            "R_AARCH64_LD_PREL_LO19",
            "R_AARCH64_TSTBR14",
            "R_AARCH64_CONDBR19",
            "R_AARCH64_JUMP26",
            "R_AARCH64_CALL26",
        }:
            raise ValueError(f"Cannot establish runtime relocation target for {name}")
        return (symbol["st_value"] + relocation["r_addend"]) & ((1 << elf.elfclass) - 1)
    order = "little" if elf.little_endian else "big"
    widths = {"R_ARM_ABS32": 4, "R_ARM_ABS32_NOI": 4, "R_ARM_ABS16": 2, "R_ARM_ABS8": 1}
    if name in widths:
        return int.from_bytes(relocation_bytes(source, relocation, widths[name]), order)
    if name in {"R_ARM_REL32", "R_ARM_REL32_NOI", "R_ARM_PREL31"}:
        value = int.from_bytes(relocation_bytes(source, relocation, 4), order)
        bits = 31 if name == "R_ARM_PREL31" else 32
        value &= (1 << bits) - 1
        if value & (1 << (bits - 1)):
            value -= 1 << bits
        return (value + relocation["r_offset"]) & 0xFFFFFFFF
    if name == "R_ARM_THM_ALU_PREL_11_0":
        instruction_order = "little" if elf.little_endian or elf["e_flags"] & EF_ARM_BE8 else "big"
        data = relocation_bytes(source, relocation, 4)
        first = int.from_bytes(data[:2], instruction_order)
        second = int.from_bytes(data[2:], instruction_order)
        if first & 0xFBFF not in {0xF20F, 0xF2AF} or second & 0x8000:
            raise ValueError("Invalid ADR.W instruction for runtime relocation")
        immediate = ((first >> 10) & 1) << 11 | ((second >> 12) & 7) << 8 | (second & 0xFF)
        pc = (relocation["r_offset"] + 4) & ~3
        return (pc + (-immediate if first & 0x80 else immediate)) & 0xFFFFFFFF
    if name in {"R_ARM_THM_PC12", "R_ARM_THM_PC8"}:
        instruction_order = "little" if elf.little_endian or elf["e_flags"] & EF_ARM_BE8 else "big"
        pc = (relocation["r_offset"] + 4) & ~3
        if name == "R_ARM_THM_PC8":
            instruction = int.from_bytes(relocation_bytes(source, relocation, 2), instruction_order)
            return (pc + ((instruction & 0xFF) << 2)) & 0xFFFFFFFF
        data = relocation_bytes(source, relocation, 4)
        first = int.from_bytes(data[:2], instruction_order)
        immediate = int.from_bytes(data[2:], instruction_order) & 0xFFF
        return (pc + (immediate if first & 0x80 else -immediate)) & 0xFFFFFFFF
    if name in {"R_ARM_MOVW_ABS_NC", "R_ARM_THM_MOVW_ABS_NC"}:
        value, _ = arm_mov_immediate(elf, source, relocation, "THM" in name)
        return value  # MOVW clears the upper half of the destination register.
    if name in {"R_ARM_MOVT_ABS", "R_ARM_THM_MOVT_ABS"}:
        value, _ = arm_mov_immediate(elf, source, relocation, "THM" in name)
        # MOVT does not establish the low half. Do not infer instruction
        # pairing: reject any possible target in this 64-KiB address page.
        return value << 16, (value + 1) << 16
    if name in {
        "R_ARM_PC24",
        "R_ARM_CALL",
        "R_ARM_JUMP24",
        "R_ARM_THM_CALL",
        "R_ARM_THM_JUMP24",
        "R_ARM_THM_JUMP19",
        "R_ARM_THM_JUMP11",
        "R_ARM_THM_JUMP8",
    }:
        target = arm_branch_target(elf, source, relocation, name)
        if target != symbol["st_value"] & ~1:
            # Linker-generated veneers can contain an unrelocated literal
            # address. The retained branch no longer exposes its REL addend.
            raise ValueError("Cannot establish runtime branch target through a veneer or addend")
        return target
    raise ValueError(f"Cannot establish runtime relocation target for {name}")


def relocation_target_ranges(elf, source, relocation, symbol):
    name = describe_reloc_type(relocation["r_info_type"], elf)
    if name == "R_ARM_TARGET1" and not relocation.is_RELA():
        # armlink chooses TARGET1 semantics using its C++/preinit routines.
        # ELF does not record that choice. Prove both interpretations safe
        # instead of changing the application's initialization ABI.
        encoded = int.from_bytes(
            relocation_bytes(source, relocation, 4), "little" if elf.little_endian else "big"
        )
        absolute_target = encoded
        relative_target = (encoded + relocation["r_offset"]) & 0xFFFFFFFF
        return [(absolute_target, absolute_target + 1), (relative_target, relative_target + 1)]
    target = relocation_target(elf, source, relocation, symbol)
    return [target] if isinstance(target, tuple) else [(target, target + 1)]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def overlap(start, size, other_start, other_size):
    return (
        size > 0
        and other_size > 0
        and start < other_start + other_size
        and other_start < start + size
    )


def runtime_image(elf, excluded):
    """Hash runtime addresses, permissions, extents, and every loaded byte."""
    result = []
    for index, segment in enumerate(elf.iter_segments()):
        if segment["p_type"] == "PT_LOAD" and index not in excluded:
            result.append(
                {
                    key: segment[key]
                    for key in (
                        "p_offset",
                        "p_vaddr",
                        "p_paddr",
                        "p_filesz",
                        "p_memsz",
                        "p_flags",
                        "p_align",
                    )
                }
                | {"sha256": sha256(segment.data())}
            )
    return result


def transform(data):
    elf = ELFFile(BytesIO(data))
    if elf["e_machine"] not in {"EM_ARM", "EM_AARCH64"}:
        raise ValueError("ISR metadata finalization supports armlink ARM ELF machines only")
    if elf["e_type"] != "ET_EXEC":
        raise ValueError("ISR metadata finalization requires an executable ELF")
    symtab = elf.get_section_by_name(".symtab")
    if symtab is None:
        raise ValueError("ISR metadata finalization requires a symbol table")
    symbols = symtab.get_symbol_by_name(POLICY_SYMBOL) or []
    if len(symbols) != 1 or symbols[0]["st_shndx"] != "SHN_ABS" or symbols[0]["st_value"] != 1:
        raise ValueError("Missing absolute nonallocated ISR metadata policy")
    sections = list(elf.iter_sections())
    metadata = {
        index: section
        for index, section in enumerate(sections)
        if section.name in METADATA_NAMES and section["sh_size"]
    }
    if not metadata:
        raise ValueError("No ISR metadata section to finalize")
    for section in metadata.values():
        if section["sh_type"] != "SHT_PROGBITS" or section["sh_flags"] & (
            SHF_WRITE | SHF_EXECINSTR
        ):
            raise ValueError("ISR metadata must contain readonly record bytes")
        for other_index, other in enumerate(sections):
            if (
                other_index not in metadata
                and other["sh_flags"] & SHF_ALLOC
                and overlap(
                    section["sh_addr"], section["sh_size"], other["sh_addr"], other["sh_size"]
                )
            ):
                raise ValueError("ISR metadata virtual range overlaps runtime storage")
    headers = symtab.get_symbol_by_name("_iheader") or []
    if len(headers) != 1 or headers[0]["st_shndx"] not in metadata:
        raise ValueError("ISR metadata header is not in its dedicated section")
    header = headers[0]
    header_section = metadata[header["st_shndx"]]
    header_offset = header["st_value"] - header_section["sh_addr"]
    prefix = "<" if elf.little_endian else ">"
    if header_offset < 0 or header_offset + METADATA_HEADER_SIZE > header_section["sh_size"]:
        raise ValueError("ISR metadata header exceeds its section")
    version, header_size = struct.unpack_from(prefix + "II", header_section.data(), header_offset)
    if version != METADATA_VERSION or header_size != METADATA_HEADER_SIZE:
        raise ValueError("Unexpected ISR metadata header layout")

    # Retained relocations let the finalizer reject real code/data which tries
    # to use a record address. Debug-only references do not load target bytes.
    relocations = 0
    for section in sections:
        if not isinstance(section, RelocationSection):
            continue
        source_index = section["sh_info"]
        if source_index in metadata and section.num_relocations():
            raise ValueError("ISR metadata contains an address relocation")
        source = sections[source_index]
        table = sections[section["sh_link"]]
        for relocation in section.iter_relocations():
            relocations += 1
            symbol = table.get_symbol(relocation["r_info_sym"])
            if source["sh_flags"] & SHF_ALLOC and symbol["st_shndx"] in metadata:
                raise ValueError("Runtime relocation refers to ISR metadata")
            if source["sh_flags"] & SHF_ALLOC and relocation["r_info_type"] != 0:
                if any(
                    item["sh_addr"] <= symbol["st_value"] < item["sh_addr"] + item["sh_size"]
                    for item in metadata.values()
                ):
                    raise ValueError("Runtime relocation symbol refers to ISR metadata")
                name = describe_reloc_type(relocation["r_info_type"], elf)
                if name in {"R_ARM_V4BX"}:
                    continue  # Instruction-set fixup has no symbol address operand.
                ranges = relocation_target_ranges(elf, source, relocation, symbol)
                if any(
                    overlap(start, end - start, item["sh_addr"], item["sh_size"])
                    for start, end in ranges
                    for item in metadata.values()
                ):
                    if name == "R_ARM_TARGET1":
                        raise ValueError(
                            "Runtime relocation TARGET1 has an ambiguous ABS32/REL32 "
                            "target which can refer to ISR metadata; ELF unchanged"
                        )
                    raise ValueError("Runtime relocation target refers to ISR metadata")

    detached = set()
    covered = set()
    for index, segment in enumerate(elf.iter_segments()):
        if segment["p_type"] != "PT_LOAD":
            continue
        matches = {
            key
            for key, section in metadata.items()
            if overlap(
                segment["p_offset"], segment["p_filesz"], section["sh_offset"], section["sh_size"]
            )
        }
        if not matches:
            continue
        if segment["p_memsz"] != segment["p_filesz"]:
            raise ValueError("Metadata load segment contains anonymous memory storage")
        for key in matches:
            section = metadata[key]
            if (
                section["sh_offset"] < segment["p_offset"]
                or section["sh_offset"] + section["sh_size"]
                > segment["p_offset"] + segment["p_filesz"]
            ):
                raise ValueError("A load segment contains only part of ISR metadata")
            if not segment.section_in_segment(section):
                raise ValueError("Metadata section does not match its load mapping")
        for key, section in enumerate(sections):
            if key in metadata or section["sh_size"] == 0:
                continue
            if section["sh_type"] != "SHT_NOBITS" and overlap(
                segment["p_offset"], segment["p_filesz"], section["sh_offset"], section["sh_size"]
            ):
                raise ValueError(f"Metadata load segment also contains {section.name}")
            if (
                section["sh_type"] == "SHT_NOBITS"
                and section["sh_flags"] & SHF_ALLOC
                and segment.section_in_segment(section)
            ):
                raise ValueError("Metadata load segment also contains runtime BSS")
        cursor = segment["p_offset"]
        for section in sorted((metadata[key] for key in matches), key=lambda s: s["sh_offset"]):
            if any(data[cursor : section["sh_offset"]]):
                raise ValueError("Metadata load segment contains unclassified bytes")
            cursor = section["sh_offset"] + section["sh_size"]
        if any(data[cursor : segment["p_offset"] + segment["p_filesz"]]):
            raise ValueError("Metadata load segment contains unclassified bytes")
        covered.update(matches)
        detached.add(index)
    if any(
        section["sh_flags"] & SHF_ALLOC and index not in covered
        for index, section in metadata.items()
    ):
        raise ValueError("Allocated ISR metadata has no dedicated load segment")
    if detached and relocations == 0:
        raise ValueError("Retain executable relocations when finalizing ISR metadata")

    before_runtime = runtime_image(elf, detached)
    output = bytearray(data)
    changes = []
    flag_format = prefix + ("I" if elf.elfclass == 32 else "Q")
    for index, section in metadata.items():
        position = elf["e_shoff"] + index * elf["e_shentsize"] + 8
        struct.pack_into(flag_format, output, position, section["sh_flags"] & ~SHF_ALLOC)
        changes.append((position, struct.calcsize(flag_format)))
    for index in detached:
        position = elf["e_phoff"] + index * elf["e_phentsize"]
        struct.pack_into(prefix + "I", output, position, 0)  # PT_NULL
        changes.append((position, 4))
    changed = {index for start, size in changes for index in range(start, start + size)}
    if any(
        a != b and index not in changed
        for index, (a, b) in enumerate(zip(data, output, strict=True))
    ):
        raise ValueError("Finalization changed bytes outside metadata headers")
    finalized = ELFFile(BytesIO(output))
    after_runtime = runtime_image(finalized, set())
    if before_runtime != after_runtime:
        raise ValueError("Finalization would change the runtime load image")
    for section in finalized.iter_sections():
        if section.name in METADATA_NAMES:
            if section["sh_flags"] & SHF_ALLOC:
                raise ValueError("Finalized ISR metadata is still allocated")
            if any(
                segment["p_type"] == "PT_LOAD" and segment.section_in_segment(section)
                for segment in finalized.iter_segments()
            ):
                raise ValueError("Finalized ISR metadata is still loadable")
    runtime_hash = sha256(json.dumps(before_runtime, sort_keys=True).encode())
    return bytes(output), {
        "metadata_policy": "nonallocated",
        "elf_before_sha256": sha256(data),
        "elf_after_sha256": sha256(output),
        "detached_load_segments": sorted(detached),
        "runtime_load_image_before_sha256": runtime_hash,
        "runtime_load_image_after_sha256": sha256(
            json.dumps(after_runtime, sort_keys=True).encode()
        ),
        "runtime_load_segments": before_runtime,
        "relocations_checked": relocations,
        "metadata": [
            {"name": section.name, "size": section["sh_size"], "sha256": sha256(section.data())}
            for section in metadata.values()
        ],
    }


def finalize(path, report_path):
    path = Path(path)
    original = path.read_bytes()
    output, report = transform(original)
    # Validate the complete result before replacing the linker output.
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=path.parent, prefix=f".{path.name}.", delete=False
        ) as temp:
            temporary = Path(temp.name)
            temp.write(output)
            temp.flush()
        os.chmod(temporary, path.stat().st_mode & 0o777)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    Path(report_path).write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--report", required=True)
    args = parser.parse_args()
    try:
        finalize(args.elf, args.report)
    except (ValueError, OSError, ELFError) as error:
        parser.exit(1, f"ISR metadata finalization failed: {error}\n")


if __name__ == "__main__":
    main()
