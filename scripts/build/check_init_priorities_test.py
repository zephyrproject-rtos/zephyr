#!/usr/bin/env python3

# Copyright 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0
"""
Tests for check_init_priorities
"""

import mock
import pathlib
import unittest

from elftools.elf.relocation import RelocationSection
from elftools.elf.sections import SymbolTableSection

import check_init_priorities

class TestPriority(unittest.TestCase):
    """Tests for the Priority class."""

    def test_priority_parsing(self):
        prio1 = check_init_priorities.Priority(".rel.z_init_POST_KERNEL12_")
        self.assertEqual(prio1._level_priority, 312)

        prio2 = check_init_priorities.Priority("noisenoise_POST_KERNEL99_")
        self.assertEqual(prio2._level_priority, 399)

        prio3 = check_init_priorities.Priority("_PRE_KERNEL_10_")
        self.assertEqual(prio3._level_priority, 100)

        prio4 = check_init_priorities.Priority("_PRE_KERNEL_110_")
        self.assertEqual(prio4._level_priority, 110)

        with self.assertRaises(ValueError):
            check_init_priorities.Priority("i-am-not-a-priority")
            check_init_priorities.Priority("_DOESNOTEXIST0_")
            check_init_priorities.Priority(".rel.z_init_POST_KERNEL12_blah")
            check_init_priorities.Priority(".rel.z_init_2_")
            check_init_priorities.Priority(".rel.z_init_POST_KERNEL1_")

    def test_priority_levels(self):
        prios = [
                check_init_priorities.Priority(".rel.z_init_EARLY0_"),
                check_init_priorities.Priority(".rel.z_init_EARLY1_"),
                check_init_priorities.Priority(".rel.z_init_EARLY11_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_10_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_11_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_111_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_20_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_21_"),
                check_init_priorities.Priority(".rel.z_init_PRE_KERNEL_211_"),
                check_init_priorities.Priority(".rel.z_init_POST_KERNEL0_"),
                check_init_priorities.Priority(".rel.z_init_POST_KERNEL1_"),
                check_init_priorities.Priority(".rel.z_init_POST_KERNEL11_"),
                check_init_priorities.Priority(".rel.z_init_APPLICATION0_"),
                check_init_priorities.Priority(".rel.z_init_APPLICATION1_"),
                check_init_priorities.Priority(".rel.z_init_APPLICATION11_"),
                check_init_priorities.Priority(".rel.z_init_SMP0_"),
                check_init_priorities.Priority(".rel.z_init_SMP1_"),
                check_init_priorities.Priority(".rel.z_init_SMP11_"),
                ]

        self.assertListEqual(prios, sorted(prios))

    def test_priority_strings(self):
        prio = check_init_priorities.Priority(".rel.z_init_POST_KERNEL12_")
        self.assertEqual(str(prio), "POST_KERNEL 12")
        self.assertEqual(repr(prio), "<Priority POST_KERNEL 12>")

