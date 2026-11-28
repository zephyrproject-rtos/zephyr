#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Build the zephyrtc RT1170 image from one credential-safe settings file."""

from __future__ import annotations

import argparse
import contextlib
import dataclasses
import os
import stat
import subprocess
import tempfile
import tomllib
from pathlib import Path
from typing import Any

BOARD = "mimxrt1170_evk@B/mimxrt1176/cm7"
SHIELD = "nxp_btb44_ov5640;rk055hdmipi4ma0"
PANEL_WIDTH = 720
PANEL_HEIGHT = 1280
VIDEO_WIDTH = 320
VIDEO_HEIGHT = 240
VALID_ROTATIONS = {0, 90, 180, 270}
MAX_SCALE = 3
ALLOWED_KEYS = {
    "network",
    "wifi_ssid",
    "wifi_password",
    "signaling_url",
    "run_ms",
    "display_rotation",
    "display_scale",
}


class SettingsError(ValueError):
    """The private startup profile is invalid."""


@dataclasses.dataclass(frozen=True)
class Settings:
    network: str
    wifi_ssid: str
    wifi_password: str
    signaling_url: str
    run_ms: int
    display_rotation: int
    display_scale: int

    @property
    def output_dimensions(self) -> tuple[int, int]:
        if self.display_rotation in (90, 270):
            width, height = VIDEO_HEIGHT, VIDEO_WIDTH
        else:
            width, height = VIDEO_WIDTH, VIDEO_HEIGHT
        return width * self.display_scale, height * self.display_scale


def _require(data: dict[str, Any], key: str, expected_type: type) -> Any:
    value = data.get(key)
    if not isinstance(value, expected_type) or isinstance(value, bool):
        raise SettingsError(f"{key} must be {expected_type.__name__}")
    return value


def load_settings(path: Path) -> Settings:
    mode = stat.S_IMODE(path.stat().st_mode)
    if mode & 0o077:
        raise SettingsError(f"{path} must not be accessible by group or other")
    with path.open("rb") as source:
        data = tomllib.load(source)
    unknown = sorted(set(data) - ALLOWED_KEYS)
    if unknown:
        raise SettingsError(f"unknown settings: {', '.join(unknown)}")

    network = _require(data, "network", str)
    if network not in ("ethernet", "wifi"):
        raise SettingsError("network must be 'ethernet' or 'wifi'")
    ssid = data.get("wifi_ssid", "")
    password = data.get("wifi_password", "")
    if not isinstance(ssid, str) or not isinstance(password, str):
        raise SettingsError("wifi_ssid and wifi_password must be strings")
    if any(character in ssid + password for character in "\r\n"):
        raise SettingsError("Wi-Fi credentials must not contain newlines")
    ssid_bytes = len(ssid.encode("utf-8"))
    password_bytes = len(password.encode("utf-8"))
    if network == "wifi" and not 1 <= ssid_bytes <= 32:
        raise SettingsError("wifi_ssid must contain 1 to 32 UTF-8 bytes")
    if network == "wifi" and password_bytes != 0 and not 8 <= password_bytes <= 64:
        raise SettingsError("wifi_password must be empty or contain 8 to 64 UTF-8 bytes")

    settings = Settings(
        network=network,
        wifi_ssid=ssid,
        wifi_password=password,
        signaling_url=_require(data, "signaling_url", str),
        run_ms=_require(data, "run_ms", int),
        display_rotation=_require(data, "display_rotation", int),
        display_scale=_require(data, "display_scale", int),
    )
    if not settings.signaling_url.startswith("http://"):
        raise SettingsError("signaling_url must begin with http://")
    if any(character in settings.signaling_url for character in "\r\n"):
        raise SettingsError("signaling_url must not contain newlines")
    if not 5_000 <= settings.run_ms <= 86_400_000:
        raise SettingsError("run_ms must be between 5000 and 86400000")
    if settings.display_rotation not in VALID_ROTATIONS:
        raise SettingsError("display_rotation must be 0, 90, 180, or 270")
    if not 1 <= settings.display_scale <= MAX_SCALE:
        raise SettingsError("display_scale must be between 1 and 3")
    width, height = settings.output_dimensions
    if width > PANEL_WIDTH or height > PANEL_HEIGHT:
        raise SettingsError(
            f"transformed video {width}x{height} does not fit {PANEL_WIDTH}x{PANEL_HEIGHT}"
        )
    return settings


