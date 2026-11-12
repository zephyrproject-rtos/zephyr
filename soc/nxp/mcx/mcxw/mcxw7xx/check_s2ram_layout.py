# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

import argparse
import pickle
import sys
from pathlib import Path

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile

sys.path.insert(0, str(Path(__file__).resolve().parents[5] / "scripts/dts/python-devicetree/src"))
from devicetree import edtlib  # noqa: E402


def overlaps(start, size, base, length):
    return start < base + length and base < start + size


def checked_range(base, size):
    if base is None or size is None or base < 0 or size <= 0 or base + size >= 1 << 32:
        raise ValueError("each memory region must be nonempty with a non-wrapping 32-bit end")
    return base, size


def read_policy(edt_path, config_path):
    with edt_path.open("rb") as stream:
        edt: edtlib.EDT = pickle.load(stream)
    settings = config_path.read_text(encoding="utf-8").splitlines()
    option = "CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM"
    enabled = f"{option}=y" in settings
    disabled = f"# {option} is not set" in settings
    if enabled == disabled:
        raise ValueError(f"{config_path}: expected one enabled/disabled setting for {option}")

    ram = []
    for label in ("ctcm", "stcm"):
        node = edt.label2node.get(label)
        if node is None or not node.ranges:
            raise ValueError(f"{edt_path}: missing {label} memory ranges")
        for region in node.ranges:
            base, size = checked_range(region.parent_bus_addr, region.length)
            ram.extend(((base, size), (base ^ 0x10000000, size)))

    flash_node = edt.chosen_node("zephyr,flash")
    sram_node = edt.chosen_node("zephyr,sram")
    reserved = edt.label2node.get("s2ram_reserved")
    cmc = edt.label2node.get("cmc")
    if flash_node is None or sram_node is None or reserved is None or cmc is None:
        raise ValueError(f"{edt_path}: missing chosen flash/SRAM, ROM reservation or CMC")

    flash = [checked_range(reg.addr, reg.size) for reg in flash_node.regs]
    keep_nodes = [sram_node, flash_node, *edt.compat2okay.get("zephyr,memory-region", [])]
    keep = []
    for node in keep_nodes:
        if not node.regs:
            raise ValueError(f"{edt_path}: {node.path} has no memory region")
        keep.append(checked_range(node.regs[0].addr, node.regs[0].size))
    exclusions = [checked_range(reg.addr, reg.size) for reg in reserved.regs]
    secure_rom = next((reg for reg in reserved.regs if reg.name == "secure"), None)
    if secure_rom is None or not exclusions:
        raise ValueError(f"{edt_path}: missing secure ROM reservation")
    keep.append(checked_range(secure_rom.addr, secure_rom.size))

    banks_property = cmc.props.get("sram-banks")
    if banks_property is None or not banks_property.val or len(banks_property.val) % 3:
        raise ValueError(f"{edt_path}: CMC sram-banks requires <address size disable-bit> triples")
    cells = banks_property.val
    off_banks = []
    for index in range(0, len(cells), 3):
        base, size = checked_range(cells[index], cells[index + 1])
        bit = cells[index + 2]
        if not 0 <= bit < 32:
            raise ValueError(f"{edt_path}: invalid CMC SRAM disable bit {bit}")
        if enabled and not any(overlaps(base, size, start, length) for start, length in keep):
            off_banks.append((base, size, bit))
    return ram, flash, off_banks, exclusions


def main():
    parser = argparse.ArgumentParser(
        description="Check MCXW7x S2RAM ROM exclusion and retained RAM", allow_abbrev=False
    )
    parser.add_argument("elf", type=Path)
    parser.add_argument("--edt-pickle", type=Path, help="generated EDT; defaults beside the ELF")
    parser.add_argument("--config", type=Path, help="generated .config; defaults beside the ELF")
    parser.add_argument(
        "--region",
        nargs=2,
        action="append",
        type=lambda value: int(value, 0),
        required=True,
        metavar=("ADDRESS", "SIZE"),
        help="excluded DT range; repeat for each address alias",
    )
    args = parser.parse_args()
    try:
        for base, size in args.region:
            checked_range(base, size)
        ram, flash, off_banks, exclusions = read_policy(
            args.edt_pickle or args.elf.parent / "edt.pickle",
            args.config or args.elf.parent / ".config",
        )
        if set(map(tuple, args.region)) != set(exclusions):
            raise ValueError("excluded ranges must match the generated DT ROM reservation")
    except (OSError, ValueError, pickle.UnpicklingError) as error:
        parser.error(str(error))
    conflicts = []

    with args.elf.open("rb") as stream:
        for section in ELFFile(stream).iter_sections():
            start = section["sh_addr"]
            size = section["sh_size"]
            if not (section["sh_flags"] & SH_FLAGS.SHF_ALLOC) or size == 0:
                continue
            reason = None
            if any(overlaps(start, size, base, length) for base, length in exclusions):
                reason = "overlaps the low-STCM exclusion"
            elif any(base <= start and start + size <= base + length for base, length in ram):
                for base, length, bit in off_banks:
                    if any(
                        overlaps(start, size, base ^ alias, length) for alias in (0, 0x10000000)
                    ):
                        reason = f"overlaps powered-off CMC bank {bit}"
                        break
            elif not any(base <= start and start + size <= base + length for base, length in flash):
                reason = "outside supported TCM/flash"
            if reason:
                conflicts.append(
                    f"  {section.name}: [{start:#010x}, {start + size:#010x}) {reason}"
                )

    if conflicts:
        parser.exit(
            1,
            f"{args.elf}: unsafe allocated sections in the MCXW7x S2RAM layout:\n"
            + "\n".join(conflicts)
            + "\n",
        )
    print("MCXW7x S2RAM ROM-safe layout PASS")
    disable_mask = 0
    for _, _, bit in off_banks:
        disable_mask |= 1 << bit
    print(f"MCXW7x S2RAM retained layout PASS (SRAMDIS policy {disable_mask:#x})")


if __name__ == "__main__":
    main()
