# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

import pathlib
import sys
from types import SimpleNamespace
from unittest import mock

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "pinctrl"))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "clock"))

import dedup_pinctrl
import gen_ccm_rev3_ids
import gen_soc_headers
from imx import imx_cfg_utils
from kinetis import kinetis_cfg_utils
from lpc import lpc_cfg_utils
from shared import pinctrl_layout
from socs.rt266x import CONFIG

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
ZEPHYR = SCRIPTS.parents[2]


@pytest.mark.parametrize(
    "part,suffix,subdir",
    [
        ("MCXA344VLL", ".h", "mcx/mcxa"),
        ("MIMXRT595SFFOC", ".h", "imxrt/imxrt5xx"),
        ("LPC55S69JBD100", ".h", "lpc/lpc55xxx"),
        ("MK02FN128VFM10", ".h", "kinetis/k0x"),
        ("MKM35Z512VLL7", ".h", "kinetis/km3x"),
        ("MKV31F512VLL12", ".h", "kinetis/kv3x"),
        ("RW612ETA2I", ".h", "rw"),
        ("S32K344", ".h", "s32"),
        ("MIMXRT1186CVJ8C", ".dtsi", "imxrt/imxrt118x/pinctrl"),
        ("MIMXRT2663CHPAA", ".dtsi", "imxrt/imxrt266x/pinctrl"),
        ("MIMX9352CVVXM", ".dtsi", "imx/pinctrl"),
    ],
)
def test_family_series_layout(part, suffix, subdir):
    assert pinctrl_layout.resolve_subdir(part, suffix) == subdir


@pytest.mark.parametrize("zephyr_base", [None, "./", str(ZEPHYR)])
def test_default_roots_and_clock_output(monkeypatch, zephyr_base):
    monkeypatch.chdir(ZEPHYR)
    if zephyr_base is None:
        monkeypatch.delenv("ZEPHYR_BASE", raising=False)
    else:
        monkeypatch.setenv("ZEPHYR_BASE", zephyr_base)
    assert pinctrl_layout.output_root(".h") == (ZEPHYR / "include/zephyr/dt-bindings/pinctrl/nxp")
    assert pinctrl_layout.output_root(".dtsi") == ZEPHYR / "dts/arm/nxp"
    assert gen_ccm_rev3_ids.default_output(CONFIG) == (
        ZEPHYR / "include/zephyr/dt-bindings/clock/imx_ccm_rev3_rt266x.h"
    )
    assert dedup_pinctrl._find_zephyr(SCRIPTS).resolve() == ZEPHYR


def test_all_committed_definitions_match_generated_paths():
    count = 0
    for suffix in (".h", ".dtsi"):
        root = pinctrl_layout.output_root(suffix)
        for path in root.rglob(f"*-pinctrl{suffix}"):
            part = path.name.removesuffix(f"-pinctrl{suffix}")
            subdir = pinctrl_layout.resolve_subdir(part, suffix)
            assert subdir is not None, path
            assert root / subdir / path.name == path
            include = pinctrl_layout.include_path(part, suffix)
            include_root = ZEPHYR / ("include" if suffix == ".h" else "dts/arm")
            assert include_root / include == path
            count += 1
    assert count > 0


