# SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).parents[1] / "tools/rt1170_demo.py"
SPEC = importlib.util.spec_from_file_location("rt1170_demo", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
rt1170_demo = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = rt1170_demo
SPEC.loader.exec_module(rt1170_demo)


class SettingsTest(unittest.TestCase):
    def write_profile(self, directory: Path, contents: str, mode: int = 0o600) -> Path:
        path = directory / "settings.toml"
        path.write_text(contents, encoding="utf-8")
        path.chmod(mode)
        return path

    def profile(self, network: str = "wifi", rotation: int = 90, scale: int = 3) -> str:
        return f'''network = "{network}"
wifi_ssid = "test-network"
wifi_password = "test-password"
signaling_url = "http://192.0.2.2:8080/offer"
run_ms = 120000
display_rotation = {rotation}
display_scale = {scale}
'''

    def test_wifi_profile_and_generated_config(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = self.write_profile(Path(temporary), self.profile())
            settings = rt1170_demo.load_settings(path)
            config = rt1170_demo.render_kconfig(settings)

        self.assertEqual(settings.network, "wifi")
        self.assertEqual(settings.output_dimensions, (720, 960))
        self.assertIn('CONFIG_ZEPHYRTC_WIFI_SSID="test-network"', config)
        self.assertIn('CONFIG_ZEPHYRTC_WIFI_PASSWORD="test-password"', config)
        self.assertIn("CONFIG_ZEPHYRTC_DISPLAY_ROTATION=90", config)
        self.assertIn("CONFIG_ZEPHYRTC_DISPLAY_SCALE=3", config)

    def test_ethernet_omits_credentials(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = self.write_profile(Path(temporary), self.profile("ethernet", 0, 2))
            settings = rt1170_demo.load_settings(path)
            config = rt1170_demo.render_kconfig(settings)

        self.assertEqual(settings.output_dimensions, (640, 480))
        self.assertNotIn("WIFI_SSID", config)
        self.assertNotIn("WIFI_PASSWORD", config)

    def test_rejects_insecure_mode(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = self.write_profile(Path(temporary), self.profile(), 0o644)
            with self.assertRaises(rt1170_demo.SettingsError):
                rt1170_demo.load_settings(path)

    def test_rejects_invalid_rotation_and_geometry(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            invalid_rotation = self.write_profile(root, self.profile(rotation=45))
            with self.assertRaises(rt1170_demo.SettingsError):
                rt1170_demo.load_settings(invalid_rotation)
            invalid_rotation.unlink()
            invalid_geometry = self.write_profile(root, self.profile(rotation=0, scale=3))
            with self.assertRaises(rt1170_demo.SettingsError):
                rt1170_demo.load_settings(invalid_geometry)

    def test_generated_config_is_owner_only(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "generated.conf"
            rt1170_demo.write_generated_config(path, "CONFIG_TEST=y\n")
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)

    def test_build_command_omits_wifi_files_for_ethernet(self) -> None:
        command = rt1170_demo.build_command(
            Path("/app"), Path("/build"), Path("/private/generated.conf"), "ethernet"
        )
        joined = " ".join(command)
        self.assertNotIn("overlay-mimxrt1170-wifi.conf", joined)
        self.assertNotIn("_wifi.overlay", joined)

    def test_build_command_selects_wifi_files(self) -> None:
        command = rt1170_demo.build_command(
            Path("/app"), Path("/build"), Path("/private/generated.conf"), "wifi"
        )
        joined = " ".join(command)
        self.assertIn("overlay-mimxrt1170-wifi.conf", joined)
        self.assertIn("_wifi.overlay", joined)
        self.assertNotIn("test-network", joined)
        self.assertNotIn("test-password", joined)


if __name__ == "__main__":
    unittest.main()
