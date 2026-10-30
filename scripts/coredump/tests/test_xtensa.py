# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import importlib
import struct
from pathlib import Path
from types import SimpleNamespace

import pytest


@pytest.fixture
def xtensa(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[1]))
    return importlib.import_module("gdbstubs.arch.xtensa")


def make_stub(xtensa, soc, toolchain, version=2, extra=b"", extra_valid=0):
    definition = xtensa.get_gdb_reg_definition(soc, toolchain)
    values = list(range(struct.calcsize(definition.ARCH_DATA_BLK_STRUCT_REGS) // 4))
    values[1] = 63  # Z_EXCEPT_REASON
    header = struct.pack("<BH", soc.value, version)
    if version != 1:
        header += bytes([toolchain.value])
    if version == 3:
        header += struct.pack("<I", extra_valid)
    data = header + struct.pack(definition.ARCH_DATA_BLK_STRUCT_REGS, *values) + extra
    logfile = SimpleNamespace(get_arch_data=lambda: {"data": data}, get_memory_regions=list)
    elffile = SimpleNamespace(get_memory_regions=list)
    return xtensa.GdbStub_Xtensa(logfile, elffile)


@pytest.mark.parametrize("soc,toolchain", [(1, 1), (2, 1), (3, 1), (3, 2), (4, 1), (5, 1), (6, 1)])
@pytest.mark.parametrize(
    "version,extra_valid", [(1, 0), (2, 0), (3, 0), (3, 1), (3, 1 << 17), (3, (1 << 18) - 1)]
)
def test_registers(xtensa, soc, toolchain, version, extra_valid):
    soc = xtensa.XtensaSoc(soc)
    toolchain = xtensa.XtensaToolchain(toolchain)
    extra = bytes(4 * extra_valid.bit_count())
    stub = make_stub(xtensa, soc, toolchain, version, extra, extra_valid)
    registers = stub.gdb_reg_def.RegNum
    assert stub.registers[registers.A0.value] == (6 if hasattr(registers, "SCOMPARE1") else 5)
    assert stub.registers[registers.WINDOWBASE.value] == 0
    assert stub.registers[registers.WINDOWSTART.value] == 1
    assert stub.gdb_signal == 6
    assert stub.extra_registers == {
        name: 0 for index, name in enumerate(xtensa.EXTRA_REGISTERS) if extra_valid & (1 << index)
    }
    assert stub.extra_valid == extra_valid
    for register in stub.gdb_reg_def.ExtraRegNum:
        if register.name in stub.extra_registers:
            assert stub.registers[register.value] == stub.extra_registers[register.name]
        else:
            assert register.value not in stub.registers


def test_extra_registers(xtensa, caplog):
    # Zero is a valid captured value; absent slots must not be presented as zero.
    extra = struct.pack("<3I", 7, 0, 0x12345678)
    with caplog.at_level("INFO", logger="gdbstub"):
        stub = make_stub(
            xtensa,
            xtensa.XtensaSoc.DC233C,
            xtensa.XtensaToolchain.ZEPHYR,
            3,
            extra,
            13,
        )
    assert stub.extra_registers == {"WINDOWBASE": 7, "THREADPTR": 0, "EPC1": 0x12345678}
    assert "EPC1: 0x12345678" in caplog.text
    # Actual window state cannot replace GDB's synthetic window for the saved A registers.
    assert stub.registers[stub.gdb_reg_def.RegNum.WINDOWBASE.value] == 0


@pytest.mark.parametrize("extra", [b"", bytes(8), bytes(16)])
def test_invalid_extra_length(xtensa, extra):
    with pytest.raises(struct.error):
        make_stub(
            xtensa,
            xtensa.XtensaSoc.DC233C,
            xtensa.XtensaToolchain.ZEPHYR,
            3,
            extra,
            13,
        )


@pytest.mark.parametrize("bit", [18, 31])
def test_unknown_extra_slot(xtensa, bit):
    with pytest.raises(ValueError, match="Unknown Xtensa extra register slots"):
        make_stub(
            xtensa,
            xtensa.XtensaSoc.DC233C,
            xtensa.XtensaToolchain.ZEPHYR,
            3,
            extra_valid=1 << bit,
        )


def test_values_without_valid_bits(xtensa):
    with pytest.raises(struct.error):
        make_stub(xtensa, xtensa.XtensaSoc.DC233C, xtensa.XtensaToolchain.ZEPHYR, 3, bytes(4))


def test_truncated_validity_bitmap(xtensa):
    data = struct.pack("<BHB", 6, 3, 1) + bytes(3)
    logfile = SimpleNamespace(get_arch_data=lambda: {"data": data}, get_memory_regions=list)
    elffile = SimpleNamespace(get_memory_regions=list)
    with pytest.raises(struct.error):
        xtensa.GdbStub_Xtensa(logfile, elffile)


@pytest.mark.parametrize(
    "soc,toolchain,threadptr,epc1,excsave1,epc1_offset",
    [
        (1, 1, None, 51, 65, 204),
        (2, 1, 74, 116, 130, 464),
        (3, 1, 75, 121, 135, 564),
        (3, 2, 0x3E7, 0x2B1, 0x2D1, None),
        (4, 1, 71, 82, 96, 328),
        (5, 1, 74, 139, 153, 532),
        (6, 1, 43, 66, 80, 264),
    ],
)
def test_gdb_extra_packets(xtensa, soc, toolchain, threadptr, epc1, excsave1, epc1_offset):
    # GDB numbers and packet offsets come from the public toolchain xtensa-config.c files.
    stub = make_stub(
        xtensa,
        xtensa.XtensaSoc(soc),
        xtensa.XtensaToolchain(toolchain),
        3,
        struct.pack("<5I", 7, 0x80, 0, 0x12345678, 0x87654321),
        (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 17),
    )
    packets = []
    stub.put_gdb_packet = packets.append
    stub.handle_register_single_read_packet(f"p{epc1:x}".encode())
    assert packets.pop() == b"78563412"
    stub.handle_register_single_read_packet(f"p{excsave1:x}".encode())
    assert packets.pop() == b"21436587"
    if threadptr is not None:
        stub.handle_register_single_read_packet(f"p{threadptr:x}".encode())
        assert packets.pop() == b"00000000"
    # EPC2 exists in each mapping, but this capture did not include it.
    stub.handle_register_single_read_packet(f"p{epc1 + 1:x}".encode())
    assert packets.pop() == b"xxxxxxxx"
    for name, value in (("WINDOWBASE", 0), ("WINDOWSTART", 1)):
        number = getattr(stub.gdb_reg_def.RegNum, name).value
        stub.handle_register_single_read_packet(f"p{number:x}".encode())
        assert packets.pop() == struct.pack("<I", value).hex().encode()
    stub.handle_register_group_read_packet()
    packet = packets.pop()
    assert len(packet) == min(
        (max(r.value for r in stub.gdb_reg_def.RegNum) + 1) * 8,
        stub.gdb_reg_def.SOC_GDB_GPKT_BIN_SIZE * 2,
    )
    if epc1_offset is not None and epc1_offset * 2 < len(packet):
        assert packet[epc1_offset * 2 : (epc1_offset + 4) * 2] == b"78563412"
        assert packet[(epc1_offset + 4) * 2 : (epc1_offset + 8) * 2] == b"xxxxxxxx"
    if threadptr is not None and threadptr * 8 < len(packet):
        assert packet[threadptr * 8 : (threadptr + 1) * 8] == b"00000000"


@pytest.mark.parametrize("version", [1, 2, 3])
def test_missing_extra_registers_in_gdb(xtensa, version):
    stub = make_stub(xtensa, xtensa.XtensaSoc.DC233C, xtensa.XtensaToolchain.ZEPHYR, version)
    packets = []
    stub.put_gdb_packet = packets.append
    stub.handle_register_single_read_packet(b"p42")  # EPC1
    assert packets.pop() == b"xxxxxxxx"
    stub.handle_register_group_read_packet()
    assert packets.pop()[66 * 8 : 67 * 8] == b"xxxxxxxx"
