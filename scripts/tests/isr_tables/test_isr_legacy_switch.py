# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Generate shared switch dispatch through the legacy registration parser."""

import importlib
import struct
from io import StringIO
from pathlib import Path
from types import SimpleNamespace

import pytest


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
def test_shared_switch_keeps_symbolic_dispatcher(monkeypatch, bits, little):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "build"))
    module = importlib.import_module("gen_isr_tables_parser_carrays")

    class Configuration:
        args = SimpleNamespace(sw_isr_table=True, vector_table=False)
        swt_shared_handler = "z_shared_isr"
        swt_spurious_handler = "z_irq_spurious"

        @staticmethod
        def endian_prefix():
            return "<" if little else ">"

        @staticmethod
        def check_64b():
            return bits == 64

        @staticmethod
        def check_sym(name):
            return name == "CONFIG_GEN_SW_ISR_TABLE_SWITCH"

        @staticmethod
        def check_shared_interrupts():
            return True

        @staticmethod
        def test_isr_direct(flags):
            return flags & 1

        @staticmethod
        def get_swt_table_index(offset, irq):
            return irq - offset

        @staticmethod
        def get_sym(name):
            return {
                "CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS": 2,
                "CONFIG_GEN_SW_ISR_TABLE_ARRAY": 0,
                "CONFIG_GEN_SW_ISR_TABLE_SWITCH": 1,
            }[name]

    config = Configuration()
    prefix = config.endian_prefix()
    data = struct.pack(prefix + "II", 3, 16)
    for irq, handler, argument in [
        (16, 0x1000, 0x2000),
        (16, 0x1100, 0x2100),
        (17, 0x1200, 0x2200),
    ]:
        data += struct.pack(prefix + ("iiQQ" if bits == 64 else "iiII"), irq, 0, handler, argument)

    def error(message):
        raise ValueError(message)

    parser = module.gen_isr_parser(data, config, SimpleNamespace(debug=lambda _: None, error=error))
    output = StringIO()
    parser.write_source(output)
    source = output.getvalue()
    assert "entry->isr = (ISR)z_shared_isr;" in source
    assert "entry->arg = (const void *)&z_shared_sw_isr_table[0];" in source
    assert "entry->isr = (ISR)0x1200;" in source
    assert "entry->arg = (const void *)0x2200;" in source
    assert "entry->isr = (ISR)z_irq_spurious;" in source
    assert ".client_num = 2" in source
