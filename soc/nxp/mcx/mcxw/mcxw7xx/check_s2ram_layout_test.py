# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

import pickle
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ZEPHYR_BASE = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(ZEPHYR_BASE / "scripts/dts/python-devicetree/src"))
from devicetree import edtlib  # noqa: E402

CHECKER = Path(__file__).with_name("check_s2ram_layout.py")
SHUTDOWN_CONFIG = "CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM"


def write_elf(path, address, size, *, allocated=True, nobits=True, name=".probe"):
    names = b"\0" + name.encode() + b"\0.shstrtab\0"
    offset = 52 + 3 * 40
    header = struct.pack(
        "<16sHHIIIIIHHHHHH",
        b"\x7fELF\x01\x01\x01" + bytes(9),
        2,
        40,
        1,
        0,
        0,
        52,
        0,
        52,
        0,
        0,
        40,
        3,
        2,
    )
    section = struct.pack(
        "<10I",
        1,
        8 if nobits else 1,
        2 if allocated else 0,
        address,
        offset + len(names),
        size,
        0,
        0,
        4,
        0,
    )
    strings = struct.pack("<10I", len(name) + 2, 3, 0, 0, offset, len(names), 0, 0, 1, 0)
    path.write_bytes(
        header + bytes(40) + section + strings + names + (b"" if nobits else bytes(size))
    )


class LayoutCheckerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mcxw7x-layout-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.image = self.directory / "probe.elf"
        self.write_policy()

    def write_policy(
        self, *, w72=False, shutdown=True, keep=None, rom_base=0x30000000, rom_size=None
    ):
        if rom_size is None:
            rom_size = 0x8000 if w72 else 0x4000
        self.regions = [
            "--region",
            hex(rom_base),
            hex(rom_size),
            "--region",
            hex(rom_base ^ 0x10000000),
            hex(rom_size),
        ]
        ctcm_size = 0x8000 if w72 else 0x4000
        stcm_size = 0x3A000 if w72 else 0x1C000
        banks = [
            (0x14000000, ctcm_size // 2, 0),
            (0x14000000 + ctcm_size // 2, ctcm_size // 2, 1),
            (0x30000000, 0x4000, 2),
            (0x30004000, 0x4000, 3),
            (0x30008000, 0x8000, 4),
            (0x30010000, 0x8000, 5),
        ]
        banks.extend(
            (base, 0x8000 if w72 else 0x2000, bit)
            for bit, base in enumerate(
                range(0x30018000, 0x30038000 if w72 else 0x3001A000, 0x8000), 6
            )
        )
        bank_cells = ", ".join(f"<{base:#x} {size:#x} {bit}>" for base, size, bit in banks)
        named = ""
        if keep is not None:
            base, size = keep
            named = f"""
                memory@{base:x} {{
                    compatible = "zephyr,memory-region", "mmio-sram";
                    reg = <{base:#x} {size:#x}>;
                    zephyr,memory-region = "PROBE";
                }};
            """
        source = self.directory / "test.dts"
        source.write_text(
            f"""
            /dts-v1/;
            / {{
                #address-cells = <1>;
                #size-cells = <1>;
                chosen {{
                    zephyr,sram = &ram;
                    zephyr,flash = &flash;
                }};
                flash: flash@10000000 {{
                    reg = <0x10000000 0x100000>;
                }};
                ctcm: ctcm {{
                    #address-cells = <1>;
                    #size-cells = <1>;
                    ranges = <0 0x14000000 {ctcm_size:#x}>;
                }};
                stcm: stcm {{
                    #address-cells = <1>;
                    #size-cells = <1>;
                    ranges = <0 0x30000000 {stcm_size:#x}>;
                    ram: memory@8000 {{
                        compatible = "mmio-sram";
                        reg = <0x8000 0x8000>;
                    }};
                }};
                cmc: power-controller@50001000 {{
                    compatible = "nxp,cmc";
                    reg = <0x50001000 0x1000>;
                    sram-banks = {bank_cells};
                }};
                reserved-memory {{
                    #address-cells = <1>;
                    #size-cells = <1>;
                    ranges;
                    s2ram_reserved: memory@{rom_base:x} {{
                        reg = <{rom_base:#x} {rom_size:#x}>,
                              <{rom_base ^ 0x10000000:#x} {rom_size:#x}>;
                        reg-names = "secure", "nonsecure";
                    }};
                }};
                {named}
            }};
            """,
            encoding="utf-8",
        )
        edt = edtlib.EDT(str(source), [str(ZEPHYR_BASE / "dts/bindings")])
        (self.directory / "edt.pickle").write_bytes(pickle.dumps(edt))
        (self.directory / ".config").write_text(
            f"{SHUTDOWN_CONFIG}=y\n" if shutdown else f"# {SHUTDOWN_CONFIG} is not set\n",
            encoding="utf-8",
        )

    def run_checker(self, address, size=4, *, regions=None, **section_options):
        write_elf(self.image, address, size, **section_options)
        return subprocess.run(
            [
                sys.executable,
                str(CHECKER),
                str(self.image),
                *(self.regions if regions is None else regions),
            ],
            capture_output=True,
            text=True,
            timeout=30,
        )

    def assert_layout(self, address, size=4, *, error=None, **section_options):
        result = self.run_checker(address, size, **section_options)
        self.assertEqual(result.returncode, 1 if error else 0, result.stderr)
        if error:
            self.assertIn(error, result.stderr)
            self.assertIn(section_options.get("name", ".probe"), result.stderr)
            self.assertNotIn("PASS", result.stdout)
        else:
            self.assertIn("ROM-safe layout PASS", result.stdout)
            self.assertEqual(result.stderr, "")

    def test_rejects_powered_off_banks_in_both_aliases(self):
        for address in (0x30010000, 0x20010000, 0x14000000, 0x04000000):
            for nobits in (False, True):
                with self.subTest(address=hex(address), nobits=nobits):
                    self.assert_layout(address, nobits=nobits, error="powered-off CMC bank")

    def test_named_region_keeps_only_overlapping_banks(self):
        self.write_policy(keep=(0x30010000, 0x8000))
        for address in (0x30010000, 0x30017FFC, 0x20010000):
            with self.subTest(address=hex(address)):
                self.assert_layout(address)
        self.assert_layout(0x30018000, error="powered-off CMC bank")
        self.write_policy(keep=(0x20010000, 0x8000))
        self.assert_layout(0x20010000, error="powered-off CMC bank")

    def test_keep_region_end_must_not_wrap_uintptr(self):
        self.write_policy(keep=(0x10000000, 0xF0000000))
        for address in (0x14000000, 0x04000000, 0x30010000, 0x20010000):
            with self.subTest(address=hex(address)):
                result = self.run_checker(address)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertIn("32-bit", result.stderr)
                self.assertNotIn("PASS", result.stdout)
        self.write_policy(keep=(0xFFFFFFFB, 4))
        self.assert_layout(0x14000000, error="powered-off CMC bank")

    def test_shutdown_disabled_accepts_tcm_but_not_unknown_memory(self):
        self.write_policy(shutdown=False)
        for address in (0x30010000, 0x20010000, 0x14000000, 0x04000000, 0x10000000):
            with self.subTest(address=hex(address)):
                self.assert_layout(address)
        self.assert_layout(0x40000000, error="outside supported TCM/flash")

    def test_rom_boundaries_and_allocated_flags(self):
        for w72 in (False, True):
            self.write_policy(w72=w72, shutdown=False)
            end = 0x8000 if w72 else 0x4000
            for base in (0x30000000, 0x20000000):
                with self.subTest(w72=w72, base=hex(base)):
                    self.assert_layout(base, error="low-STCM exclusion")
                    self.assert_layout(base + end - 4, 8, error="low-STCM exclusion")
                    self.assert_layout(base + end)
                    self.assert_layout(base, 0)
                    self.assert_layout(base, allocated=False, nobits=False)

    def test_bank_and_physical_extent_crossings(self):
        self.assert_layout(0x3000FFFC, 4)
        self.assert_layout(0x3000FFFC, 8, error="powered-off CMC bank")
        for w72 in (False, True):
            self.write_policy(w72=w72)
            upper = 0x30038000 if w72 else 0x3001A000
            for alias in (0, 0x10000000):
                with self.subTest(w72=w72, alias=hex(alias)):
                    self.assert_layout(upper ^ alias, 0x2000)
                    self.assert_layout(
                        (upper ^ alias) + 0x1FFC, 8, error="outside supported TCM/flash"
                    )

    def test_rejects_named_radio_ram_without_section_name_exception(self):
        self.write_policy(keep=(0x589C0000, 0xA000))
        self.assert_layout(
            0x589C0000,
            0xA000,
            name=".noinit_shared_mem",
            error="outside supported TCM/flash",
        )

    def test_exclusion_bounds_are_supplied_not_hardcoded(self):
        self.write_policy(shutdown=False, rom_base=0x30018000, rom_size=0x1000)
        self.assert_layout(0x30000000)
        self.assert_layout(0x30018000, error="low-STCM exclusion")
        self.assert_layout(0x20018FFC, 8, error="low-STCM exclusion")
        self.assert_layout(0x30019000)

    def test_missing_policy_and_invalid_ranges_fail_explicitly(self):
        (self.directory / "edt.pickle").unlink()
        result = self.run_checker(0x10000000)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("edt.pickle", result.stderr)
        self.assertNotIn("PASS", result.stdout)
        self.write_policy()
        (self.directory / ".config").write_text("", encoding="utf-8")
        result = self.run_checker(0x10000000)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn(SHUTDOWN_CONFIG, result.stderr)
        for regions in (
            [],
            ["--region", "-1", "4"],
            ["--region", "0x30000000", "0"],
            ["--region", "0xfffffffc", "8"],
        ):
            with self.subTest(regions=regions):
                result = self.run_checker(0x10000000, regions=regions)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("error:", result.stderr)
                self.assertNotIn("PASS", result.stdout)


if __name__ == "__main__":
    unittest.main()
