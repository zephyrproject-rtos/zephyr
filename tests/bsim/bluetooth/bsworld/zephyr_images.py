# Copyright (c) 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

"""Run complete nrf52_bsim images, built by ci.bt.bsworld.sh, under BSWorld."""

import os
from pathlib import Path

import pytest
from bsworld.devices import ProcessInstance
from bsworld.topology import BluetoothTopology


def zephyr_image(scenario: str) -> Path:
    """Return the zephyr.exe twister built for scenario in BSWORLD_ZEPHYR_BUILD_DIR.

    Skip when the variable is unset, so tests run only where images exist.
    Fail when an image is missing, so a broken build cannot pass as skipped.
    """
    build_dir = os.environ.get("BSWORLD_ZEPHYR_BUILD_DIR")
    if not build_dir:
        pytest.skip("Set BSWORLD_ZEPHYR_BUILD_DIR; see README.rst")
    binaries = list(Path(build_dir).glob(f"**/{scenario}/zephyr/zephyr.exe"))
    if len(binaries) != 1 or not os.access(binaries[0], os.X_OK):
        pytest.fail(f"Scenario {scenario} was not built once in {build_dir}: {binaries}")
    return binaries[0]


def add_image(
    topo: BluetoothTopology, scenario: str, *, label: str, seed: int, shell: bool = False
) -> ProcessInstance:
    """Register an image that joins the PHY with its own Controller and radio."""
    binary = zephyr_image(scenario)
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
