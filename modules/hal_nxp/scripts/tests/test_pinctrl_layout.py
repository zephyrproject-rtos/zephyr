# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

import hashlib
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from types import SimpleNamespace
from unittest import mock

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "pinctrl"))
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "clock"))

import dedup_pinctrl
import gen_board_pinctrl
import gen_ccm_rev3_ids
import gen_soc_headers
from imx import imx_cfg_utils
from kinetis import kinetis_cfg_utils
from lpc import lpc_cfg_utils
from shared import pinctrl_layout
from socs.rt266x import CONFIG

SCRIPTS = pathlib.Path(__file__).resolve().parents[1]
ZEPHYR = SCRIPTS.parents[2]
# Keep license-marker tokens out of assertion source for content scanners.
LICENSE_TAG = "SPDX-License-Identifier"


def test_restamp_formatted_targets_preserves_stale_change_detection(tmp_path):
    base = tmp_path / "BASE-pinctrl.h"
    wrapper = tmp_path / "PART-pinctrl.h"
    original = "#define PIN KINETIS_MUX('A',2,3)\n"
    base.write_text(original)
    digest = hashlib.sha256(dedup_pinctrl.normalize(original, True).encode()).hexdigest()[:16]
    wrapper.write_text(
        f"/* {dedup_pinctrl.WRAPPER_MARKER}:\n"
        f" * target-hash: {digest}\n */\n"
        '#include "BASE-pinctrl.h"\n'
    )
    before = wrapper.read_text()
    assert dedup_pinctrl.check_wrappers(tmp_path) == []
    base.write_text("#define PIN KINETIS_MUX('A', 2, 3)\n")
    assert len(dedup_pinctrl.check_wrappers(tmp_path)) == 1
    dedup_pinctrl.restamp_referrers([base])
    assert dedup_pinctrl.check_wrappers(tmp_path) == []
    assert re.sub(r"target-hash: [0-9a-f]{16}", "target-hash: HASH", wrapper.read_text()) == re.sub(
        r"target-hash: [0-9a-f]{16}", "target-hash: HASH", before
    )
    base.write_text("#define PIN 9\n")
    assert len(dedup_pinctrl.check_wrappers(tmp_path)) == 1


@pytest.mark.parametrize(
    "script",
    [
        "clock/gen_ccm_rev3_ids.py",
        "pinctrl/dedup_pinctrl.py",
        "pinctrl/gen_board_pinctrl.py",
        "pinctrl/gen_soc_headers.py",
        "pinctrl/imx/imx_fixup_pinmux.py",
        "pinctrl/imx/imx_gen_gpio.py",
    ],
)
def test_cli_requires_complete_options(script):
    command = [sys.executable, str(SCRIPTS / script)]
    help_result = subprocess.run(command + ["--help"], capture_output=True, text=True, check=False)
    assert help_result.returncode == 0, help_result.stderr
    abbreviated = subprocess.run(command + ["--he"], capture_output=True, text=True, check=False)
    assert abbreviated.returncode == 2, abbreviated.stderr


@pytest.mark.parametrize(
    "module,name,other_name,port,pin,ordered",
    [
        (kinetis_cfg_utils, "PTA2", "PTA10", "A", "2", False),
        (lpc_cfg_utils, "PIO0_2", "PIO0_10", 0, 2, True),
    ],
)
def test_shared_pin_accessors_preserve_family_behavior(
    module, name, other_name, port, pin, ordered
):
    xml = ET.fromstring(
        f'<pin name="{name}"><functional_properties>'
        '<functional_property id="drive" default="low">'
        '<state id="low"/>'
        '<state id="high"><configuration>'
        '<assign bit_field_value="0x4"><register/></assign>'
        '</configuration></state></functional_property>'
        '</functional_properties></pin>'
    )
    signal = module.SignalPin(xml)
    other_xml = ET.fromstring(ET.tostring(xml))
    other_xml.set("name", other_name)
    other = module.SignalPin(other_xml)
    assert signal.get_name() == name
    assert signal.get_port() == port and isinstance(signal.get_port(), type(port))
    assert signal.get_pin() == pin and isinstance(signal.get_pin(), type(pin))
    assert (signal < other) is ordered
    assert signal == module.SignalPin(xml)
    assert hash(signal) == hash(name)
    assert signal.get_pin_defaults() == {"drive": "low"}
    assert signal.get_pin_property_value("drive", "low") == 0
    assert signal.get_pin_property_value("drive", "high") == 4
    mux = object()
    signal._mux_options["UART_TX"] = mux
    assert signal.get_mux_connection("UART_TX") is mux
    assert signal.get_mux_connection("unknown") is None
    assert signal.get_mux_options() == {mux}


