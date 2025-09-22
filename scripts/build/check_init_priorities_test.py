#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0
"""
Tests for check_init_priorities
"""

import mock
import pathlib
import unittest

from elftools.elf.relocation import Section
from elftools.elf.sections import SymbolTableSection

import check_init_priorities

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
        s0.entry.st_value = 0xaa
        s0.entry.st_shndx = 1

        s1 = mock.Mock()
        s1.name = None

        s2 = mock.Mock()
        s2.name = "b"
        s2.entry.st_info.type = "STT_FUNC"
        s2.entry.st_size = 8
        s2.entry.st_value = 0xbb
        s2.entry.st_shndx = 2

        sts.iter_symbols.return_value = [s0, s1, s2]

        obj = check_init_priorities.ZephyrInitLevels("")
        obj._elf = mock_elf
        obj._load_objects()

        self.assertDictEqual(obj._objects, {0xaa: ("a", 4, 1), 0xbb: ("b", 8, 2)})

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

        obj = check_init_priorities.ZephyrInitLevels("")
        obj._elf = mock_elf
        obj._load_level_addr()

        self.assertDictEqual(obj._init_level_addr, {
            "EARLY": 0x00,
            "PRE_KERNEL_1": 0x11,
            "PRE_KERNEL_2": 0x22,
            "POST_KERNEL": 0x33,
            "APPLICATION": 0x44,
            "SMP": 0x55,
            })
        self.assertEqual(obj._init_level_end, 0x66)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_device_ord_from_name(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("")

        self.assertEqual(obj._device_ord_from_name(None), None)
        self.assertEqual(obj._device_ord_from_name("hey, hi!"), None)
        self.assertEqual(obj._device_ord_from_name("__device_dts_ord_123"), 123)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_object_name(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("")
        obj._objects = {0x123: ("name", 4)}

        self.assertEqual(obj._object_name(0), "NULL")
        self.assertEqual(obj._object_name(73), "unknown")
        self.assertEqual(obj._object_name(0x123), "name")

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_32(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("")
        obj._elf = mock.Mock()
        obj._elf.elfclass = 32
        mock_section = mock.Mock()
        obj._elf.get_section.return_value = mock_section
        mock_section.header.sh_addr = 0x100
        mock_section.data.return_value = (b"\x01\x00\x00\x00"
                                          b"\x02\x00\x00\x00"
                                          b"\x03\x00\x00\x00")

        self.assertEqual(obj._initlevel_pointer(0x100, 0, 0), 1)
        self.assertEqual(obj._initlevel_pointer(0x100, 1, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x104, 0, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x104, 1, 0), 3)

    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_initlevel_pointer_64(self, mock_zilinit):
        obj = check_init_priorities.ZephyrInitLevels("")
        obj._elf = mock.Mock()
        obj._elf.elfclass = 64
        mock_section = mock.Mock()
        obj._elf.get_section.return_value = mock_section
        mock_section.header.sh_addr = 0x100
        mock_section.data.return_value = (b"\x01\x00\x00\x00\x00\x00\x00\x00"
                                          b"\x02\x00\x00\x00\x00\x00\x00\x00"
                                          b"\x03\x00\x00\x00\x00\x00\x00\x00")

        self.assertEqual(obj._initlevel_pointer(0x100, 0, 0), 1)
        self.assertEqual(obj._initlevel_pointer(0x100, 1, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x108, 0, 0), 2)
        self.assertEqual(obj._initlevel_pointer(0x108, 1, 0), 3)

    @mock.patch("check_init_priorities.ZephyrInitLevels._object_name")
    @mock.patch("check_init_priorities.ZephyrInitLevels._initlevel_pointer")
    @mock.patch("check_init_priorities.ZephyrInitLevels.__init__", return_value=None)
    def test_process_initlevels(self, mock_zilinit, mock_ip, mock_on):
        obj = check_init_priorities.ZephyrInitLevels("")
        obj._init_level_addr = {
            "EARLY": 0x00,
            "PRE_KERNEL_1": 0x00,
            "PRE_KERNEL_2": 0x00,
            "POST_KERNEL": 0x08,
            "APPLICATION": 0x0c,
            "SMP": 0x0c,
            }
        obj._init_level_end = 0x0c
        obj._objects = {
                0x00: ("a", 4, 0),
                0x04: ("b", 4, 0),
                0x08: ("c", 4, 0),
                }

        mock_ip.side_effect = lambda *args: args

        def mock_obj_name(*args):
            if args[0] == (0, 0, 0):
                return "i0"
            elif args[0] == (0, 1, 0):
                return "__device_dts_ord_11"
            elif args[0] == (4, 0, 0):
                return "i1"
            elif args[0] == (4, 1, 0):
                return "__device_dts_ord_22"
            return f"name_{args[0][0]}_{args[0][1]}"
        mock_on.side_effect = mock_obj_name

        obj._process_initlevels()

        self.assertDictEqual(obj.initlevels, {
            "EARLY": [],
            "PRE_KERNEL_1": [],
            "PRE_KERNEL_2": ["a: i0(__device_dts_ord_11)", "b: i1(__device_dts_ord_22)"],
            "POST_KERNEL": ["c: name_8_0(name_8_1)"],
            "APPLICATION": [],
            "SMP": [],
            })
        self.assertDictEqual(obj.devices, {
            11: (check_init_priorities.Priority("PRE_KERNEL_2", 0), "i0"),
            22: (check_init_priorities.Priority("PRE_KERNEL_2", 1), "i1"),
            })

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
            validator = check_init_priorities.Validator("path", "pickle", mock_log)

        self.assertEqual(validator._obj, mock_obj)
        mock_zil.assert_called_once_with("path")
        mock_open.assert_called_once_with(pathlib.Path("pickle"), "rb")

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_dep_same_node(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()

        validator._check_dep(123, 123)

        self.assertFalse(validator.log.info.called)
        self.assertFalse(validator.log.warning.called)
        self.assertFalse(validator.log.error.called)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_dep_no_prio(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[2]._binding = None

        validator._obj.devices = {1: (10, "i1")}
        validator._check_dep(1, 2)

        validator._obj.devices = {2: (20, "i2")}
        validator._check_dep(1, 2)

        self.assertFalse(validator.log.info.called)
        self.assertFalse(validator.log.warning.called)
        self.assertFalse(validator.log.error.called)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"

        validator._obj.devices = {1: (10, "i1"), 2: (20, "i2")}

        validator._check_dep(2, 1)
        validator._check_dep(1, 2)

        validator.log.info.assert_called_once_with("/2 <i2> 20 > /1 <i1> 10")
        validator.log.error.assert_has_calls([
            mock.call("/1 <i1> is initialized before its dependency /2 <i2> (10 < 20)")
            ])
        self.assertEqual(validator.errors, 1)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_same_prio_assert(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()
        validator._obj = mock.Mock()
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"

        validator._obj.devices = {1: (10, "i1"), 2: (10, "i2")}

        with self.assertRaises(ValueError):
            validator._check_dep(1, 2)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_ignored(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
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

        self.assertListEqual(validator.log.info.call_args_list, [
            mock.call("Ignoring priority: compat-3"),
        ])
        self.assertEqual(validator.errors, 0)

        check_init_priorities._IGNORE_COMPATIBLES = save_ignore_compatibles

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

        validator = check_init_priorities.Validator("", "", None)
        validator._ord2node = {1: dev0, 2: dev1, 3: dev2}
        validator._obj = mock.Mock()
        validator._obj.devices = {1: 10, 2: 10, 3: 20}

        validator.check_edt()

        self.assertListEqual(mock_cd.call_args_list, [
            mock.call(1, 1),
            mock.call(2, 2),
            mock.call(3, 3),
            ])

if __name__ == "__main__":
    unittest.main()