class testZephyrObjectFile(unittest.TestCase):
    """Tests for the ZephyrObjectFile class."""

    @mock.patch("check_init_priorities.ZephyrObjectFile.__init__", return_value=None)
    def test_load_symbols(self, mock_zofinit):
        mock_elf = mock.Mock()

        sts = mock.Mock(spec=SymbolTableSection)
        rel = mock.Mock(spec=RelocationSection)
        mock_elf.iter_sections.return_value = [sts, rel]

        s0 = mock.Mock()
        s0.name = "a"
        s1 = mock.Mock()
        s1.name = None
        s2 = mock.Mock()
        s2.name = "b"
        sts.iter_symbols.return_value = [s0, s1, s2]

        obj = check_init_priorities.ZephyrObjectFile("")
        obj._elf = mock_elf
        obj._load_symbols()

        self.assertDictEqual(obj._symbols, {0: "a", 2: "b"})

    @mock.patch("check_init_priorities.Priority")
    @mock.patch("check_init_priorities.ZephyrObjectFile._device_ord_from_rel")
    @mock.patch("check_init_priorities.ZephyrObjectFile.__init__", return_value=None)
    def test_find_defined_devices(self, mock_zofinit, mock_dofr, mock_prio):
        mock_elf = mock.Mock()

        sts = mock.Mock(spec=SymbolTableSection)
        rel1 = mock.Mock(spec=RelocationSection)
        rel1.name = ".rel.z_init_SOMETHING"
        rel2 = mock.Mock(spec=RelocationSection)
        rel2.name = ".rel.something_else"
        mock_elf.iter_sections.return_value = [sts, rel1, rel2]

        r0 = mock.Mock()
        rel1.iter_relocations.return_value = [r0]

        mock_dofr.return_value = 123

        r0_prio = mock.Mock()
        mock_prio.return_value = r0_prio

        obj = check_init_priorities.ZephyrObjectFile("")
        obj._elf = mock_elf
        obj._find_defined_devices()

        self.assertDictEqual(obj.defined_devices, {123: r0_prio})
        mock_dofr.assert_called_once_with(r0)
        mock_prio.assert_called_once_with(rel1.name)

    @mock.patch("check_init_priorities.ZephyrObjectFile.__init__", return_value=None)
    def test_device_ord_from_rel(self, mock_zofinit):
        obj = check_init_priorities.ZephyrObjectFile("")

        obj._symbols = {
                1: "blah",
                2: "__device_dts_ord_123",
                }

        self.assertEqual(obj._device_ord_from_rel({"r_info_sym": 0}), None)
        self.assertEqual(obj._device_ord_from_rel({"r_info_sym": 1}), None)
        self.assertEqual(obj._device_ord_from_rel({"r_info_sym": 2}), 123)