def _kconfig_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def render_kconfig(settings: Settings) -> str:
    lines = [
        "# Generated from the private zephyrtc settings file. Do not commit it.",
        f"CONFIG_ZEPHYRTC_SIGNALING_URL={_kconfig_string(settings.signaling_url)}",
        f"CONFIG_ZEPHYRTC_EXTERNAL_RUN_MS={settings.run_ms}",
        f"CONFIG_ZEPHYRTC_DISPLAY_ROTATION={settings.display_rotation}",
        f"CONFIG_ZEPHYRTC_DISPLAY_SCALE={settings.display_scale}",
        "CONFIG_INIT_STACKS=n",
        "CONFIG_SYS_HEAP_RUNTIME_STATS=y",
    ]
    if settings.network == "wifi":
        lines.extend(
            [
                f"CONFIG_ZEPHYRTC_WIFI_SSID={_kconfig_string(settings.wifi_ssid)}",
                f"CONFIG_ZEPHYRTC_WIFI_PASSWORD={_kconfig_string(settings.wifi_password)}",
            ]
        )
    return "\n".join(lines) + "\n"


def write_generated_config(path: Path, contents: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", text=True
    )
    try:
        os.fchmod(descriptor, 0o600)
        with os.fdopen(descriptor, "w", encoding="utf-8") as output:
            output.write(contents)
        os.replace(temporary_name, path)
    except BaseException:
        with contextlib.suppress(OSError):
            os.close(descriptor)
        Path(temporary_name).unlink(missing_ok=True)
        raise


def build_command(
    app_dir: Path, build_dir: Path, generated_config: Path, network: str
) -> list[str]:
    config_files = [app_dir / "overlay-mimxrt1170-av.conf"]
    overlays = [
        app_dir / "boards/mimxrt1170_evk_mimxrt1176_cm7_B.overlay",
    ]
    if network == "wifi":
        config_files.append(app_dir / "overlay-mimxrt1170-wifi.conf")
        overlays.append(app_dir / "boards/mimxrt1170_evk_mimxrt1176_cm7_B_wifi.overlay")
    config_files.append(generated_config)
    return [
        "west",
        "build",
        "-p",
        "always",
        "-b",
        BOARD,
        "--shield",
        SHIELD,
        "-d",
        str(build_dir),
        str(app_dir),
        "--",
        "-DEXTRA_CONF_FILE=" + ";".join(map(str, config_files)),
        "-DDTC_OVERLAY_FILE=" + ";".join(map(str, overlays)),
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--settings",
        type=Path,
        default=Path.home() / ".config/zephyrtc/rt1170.toml",
        help="mode-0600 private TOML profile",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="build directory; defaults to build/zephyrtc-NETWORK in the west workspace",
    )
    args = parser.parse_args()

    settings_path = args.settings.expanduser().resolve()
    settings = load_settings(settings_path)
    app_dir = Path(__file__).resolve().parents[1]
    zephyr_base = Path(os.environ.get("ZEPHYR_BASE", app_dir.parents[3])).resolve()
    build_dir = (
        args.build_dir.expanduser().resolve()
        if args.build_dir
        else zephyr_base.parent / "build" / f"zephyrtc-{settings.network}"
    )
    generated_config = settings_path.with_name(settings_path.stem + ".generated.conf")
    write_generated_config(generated_config, render_kconfig(settings))
    width, height = settings.output_dimensions
    print(
        f"Building network={settings.network} display={width}x{height} "
        f"rotation={settings.display_rotation} scale={settings.display_scale}",
        flush=True,
    )
    subprocess.run(
        build_command(app_dir, build_dir, generated_config, settings.network),
        cwd=zephyr_base,
        check=True,
    )
    print(f"Build complete: {build_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
