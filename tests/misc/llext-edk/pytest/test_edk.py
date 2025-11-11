# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import shutil
import tempfile
from pathlib import Path
from subprocess import check_output

import pytest
import yaml
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_edk(unlaunched_dut: DeviceAdapter):
    # Get the SDK path from build_info.yml
    build_dir = str(unlaunched_dut.device_config.build_dir)
    with open(Path(build_dir) / "build_info.yml") as f:
        build_info = yaml.safe_load(f)
        if build_info["cmake"]["toolchain"]["name"] != "zephyr":
            logger.warning("This test requires the Zephyr SDK to be used, skipping")
            pytest.skip("The Zephyr SDK must be used")

        sdk_dir = build_info["cmake"]["toolchain"]["path"]

    # Can we build the edk?
    command = [
        "west",
        "build",
        "-b",
        unlaunched_dut.device_config.platform,
        "-t",
        "llext-edk",
        "--build-dir",
        unlaunched_dut.device_config.build_dir,
    ]
    output = check_output(command, text=True)
    logger.info(output)

    # Install the edk to a temporary location
    with tempfile.TemporaryDirectory() as tempdir:
        # Copy the edk to the temporary directory using python methods
        logger.debug(f"Copying llext-edk.tar.xz to {tempdir}")
        edk_path = Path(unlaunched_dut.device_config.build_dir) / "zephyr/llext-edk.tar.xz"
        shutil.copy(edk_path, tempdir)

        # Extract the edk using tar
        logger.debug(f"Extracting llext-edk.tar.xz to {tempdir}")
        command = ["tar", "-xf", "llext-edk.tar.xz"]
        output = check_output(command, text=True, cwd=tempdir)
        logger.info(output)

        # Copy the extension to another temporary directory to test out of tree builds
        with tempfile.TemporaryDirectory() as tempdir_extension:
            logger.debug(f"Copying extension to {tempdir_extension}")
            ext_dir = Path(os.environ["ZEPHYR_BASE"]) / "tests/misc/llext-edk/extension"
            shutil.copytree(ext_dir, tempdir_extension, dirs_exist_ok=True)

            # Also copy file2hex.py to the extension directory, so that it's possible
            # to generate a hex file from the extension binary
            logger.debug(f"Copying file2hex.py to {tempdir_extension}")
            file2hex = Path(os.environ["ZEPHYR_BASE"]) / "scripts/build/file2hex.py"
            shutil.copy(file2hex, tempdir_extension)

            # Set the LLEXT_EDK_INSTALL_DIR environment variable so that the extension
            # knows where the EDK is installed
            edk_dir = Path(tempdir) / "llext-edk"
            env = os.environ.copy()
            env.update({"ZEPHYR_SDK_INSTALL_DIR": sdk_dir})
            env.update({"LLEXT_EDK_INSTALL_DIR": edk_dir})

            # Build the extension using the edk
            logger.debug(f"Building extension in {tempdir_extension} - cmake")
            command = ["cmake", "-B", "build"]
            output = check_output(command, text=True, cwd=tempdir_extension, env=env)
            logger.info(output)

            logger.debug(f"Building extension in {tempdir_extension} - make")
            command = ["make", "-C", "build"]
            output = check_output(command, text=True, cwd=tempdir_extension, env=env)
            logger.info(output)

            # Check if the extension was built
            assert os.path.exists(Path(tempdir_extension) / "build/extension.llext")

            # Can we run it? First, rebuild the application, now including the extension
            # build directory in the include path, so that the application can find the
            # extension code.
            logger.debug(f"Running application with extension in {tempdir_extension} - west build")
            command = [
                "west",
                "build",
                "-b",
                unlaunched_dut.device_config.platform,
                "--build-dir",
                unlaunched_dut.device_config.build_dir,
                "--",
                f"-DEXTENSION_DIR={tempdir_extension}/build/",
            ]
            logger.debug(f"west command: {command}")
            output = check_output(command, text=True)
            logger.info(output)

            # Now that the application is built, run it
            logger.debug(f"Running application with extension in {tempdir_extension}")
            try:
                unlaunched_dut.launch()
                lines = unlaunched_dut.readlines_until("Done")

                assert "Calling extension from kernel" in lines
                assert "Calling extension from user" in lines
                assert "foo(42) is 1764" in lines
                assert "foo(43) is 1849" in lines

            finally:
                unlaunched_dut.close()
