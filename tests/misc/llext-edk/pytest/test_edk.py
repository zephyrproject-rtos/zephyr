# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import shutil
import tempfile

from pathlib import Path
from subprocess import check_output
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

def test_edk(dut: DeviceAdapter):
    # Can we build the edk?
    command = [
        "west",
        "build",
        "-t",
        "llext-edk",
        "--build-dir",
        dut.device_config.build_dir,
    ]
    output = check_output(command, text=True)
    logger.info(output)

    # Install the edk to a temporary location
    with tempfile.TemporaryDirectory() as tempdir:
        # Copy the edk to the temporary directory using python methods
        logger.debug(f"Copying llext-edk.tar.xz to {tempdir}")
        edk_path = Path(dut.device_config.build_dir) / "zephyr/llext-edk.tar.xz"
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

            # Set the LLEXT_EDK_INSTALL_DIR environment variable so that the extension
            # knows where the EDK is installed
            edk_dir = Path(tempdir) / "llext-edk"
            env = os.environ.copy()
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
