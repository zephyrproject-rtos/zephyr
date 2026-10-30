#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0
"""
Tests for check_init_priorities
"""

import pathlib
import unittest
from unittest import mock

import check_init_priorities
from elftools.elf.relocation import RelocationSection, Section
from elftools.elf.sections import SymbolTableSection


class TestPriority(unittest.TestCase):
    """Tests for the Priority class."""

    def test_priority_parsing(self):
        prio1 = check_init_priorities.Priority("POST_KERNEL", 12)
        self.assertEqual(prio1._level_priority, (3, 12))

        prio1 = check_init_priorities.Priority("APPLICATION", 9999)
        self.assertEqual(prio1._level_priority, (4, 9999))

        with self.assertRaises(ValueError):
            check_init_priorities.Priority("i-am-not-a-priority", 0)
            check_init_priorities.Priority("_DOESNOTEXIST0_", 0)

    def test_priority_levels(self):
        prios = [
            check_init_priorities.Priority("EARLY", 0),
            check_init_priorities.Priority("EARLY", 1),
            check_init_priorities.Priority("PRE_KERNEL_1", 0),
            check_init_priorities.Priority("PRE_KERNEL_1", 1),
            check_init_priorities.Priority("PRE_KERNEL_2", 0),
            check_init_priorities.Priority("PRE_KERNEL_2", 1),
            check_init_priorities.Priority("POST_KERNEL", 0),
            check_init_priorities.Priority("POST_KERNEL", 1),
            check_init_priorities.Priority("APPLICATION", 0),
            check_init_priorities.Priority("APPLICATION", 1),
            check_init_priorities.Priority("SMP", 0),
            check_init_priorities.Priority("SMP", 1),
        ]

        self.assertListEqual(prios, sorted(prios))

    def test_priority_strings(self):
        prio = check_init_priorities.Priority("POST_KERNEL", 12)
        self.assertEqual(str(prio), "POST_KERNEL+12")
        self.assertEqual(repr(prio), "<Priority POST_KERNEL 12>")