@pytest.mark.parametrize(
    "module,part,suffix",
    [
        (kinetis_cfg_utils, "MCXA344VLL", ".h"),
        (kinetis_cfg_utils, "MK64FN1M0VLL12", ".h"),
        (lpc_cfg_utils, "LPC55S69JBD100", ".h"),
        (lpc_cfg_utils, "MIMXRT595SFFOC", ".h"),
        (imx_cfg_utils, "MIMXRT1186CVJ8C", ".dtsi"),
        (imx_cfg_utils, "MIMXRT2663CHPAA", ".dtsi"),
        (imx_cfg_utils, "MIMX9352CVVXM", ".dtsi"),
    ],
)
def test_board_writer_includes(module, part, suffix, tmp_path):
    util = module.NXPSdkUtil.__new__(module.NXPSdkUtil)
    util._copyright = ""
    util._part_num = part
    util._soc_sku = part
    util._signal_map = {}
    group = mock.Mock()
    group.get_name.return_value = "uart_default"
    group.get_description.return_value = "UART pins"
    group.get_pin_props.return_value = [("input-enable",), ("output-enable",)]
    pin = mock.Mock()
    pin.get_name.return_value = "PIN0"
    group.get_pins.return_value = [pin]
    output = tmp_path / "board-pinctrl.dtsi"
    with (
        mock.patch.object(util, "_parse_mex_cfg", return_value={"UART": group}),
        mock.patch.object(module, "get_package_name", return_value=part),
    ):
        util.write_pinctrl_groups("board.mex", output)
    content = output.read_text()
    assert f"#include <{pinctrl_layout.include_path(part, suffix)}>" in content
    assert "group0 {" in content and "group1 {" in content and "group2 {" not in content
    assert "input-enable;" in content and "output-enable;" in content
    assert ("<&pin0>" if suffix == ".dtsi" else "<PIN0>") in content


def test_invalid_layout_is_explicit():
    assert pinctrl_layout.resolve_subdir("UNKNOWN") is None
    with pytest.raises(ValueError, match="Unknown pinctrl family"):
        pinctrl_layout.include_path("UNKNOWN", ".h")
    with pytest.raises(ValueError, match="Unsupported pinctrl file suffix"):
        pinctrl_layout.output_root(".c")
    with pytest.raises(ValueError, match="Unsupported pinctrl file suffix"):
        pinctrl_layout.resolve_subdir("MCXA344VLL", ".c")


@pytest.mark.parametrize("controller,suffix", [("PORT", ".h"), ("IOCON", ".h"), ("IOMUX", ".dtsi")])
def test_soc_generator_default_output(controller, suffix, tmp_path, monkeypatch):
    part = {"PORT": "MCXA344VLL", "IOCON": "LPC55S69JBD100", "IOMUX": "MIMXRT1186CVJ8C"}[controller]
    pack = tmp_path / "pack"
    package = pack / "processors/processor/ksdk2_0/package"
    package.mkdir(parents=True)
    (package / "signal_configuration.xml").touch()
    utility = mock.Mock()
    utility.get_part_num.return_value = part
    utility.write_pinctrl_defs.return_value = []
    utility.build_daisy_maps.return_value = {"unparsed": []}
    args = mock.Mock(
        soc_output=None,
        controller=controller,
        data_pack="pack.zip",
        copyright=False,
        no_dedup=True,
        iomuxc_file="fsl_iomuxc.h",
    )
    module = {"PORT": kinetis_cfg_utils, "IOCON": lpc_cfg_utils, "IOMUX": imx_cfg_utils}[controller]
    monkeypatch.setattr(gen_soc_headers, "parse_args", lambda: args)
    monkeypatch.setattr(gen_soc_headers, "get_pack_version", lambda _: 26.06)
    monkeypatch.setattr(
        gen_soc_headers.tempfile, "TemporaryDirectory", lambda: SimpleNamespace(name=str(pack))
    )
    monkeypatch.setattr(gen_soc_headers.zipfile, "ZipFile", mock.Mock())
    monkeypatch.setattr(module, "NXPSdkUtil", lambda *a, **k: utility)
    monkeypatch.setattr(pinctrl_layout, "output_root", lambda _: tmp_path / "output")
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "parse_iomuxc_ground_truth", lambda _: {})
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "validate_pinctrl_file", lambda *a: {})
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "format_report", lambda *a: [])
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "report_is_fatal", lambda _: False)
    gen_soc_headers.main()
    filename = (part.lower() if suffix == ".dtsi" else part.upper()) + "-pinctrl" + suffix
    expected = tmp_path / "output" / pinctrl_layout.resolve_subdir(part, suffix) / filename
    assert utility.write_pinctrl_defs.call_args.args[0] == str(expected)