class testValidator(unittest.TestCase):
    """Tests for the Validator class."""

    @mock.patch("check_init_priorities.ZephyrObjectFile")
    @mock.patch("check_init_priorities.Validator._find_build_objfiles")
    @mock.patch("pickle.load")
    def test_initialize(self, mock_pl, mock_fbo, mock_zof):
        mock_fbo.return_value = ["filepath"]

        mock_log = mock.Mock()
        mock_prio = mock.Mock()
        mock_obj = mock.Mock()
        mock_obj.defined_devices = {123: mock_prio}
        mock_zof.return_value = mock_obj

        with mock.patch("builtins.open", mock.mock_open()) as mock_open:
            validator = check_init_priorities.Validator("path", "pickle", mock_log)

        self.assertListEqual(validator._objs, [mock_obj])
        self.assertDictEqual(validator._dev_priorities, {123: mock_prio})
        mock_fbo.assert_called_once_with("path", is_root=True)
        mock_zof.assert_called_once_with("filepath")
        mock_open.assert_called_once_with(pathlib.Path("path/pickle"), "rb")

    @mock.patch("pathlib.Path")
    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_find_build_objfiles(self, mock_vinit, mock_path):
        mock_log = mock.Mock()

        validator = check_init_priorities.Validator("", "", mock_log)

        mock_file = mock.Mock()
        mock_file.is_file.return_value = True
        mock_file.is_dir.return_value = False
        mock_file.file.name = "filename.c.obj"

        mock_dir = mock.Mock()
        mock_dir.is_file.return_value = False
        mock_dir.is_dir.return_value = True
        mock_dir.resolve.return_value = "subdir"
        mock_dir.iterdir.return_value = []

        mock_nosettingsfile = mock.Mock()
        mock_nosettingsfile.exists.return_value = False

        mock_path_root = mock.Mock()
        mock_path_root.iterdir.return_value = [mock_file, mock_dir]

        def mock_path_stubs(*args):
            if args == ("root",):
                return mock_path_root
            elif args == ("subdir",):
                return mock_dir
            elif args == ("subdir", "CMakeCache.txt"):
                return mock_nosettingsfile
            raise ValueError
        mock_path.side_effect = mock_path_stubs

        ret = list(validator._find_build_objfiles("root", is_root=True))
        self.assertListEqual(ret, [mock_file])

        mock_nosettingsfile.exists.assert_called_once_with()
        self.assertListEqual(mock_path.call_args_list, [
            mock.call("root"),
            mock.call("subdir", "CMakeCache.txt"),
            mock.call("subdir")
            ])

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

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[2]._binding = None

        validator._dev_priorities = {1: 10}
        validator._check_dep(1, 2)

        validator._dev_priorities = {2: 20}
        validator._check_dep(1, 2)

        self.assertFalse(validator.log.info.called)
        self.assertFalse(validator.log.warning.called)
        self.assertFalse(validator.log.error.called)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()
        validator.warnings = 0
        validator.errors = 0

        validator._ord2node = {1: mock.Mock(), 2: mock.Mock(), 3: mock.Mock()}
        validator._ord2node[1]._binding = None
        validator._ord2node[1].path = "/1"
        validator._ord2node[2]._binding = None
        validator._ord2node[2].path = "/2"
        validator._ord2node[3]._binding = None
        validator._ord2node[3].path = "/3"

        validator._dev_priorities = {1: 10, 2: 10, 3: 20}

        validator._check_dep(3, 1)
        validator._check_dep(2, 1)
        validator._check_dep(1, 3)

        validator.log.info.assert_called_once_with("/3 20 > /1 10")
        validator.log.warning.assert_called_once_with("/2 10 == /1 10")
        validator.log.error.assert_called_once_with("/1 10 < /3 20")
        self.assertEqual(validator.warnings, 1)
        self.assertEqual(validator.errors, 1)

    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_swapped(self, mock_vinit):
        validator = check_init_priorities.Validator("", "", None)
        validator.log = mock.Mock()
        validator.warnings = 0
        validator.errors = 0

        check_init_priorities._INVERTED_PRIORITY_COMPATIBLES = set([("compat-3", "compat-1")])

        validator._ord2node = {1: mock.Mock(), 3: mock.Mock()}
        validator._ord2node[1]._binding.compatible = "compat-1"
        validator._ord2node[1].path = "/1"
        validator._ord2node[3]._binding.compatible = "compat-3"
        validator._ord2node[3].path = "/3"

        validator._dev_priorities = {1: 20, 3: 10}

        validator._check_dep(3, 1)

        self.assertListEqual(validator.log.info.call_args_list, [
            mock.call("Swapped priority: compat-3, compat-1"),
            mock.call("/3 20 > /1 10"),
        ])

    @mock.patch("check_init_priorities.Validator._check_dep")
    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_edt_r(self, mock_vinit, mock_cd):
        validator = check_init_priorities.Validator("", "", None)

        d0 = mock.Mock()
        d0.dep_ordinal = 1
        d1 = mock.Mock()
        d1.dep_ordinal = 2

        c0 = mock.Mock()
        c0.props = {"compatible": "c"}
        c1 = mock.Mock()
        c1.props = {}
        c1._binding.path = "another-binding-path.yaml"
        c2 = mock.Mock()
        c2.props = {}
        c2._binding.path = "binding-path.yaml"
        c2._binding.child_binding = None
        c2.depends_on = [d1]

        dev = mock.Mock()
        dev.depends_on = [d0]
        dev._binding.child_binding = "child-binding"
        dev._binding.path = "binding-path.yaml"
        dev.children.values.return_value = [c0, c1, c2]

        validator._check_edt_r(0, dev)

        self.assertListEqual(mock_cd.call_args_list, [
            mock.call(0, 1),
            mock.call(0, 2),
            ])

    @mock.patch("check_init_priorities.Validator._check_edt_r")
    @mock.patch("check_init_priorities.Validator.__init__", return_value=None)
    def test_check_edt(self, mock_vinit, mock_cer):
        validator = check_init_priorities.Validator("", "", None)
        validator._ord2node = {1: mock.Mock(), 2: mock.Mock(), 3: mock.Mock()}
        validator._dev_priorities = {1: 10, 2: 10, 3: 20}

        validator.check_edt()

        self.assertListEqual(mock_cer.call_args_list, [
            mock.call(1, validator._ord2node[1]),
            mock.call(2, validator._ord2node[2]),
            mock.call(3, validator._ord2node[3]),
            ])

if __name__ == "__main__":
    unittest.main()