class testZephyrInitLevels(unittest.TestCase):
    """Tests for the ZephyrInitLevels class."""

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_load_objects(self, mock_zilinit):
        mock_elf = mock.Mock()

        sts = mock.Mock(spec=SymbolTableSection)
        rel = mock.Mock(spec=Section)
        mock_elf.iter_sections.return_value = [sts, rel]

        s0 = mock.Mock()
        s0.name = "a"
        s0.entry.st_info.type = "STT_OBJECT"
        s0.entry.st_size = 4
        s0.entry.st_value = 0xAA
        s0.entry.st_shndx = 1

        s1 = mock.Mock()
        s1.name = None

        s2 = mock.Mock()
        s2.name = "b"
        s2.entry.st_info.type = "STT_FUNC"
        s2.entry.st_size = 8
        s2.entry.st_value = 0xBB
        s2.entry.st_shndx = 2

        sts.iter_symbols.return_value = [s0, s1, s2]

        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock_elf
        obj._relocatable = False
        obj._load_objects()

        self.assertDictEqual(obj._objects, {0xAA: ("a", 4, 1), 0xBB: ("b", 8, 2)})
        self.assertDictEqual(obj._object_addr, {"a": 0xAA, "b": 0xBB})

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_load_objects_relocatable(self, mock_zilinit):
        mock_elf = mock.Mock()

        sts = mock.Mock(spec=SymbolTableSection)
        mock_elf.iter_sections.return_value = [sts]

        s0 = mock.Mock()
        s0.name = "a"
        s0.entry.st_info.type = "STT_FUNC"
        s0.entry.st_size = 163
        s0.entry.st_value = 0x00
        s0.entry.st_shndx = 1

        s1 = mock.Mock()
        s1.name = "b"
        s1.entry.st_info.type = "STT_FUNC"
        s1.entry.st_size = 1
        s1.entry.st_value = 0x00
        s1.entry.st_shndx = 2

        s2 = mock.Mock()
        s2.name = "c"
        s2.entry.st_info.type = "STT_OBJECT"
        s2.entry.st_size = 8
        s2.entry.st_value = 0x08
        s2.entry.st_shndx = 2

        sts.iter_symbols.return_value = [s0, s1, s2]

        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock_elf
        obj._relocatable = True
        obj._load_objects()

        self.assertDictEqual(
            obj._objects,
            {(1, 0x00): ("a", 163, 1), (2, 0x00): ("b", 1, 2), (2, 0x08): ("c", 8, 2)},
        )
        self.assertDictEqual(obj._object_addr, {"a": (1, 0x00), "b": (2, 0x00), "c": (2, 0x08)})

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_load_level_addr(self, mock_zilinit):
        mock_elf = mock.Mock()

        sts = mock.Mock(spec=SymbolTableSection)
        rel = mock.Mock(spec=Section)
        mock_elf.iter_sections.return_value = [sts, rel]

        s0 = mock.Mock()
        s0.name = "__init_EARLY_start"
        s0.entry.st_value = 0x00

        s1 = mock.Mock()
        s1.name = "__init_PRE_KERNEL_1_start"
        s1.entry.st_value = 0x11

        s2 = mock.Mock()
        s2.name = "__init_PRE_KERNEL_2_start"
        s2.entry.st_value = 0x22

        s3 = mock.Mock()
        s3.name = "__init_POST_KERNEL_start"
        s3.entry.st_value = 0x33

        s4 = mock.Mock()
        s4.name = "__init_APPLICATION_start"
        s4.entry.st_value = 0x44

        s5 = mock.Mock()
        s5.name = "__init_SMP_start"
        s5.entry.st_value = 0x55

        s6 = mock.Mock()
        s6.name = "__init_end"
        s6.entry.st_value = 0x66

        sts.iter_symbols.return_value = [s0, s1, s2, s3, s4, s5, s6]

        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock_elf
        obj._relocatable = False
        obj._load_level_addr()

        self.assertDictEqual(
            obj._init_level_addr,
            {
                "EARLY": 0x00,
                "PRE_KERNEL_1": 0x11,
                "PRE_KERNEL_2": 0x22,
                "POST_KERNEL": 0x33,
                "APPLICATION": 0x44,
                "SMP": 0x55,
            },
        )
        self.assertEqual(obj._init_level_end, 0x66)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_device_ord_from_name(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("", None)

        self.assertEqual(obj._device_ord_from_name(None), None)
        self.assertEqual(obj._device_ord_from_name("hey, hi!"), None)
        self.assertEqual(obj._device_ord_from_name("__device_dts_ord_123"), 123)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_object_name(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._objects = {0x123: ("name", 4)}

        self.assertEqual(obj._object_name(0), "NULL")
        self.assertEqual(obj._object_name(73), "unknown")
        self.assertEqual(obj._object_name(0x123), "name")

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_32(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock.Mock()
        obj._elf.elfclass = 32
        obj._relocatable = False
        mock_section = mock.Mock()
        obj._elf.get_section.return_value = mock_section
        mock_section.header.sh_addr = 0x100
        mock_section.data.return_value = b"\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00"

        self.assertEqual(obj._initlevel_pointer(0x100, 0, 0), 1)
        self.assertEqual(obj._initlevel_pointer(0x100, 1, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x104, 0, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x104, 1, 0), 3)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_64(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock.Mock()
        obj._elf.elfclass = 64
        obj._relocatable = False
        mock_section = mock.Mock()
        obj._elf.get_section.return_value = mock_section
        mock_section.header.sh_addr = 0x100
        mock_section.data.return_value = (
            b"\x01\x00\x00\x00\x00\x00\x00\x00"
            b"\x02\x00\x00\x00\x00\x00\x00\x00"
            b"\x03\x00\x00\x00\x00\x00\x00\x00"
        )

        self.assertEqual(obj._initlevel_pointer(0x100, 0, 0), 1)
        self.assertEqual(obj._initlevel_pointer(0x100, 1, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x108, 0, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x108, 1, 0), 3)

    def _relocatable_pointer_obj(self, elfclass, rela):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._elf = mock.Mock()
        obj._elf.elfclass = elfclass
        obj._relocatable = True
        obj._relocs = {}

        ptrsize = elfclass // 8
        init_section = mock.Mock()
        # Entry 0: a relocated pointer with in-place addend 0x10 and a
        # relocated NULL pointer; entry 1: a constant and an unrelocated zero.
        init_section.data.return_value = (
            (0x10).to_bytes(ptrsize, "little")
            + (0).to_bytes(ptrsize, "little")
            + (7).to_bytes(ptrsize, "little")
            + (0).to_bytes(ptrsize, "little")
        )

        sym_a = mock.Mock()
        sym_a.entry.st_shndx = 5
        sym_a.entry.st_value = 0x20
        sym_b = mock.Mock()
        sym_b.entry.st_shndx = 6
        sym_b.entry.st_value = 0x00
        symtab = mock.Mock(spec=SymbolTableSection)
        symtab.get_symbol.side_effect = lambda idx: {1: sym_a, 2: sym_b}[idx]

        r0 = mock.Mock()
        r0.entry.r_offset = 0
        r0.entry.r_info_sym = 1
        r0.entry.r_addend = 0x30
        r1 = mock.Mock()
        r1.entry.r_offset = ptrsize
        r1.entry.r_info_sym = 2
        r1.entry.r_addend = 0
        rel_section = mock.Mock(spec=RelocationSection)
        rel_section.header = mock.Mock()
        rel_section.header.sh_info = 3
        rel_section.header.sh_link = 4
        rel_section.is_RELA.return_value = rela
        rel_section.iter_relocations.return_value = [r0, r1]

        other_rel_section = mock.Mock(spec=RelocationSection)
        other_rel_section.header = mock.Mock()
        other_rel_section.header.sh_info = 9

        obj._elf.iter_sections.return_value = [symtab, rel_section, other_rel_section]
        obj._elf.get_section.side_effect = lambda idx: {3: init_section, 4: symtab}[idx]

        return obj

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_relocatable_rel(self, mock_zilinit):
        obj = self._relocatable_pointer_obj(32, rela=False)

        self.assertEqual(obj._initlevel_pointer((3, 0), 0, 3), (5, 0x30))
        self.assertEqual(obj._initlevel_pointer((3, 0), 1, 3), (6, 0x00))
        self.assertEqual(obj._initlevel_pointer((3, 8), 0, 3), 7)
        self.assertEqual(obj._initlevel_pointer((3, 8), 1, 3), 0)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_relocatable_rela(self, mock_zilinit):
        obj = self._relocatable_pointer_obj(64, rela=True)

        self.assertEqual(obj._initlevel_pointer((3, 0), 0, 3), (5, 0x50))
        self.assertEqual(obj._initlevel_pointer((3, 0), 1, 3), (6, 0x00))
        self.assertEqual(obj._initlevel_pointer((3, 16), 0, 3), 7)
        self.assertEqual(obj._initlevel_pointer((3, 16), 1, 3), 0)

    @mock.patch("check_init_priorities.ZephyrInitLevels._object_name")
    @mock.patch("check_init_priorities.ZephyrInitLevels._initlevel_pointer")
    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_process_initlevels(self, mock_zilinit, mock_ip, mock_on):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._relocatable = False
        obj._init_level_addr = {
            "EARLY": 0x00,
            "PRE_KERNEL_1": 0x00,
            "PRE_KERNEL_2": 0x00,
            "POST_KERNEL": 0x08,
            "APPLICATION": 0x0C,
            "SMP": 0x0C,
        }
        obj._init_level_end = 0x0C
        obj._objects = {
            0x00: ("a", 4, 0),
            0x04: ("b", 4, 0),
            0x08: ("c", 4, 0),
        }
        obj._object_addr = {
            "__device_dts_ord_11": 0x00,
            "__device_dts_ord_22": 0x04,
        }

        mock_ip.side_effect = lambda *args: args

        def mock_obj_name(*args):
            if args[0] == (0, 5, 0):
                return "i0"
            elif args[0] == (0, 1, 0):
                return "__device_dts_ord_11"
            elif args[0] == (4, 5, 0):
                return "i1"
            elif args[0] == (4, 1, 0):
                return "__device_dts_ord_22"
            return f"name_{args[0][0]}_{args[0][1]}"

        mock_on.side_effect = mock_obj_name

        obj._process_initlevels()

        self.assertDictEqual(
            obj.initlevels,
            {
                "EARLY": [],
                "PRE_KERNEL_1": [],
                "PRE_KERNEL_2": ["a: i0(__device_dts_ord_11)", "b: i1(__device_dts_ord_22)"],
                "POST_KERNEL": ["c: name_8_0(name_8_1)"],
                "APPLICATION": [],
                "SMP": [],
            },
        )
        self.assertDictEqual(
            obj.devices,
            {
                11: (check_init_priorities.Priority("PRE_KERNEL_2", 0), "i0"),
                22: (check_init_priorities.Priority("PRE_KERNEL_2", 1), "i1"),
            },
        )

    @mock.patch("check_init_priorities.ZephyrInitLevels._object_name")
    @mock.patch("check_init_priorities.ZephyrInitLevels._initlevel_pointer")
    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_process_initlevels_relocatable(self, mock_zilinit, mock_ip, mock_on):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._relocatable = True
        obj._init_level_addr = {
            "EARLY": (7, 0x00),
            "PRE_KERNEL_1": (7, 0x00),
            "PRE_KERNEL_2": (7, 0x00),
            "POST_KERNEL": (7, 0x08),
            "APPLICATION": (7, 0x0C),
            "SMP": (7, 0x0C),
        }
        obj._init_level_end = (7, 0x0C)
        # Symbols from other sections share the init entries' offsets.
        obj._objects = {
            (1, 0x00): ("z_fatal_error", 163, 1),
            (2, 0x00): ("z_thread_release", 1, 2),
            (7, 0x00): ("a", 4, 7),
            (7, 0x04): ("b", 4, 7),
            (7, 0x08): ("c", 4, 7),
            (8, 0x00): ("__device_dts_ord_11", 4, 8),
        }
        obj._object_addr = {
            "__device_dts_ord_11": (8, 0x00),
        }

        mock_ip.side_effect = lambda *args: args

        def mock_obj_name(*args):
            if args[0] == ((8, 0x00), 5, 8):
                return "i0"
            elif args[0] == ((7, 0x00), 1, 7):
                return "__device_dts_ord_11"
            return f"name_{args[0][0][1]}_{args[0][1]}"

        mock_on.side_effect = mock_obj_name

        obj._process_initlevels()

        self.assertDictEqual(
            obj.initlevels,
            {
                "EARLY": [],
                "PRE_KERNEL_1": [],
                "PRE_KERNEL_2": ["a: i0(__device_dts_ord_11)", "b: name_4_0(name_4_1)"],
                "POST_KERNEL": ["c: name_8_0(name_8_1)"],
                "APPLICATION": [],
                "SMP": [],
            },
        )
        self.assertDictEqual(
            obj.devices,
            {11: (check_init_priorities.Priority("PRE_KERNEL_2", 0), "i0")},
        )

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_process_initlevels_relocatable_split_section(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("", None)
        obj._relocatable = True
        obj._init_level_addr = dict.fromkeys(check_init_priorities._DEVICE_INIT_LEVELS, (7, 0x00))
        obj._init_level_end = (8, 0x00)
        obj._objects = {}
        obj._object_addr = {}

        with self.assertRaises(ValueError):
            obj._process_initlevels()


class testValidator(unittest.TestCase):
    """Tests for the Validator class."""

    @mock.patch("check_init_priorities.ZephyrInitLevels")
    @mock.patch("pickle.load")
    def test_initialize(self, mock_pl, mock_zil):
        mock_log = mock.Mock()
        mock_prio = mock.Mock()
        mock_obj = mock.Mock()
        mock_obj.defined_devices = {123: mock_prio}
        mock_zil.return_value = mock_obj

        with mock.patch("builtins.open", mock.mock_open()) as mock_open:
            validator = check_init_priorities.Validator("path", "pickle", mock_log, None)

        self.assertEqual(validator._obj, mock_obj)
        mock_zil.assert_called_once_with("path", None)
        mock_open.assert_called_once_with(pathlib.Path("pickle"), "rb")

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_dep_same_node(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()

        validator._check_dep(123, 123)

        self.assertFalse(validator.log.info.called)
        self.assertFalse(validator.log.warning.called)
        self.assertFalse(validator.log.error.called)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_dep_no_prio(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].props = {}
        validator._ord2node[2]._binding = None
        validator._ord2node[2].props = {}

        validator._obj.devices = {1: (10, "i1")}
        validator._check_dep(1, 2)

        validator._obj.devices = {2: (20, "i2")}
        validator._check_dep(1, 2)

        self.assertFalse(validator.log.info.called)
        self.assertFalse(validator.log.warning.called)
        self.assertFalse(validator.log.error.called)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[1].props = {}
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"
        validator._ord2node[2].props = {}

        validator._obj.devices = {1: (10, "i1"), 2: (20, "i2")}

        validator._check_dep(2, 1)
        validator._check_dep(1, 2)

        validator.log.info.assert_called_once_with("/2 <i2> 20 > /1 <i1> 10")
        validator.log.error.assert_has_calls(
            [mock.call("/1 <i1> is initialized before its dependency /2 <i2> (10 < 20)")]
        )
        self.assertEqual(validator.errors, 1)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_same_prio_assert(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[1].props = {}
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"
        validator._ord2node[2].props = {}

        validator._obj.devices = {1: (10, "i1"), 2: (10, "i2")}

        with self.assertRaises(ValueError):
            validator._check_dep(1, 2)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_ignored(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        save_ignore_compatibles = check_init_priorities._IGNORE_COMPATIBLES

        check_init_priorities._IGNORE_COMPATIBLES = set(["compat-3"])

        validator._ord2node = {1: mock.Mock(), 3: mock.Mock()}
        validator._ord2node[1]._binding.compatible = "compat-1"
        validator._ord2node[1].path = "/1"
        validator._ord2node[3]._binding.compatible = "compat-3"
        validator._ord2node[3].path = "/3"

        validator._obj.devices = {1: 20, 3: 10}

        validator._check_dep(3, 1)

        self.assertListEqual(
            validator.log.info.call_args_list,
            [
                mock.call("Ignoring priority: compat-3"),
            ],
        )
        self.assertEqual(validator.errors, 0)

        check_init_priorities._IGNORE_COMPATIBLES = save_ignore_compatibles

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_deferred(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None, None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[1].props = {}
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"
        validator._ord2node[2].props = {
            check_init_priorities._DEFERRED_INIT_PROP_NAME: mock.Mock(val=True)
        }

        validator._obj.devices = {1: (10, "i1"), 2: (5, "i2")}

        validator._check_dep(2, 1)
        validator._check_dep(1, 2)

        validator.log.info.assert_called_once_with("Ignoring deferred device /2")
        validator.log.error.assert_has_calls(
            [mock.call("Non-deferred device /1 depends on deferred device /2")]
        )
        self.assertEqual(validator.errors, 1)

    @mock.patch("check_init_priorities.Validator._check_dep")
    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_edt(self, mock_vinit, mock_cd):
        d0 = mock.Mock()
        d0.dep_ordinal = 1
        d1 = mock.Mock()
        d1.dep_ordinal = 2
        d2 = mock.Mock()
        d2.dep_ordinal = 3

        dev0 = mock.Mock()
        dev0.depends_on = [d0]
        dev1 = mock.Mock()
        dev1.depends_on = [d1]
        dev2 = mock.Mock()
        dev2.depends_on = [d2]

        validator = check_init_priorities.Validator("", "", None, None)
        validator._ord2node = {1: dev0, 2: dev1, 3: dev2}
        validator._obj = mock.Mock()
        validator._obj.devices = {1: 10, 2: 10, 3: 20}

        validator.check_edt()

        self.assertListEqual(
            mock_cd.call_args_list,
            [
                mock.call(1, 1),
                mock.call(2, 2),
                mock.call(3, 3),
            ],
        )


if __name__ == "__main__":
    unittest.main()