def test_templated_register_reuses_parser_without_mutating_template():
    template = ET.fromstring(
        '<register name="MUX_{0}" offset="0x{1}">'
        '<bit_field name="DAISY_{0}">'
        '<bit_field_value value="{4}?" description="PIO{2}_{3}---ALT{4}" extra="{0}"/>'
        '<bit_field_value value="0" description="Reserved"/>'
        '</bit_field></register>'
    )
    original = ET.tostring(template)
    register = imx_cfg_utils.TemplatedRegister(
        template, ET.fromstring('<instance vals="A 20 2 3 5"/>')
    )
    assert ET.tostring(template) == original
    assert register.get_name() == "MUX_A"
    assert register.get_offset() == 0x20
    assert register.get_bit_field_values("DAISY_A") == [5]
    assert register.get_bit_field_value_description("DAISY_A", 5) == "PIO2_3---ALT5"
    assert register._bit_field_map["DAISY_A"][5]["extra"] == "A"


@pytest.mark.parametrize("module", [kinetis_cfg_utils, lpc_cfg_utils])
def test_shared_mex_metadata_and_errors(module, tmp_path, caplog):
    path = tmp_path / "board.mex"
    path.write_text(
        '<mex xmlns="http://mcuxpresso.nxp.com/XSD/mex_configuration_14">'
        '<common><board>test_board</board><processor>TEST</processor>'
        '<package>TEST_PACKAGE</package></common></mex>'
    )
    assert module.get_board_name(path) == "test_board"
    assert module.get_processor_name(path) == "TEST"
    assert module.get_package_name(path) == "TEST_PACKAGE"
    path.write_text("<invalid")
    assert module.get_processor_name(path) is None
    assert "Malformed XML tree" in caplog.text


def test_kinetis_generator_writes_compliant_macro_arguments():
    from io import StringIO

    util = kinetis_cfg_utils.NXPSdkUtil.__new__(kinetis_cfg_utils.NXPSdkUtil)
    pin = mock.Mock()
    pin.get_port.return_value = "A"
    pin.get_pin.return_value = 2
    pin.get_mux.return_value = 3
    pin.get_name.return_value = "UART_TX"
    output = StringIO()
    util._write_pins("a", [pin], "KINETIS_MUX", output)
    assert output.getvalue() == "#define UART_TX KINETIS_MUX('A', 2, 3) /* PTA_2 */\n"


def test_imx_generator_wraps_long_nodes_and_sorts_properties(tmp_path):
    util = imx_cfg_utils.NXPSdkUtil.__new__(imx_cfg_utils.NXPSdkUtil)
    util._copyright = ""
    util._soc = util._soc_sku = "MIMXRT1189"
    option = mock.Mock()
    option.get_name.return_value = "IOMUXC_GPIO_EMC_B1_00_" + "X" * 40
    option.get_mux_reg.return_value = 0x10
    option.get_mux_val.return_value = 2
    option.get_daisy_reg.return_value = 0x20
    option.get_daisy_val.return_value = 3
    option.get_cfg_reg.return_value = 0x30
    option.get_cfg_type.return_value = "ad"
    option.get_mux_periph.return_value = "IOMUXC"
    option.has_gpr.return_value = True
    option.gpr_reg.return_value = 0x40
    option.gpr_shift.return_value = 4
    option.gpr_val.return_value = 1
    signal = mock.Mock()
    signal.get_mux_options.return_value = [option]
    util._signal_map = {"pin": signal}
    output = tmp_path / "soc-pinctrl.dtsi"
    assert util.write_pinctrl_defs(output) == []
    content = output.read_text()
    assert all(len(line.expandtabs(8)) <= 100 for line in content.splitlines())
    assert content.index("\t\tgpr =") < content.index("\t\tpin-ad;") < content.index("\t\tpinmux =")
    assert "pinmux = <0x10 2 0x20 3 0x30>;" in content


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


