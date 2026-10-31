# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

"""Run complete nrf52_bsim images, built by ci.bt.bsworld.sh, under BSWorld."""

import os
from pathlib import Path

import pytest
from bsworld.devices import ProcessInstance
from bsworld.topology import BluetoothTopology


def zephyr_image(name: str) -> Path:
    """Return the zephyr.exe built for name under BSWORLD_ZEPHYR_BUILD_DIR.

    Skip when the variable is unset, so tests run only where images exist.
    Fail when an image is missing, so a broken build cannot pass as skipped.
    """
    build_dir = os.environ.get("BSWORLD_ZEPHYR_BUILD_DIR")
    if not build_dir:
        pytest.skip("Set BSWORLD_ZEPHYR_BUILD_DIR; see README.rst")
    binary = Path(build_dir, name, "zephyr", "zephyr.exe").resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        pytest.fail(f"Image {name} was not built: {binary}")
    return binary


def add_image(
    topo: BluetoothTopology, name: str, *, label: str, seed: int, shell: bool = False
) -> ProcessInstance:
    """Register an image that joins the PHY with its own Controller and radio."""
    binary = zephyr_image(name)
    library_path = os.pathsep.join(
        filter(None, [str(topo.env.bsim_dir / "lib"), os.environ.get("LD_LIBRARY_PATH")])
    )
    return topo.add_process(
        ProcessInstance(
            command=lambda slot: [
                binary,
                f"-s={slot.sim_id}",
                f"-p={slot.phy_id}",
                f"-d={slot.dev_id}",
                f"-rs={seed}",
                "-mro=1000",
                "-RealEncryption=1",
            ],
            label=label,
            shell=shell,
            # -RealEncryption=1 loads libCryptov1.so from BabbleSim's lib/.
            env={"LD_LIBRARY_PATH": library_path},
        )
    )