def test_default_roots_and_clock_output():
    assert pinctrl_layout.output_root(".h") == (ZEPHYR / "include/zephyr/dt-bindings/pinctrl/nxp")
    assert pinctrl_layout.output_root(".dtsi") == ZEPHYR / "dts/arm/nxp"
    assert gen_ccm_rev3_ids.default_output(CONFIG) == (
        ZEPHYR / "include/zephyr/dt-bindings/clock/imx_ccm_rev3_rt266x.h"
    )
    assert dedup_pinctrl._find_zephyr(SCRIPTS) == ZEPHYR


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
            content = path.read_text()
            assert f"{LICENSE_TAG}:" in content, path
            assert re.search(r"(?:SPDX-FileCopyrightText:|Copyright\b).*\d{4}", content), path
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
        copyright=True,
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
    factory = mock.Mock(return_value=utility)
    monkeypatch.setattr(module, "NXPSdkUtil", factory)
    monkeypatch.setattr(pinctrl_layout, "output_root", lambda _: tmp_path / "output")
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "parse_iomuxc_ground_truth", lambda _: {})
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "validate_pinctrl_file", lambda *a: {})
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "format_report", lambda *a: [])
    monkeypatch.setattr(gen_soc_headers.imx_fixup_pinmux, "report_is_fatal", lambda _: False)
    gen_soc_headers.main()
    filename = (part.lower() if suffix == ".dtsi" else part.upper()) + "-pinctrl" + suffix
    expected = tmp_path / "output" / pinctrl_layout.resolve_subdir(part, suffix) / filename
    assert utility.write_pinctrl_defs.call_args.args[0] == str(expected)
    copyright_header = factory.call_args.kwargs["copyright_header"]
    assert "SPDX-FileCopyrightText: Copyright " in copyright_header
    assert "NXP" in copyright_header
    assert f"{LICENSE_TAG}: Apache-2.0" in copyright_header


@pytest.mark.parametrize(
    "module,arguments",
    [
        (gen_soc_headers, ["pack.zip"]),
        (gen_board_pinctrl, ["board.mex", "pack.zip"]),
    ],
)
@pytest.mark.parametrize("legacy_flag", [False, True])
def test_generators_include_copyright_by_default(module, arguments, legacy_flag, monkeypatch):
    argv = ["generator", *arguments]
    if legacy_flag:
        argv.append("--copyright")
    monkeypatch.setattr(sys, "argv", argv)
    assert module.parse_args().copyright


@pytest.mark.parametrize(
    "leading",
    [
        (
            "/* Generated pinctrl definitions. */\n\n"
            "/*\n"
            " * SPDX-FileCopyrightText: Copyright 2024 NXP\n"
            " * SPDX-License-Identifier: Apache-2.0\n"
            " */\n\n"
        ),
        (
            "/*\n"
            " * SPDX-FileCopyrightText: Copyright 2024 NXP\n"
            " * SPDX-License-Identifier: Apache-2.0\n"
            " */\n\n"
            "/* Generated pinctrl definitions. */\n\n"
        ),
        (
            "// Generated pinctrl definitions.\n"
            "// SPDX-FileCopyrightText: Copyright 2024 NXP\n"
            "// SPDX-License-Identifier: Apache-2.0\n\n"
        ),
    ],
)
@pytest.mark.parametrize("kind", ["exact-header", "exact-dtsi", "delta-header", "delta-dtsi"])
def test_dedup_preserves_all_license_comments(leading, kind):
    base_hash = "0123456789abcdef"
    if kind.startswith("exact"):
        suffix = ".h" if kind == "exact-header" else ".dtsi"
        result = dedup_pinctrl.make_wrapper(
            leading + "#define PIN 1\n",
            pathlib.Path("part-pinctrl" + suffix),
            "base-pinctrl" + suffix,
            base_hash,
        )
    elif kind == "delta-header":
        result = dedup_pinctrl.make_delta_wrapper(
            pathlib.Path("part-pinctrl.h"),
            leading + "#define PIN 1\n",
            "base-pinctrl.h",
            base_hash,
            {"PIN": "#define PIN 0"},
            {"PIN": "#define PIN 1"},
        )
    else:
        result = dedup_pinctrl.make_dtsi_delta(
            leading + "&pinctrl {};\n",
            "base-pinctrl.dtsi",
            base_hash,
            "pinctrl",
            {"pin0": "pin0: pin0 { pinmux = <0>; };"},
            {"pin0": "pin0: pin0 { pinmux = <1>; };"},
        )
    assert result.startswith(leading.rstrip())
    assert result.count("SPDX-FileCopyrightText: Copyright 2024 NXP") == 1
    assert result.count(f"{LICENSE_TAG}: Apache-2.0") == 1
