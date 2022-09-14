#!/usr/bin/env python3
#
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''
This script allows flashing a mec172xevb_assy6906 board
attached to a remote system.

Usage:
  west flash -r misc-flasher -- mec172x_remote_flasher.py <remote host>

Note:
1. SSH access to remote host with write access to remote /tmp.
   Since the script does multiple SSH connections, it is a good idea
   to setup public key authentication and ssh-agent.
2. Dediprog "dpcmd" available in path on remote host.
   (Can be compiled from https://github.com/DediProgSW/SF100Linux)
3. SSH user must have permission to access USB devices,
   since dpcmd needs USB access to communicate with
   the Dediprog programmer attached to remote host.

To use with twister, a hardware map file is needed.
Here is a sample map file:

  - connected: true
    available: true
    id: mec172xevb_assy6906
    platform: mec172xevb_assy6906
    product: mec172xevb_assy6906
    runner: misc-flasher
    runner_params:
      - <ZEPHYR_BASE>/boards/microchip/mec172xevb_assy6906/support/mec172x_remote_flasher.py
      - <remote host>
    serial_pty: "nc,<remote host>,<ser2net port>"

The sample map file assumes the serial console is exposed via ser2net,
and that it can be accessed using nc (netcat).

To use twister:
  ./scripts/twister --hardware-map <hw map file> --device-testing

Required:
* Fabric (https://www.fabfile.org/)
'''

import argparse
import hashlib
import pathlib
import sys

from datetime import datetime

import fabric
from invoke.exceptions import UnexpectedExit

def calc_sha256(spi_file):
    '''
    Calculate a SHA256 of the SPI binary content plus current
    date string.

    This is used for remote file name to avoid file name
    collision.
    '''
    sha256 = hashlib.sha256()

    # Use SPI file content to calculate SHA.
    with open(spi_file, "rb") as fbin:
        spi_data = fbin.read()
        sha256.update(spi_data)

    # Add a date/time to SHA to hopefully
    # further avoid file name collision.
    now = datetime.now().isoformat()
    sha256.update(now.encode("utf-8"))

    return sha256.hexdigest()

def parse_args():
    '''
    Parse command line arguments.
    '''
    parser = argparse.ArgumentParser(allow_abbrev=False)

    # Fixed arguments
    parser.add_argument("build_dir",
                        help="Build directory")
    parser.add_argument("remote_host",
                        help="Remote host name or IP address")

    # Arguments about remote machine
    remote = parser.add_argument_group("Remote Machine")
    remote.add_argument("--remote-tmp", required=False,
                        help="Remote temporary directory to store SPI binary "
                             "[default=/tmp for Linux remote]")
    remote.add_argument("--dpcmd", required=False, default="dpcmd",
                        help="Full path to dpcmd on remote machine")

    # Remote machine type.
    # This affects how remote path is constructed.
    remote_type = remote.add_mutually_exclusive_group()
    remote_type.add_argument("--remote-is-linux", required=False,
                             default=True, action="store_true",
                             help="Set if remote machine is a Linux-like machine [default]")
    remote_type.add_argument("--remote-is-win", required=False,
                             action="store_true",
                             help="Set if remote machine is a Windows machine")

    return parser.parse_args()

def main():
    '''
    Main
    '''
    args = parse_args()

    # Check for valid arguments and setup variables.
    if not args.remote_tmp:
        if args.remote_is_win:
            # Do not assume a default temporary on Windows,
            # as it is usually under user's directory and
            # we do not know enough to construct a valid path
            # at this time.
            print("[ERROR] --remote-tmp is required for --remote-is-win")
            sys.exit(1)

        if args.remote_is_linux:
            remote_tmp = pathlib.PurePosixPath("/tmp")
    else:
        if args.remote_is_win:
            remote_tmp = pathlib.PureWindowsPath(args.remote_tmp)
        elif args.remote_is_linux:
            remote_tmp = pathlib.PurePosixPath(args.remote_tmp)

    # Construct full path to SPI binary.
    spi_file_path = pathlib.Path(args.build_dir)
    spi_file_path = spi_file_path.joinpath("zephyr", "spi_image.bin")

    # Calculate a sha256 digest for SPI file.
    # This is used for remote file to avoid file name collision
    # if there are multiple MEC17x attached to remote machine
    # and all are trying to flash at same time.
    sha256 = calc_sha256(spi_file_path)

    # Construct full path on remote to store
    # the transferred SPI binary.
    remote_file_name = remote_tmp.joinpath(f"mec172x_{sha256}.bin")

    print(f"[INFO] Build directory: {args.build_dir}")
    print(f"[INFO] Remote host: {args.remote_host}")

    # Connect to remote host via SSH.
    ssh = fabric.Connection(args.remote_host, forward_agent=True)

    print("[INFO] Sending file...")
    print(f"[INFO]   Local SPI file: {spi_file_path}")
    print(f"[INFO]   Remote SPI file: {remote_file_name}")

    # Open SFTP channel, and send the SPI binary over.
    sftp = ssh.sftp()
    sftp.put(str(spi_file_path), str(remote_file_name))

    # Run dpcmd to flash the device.
    try:
        dpcmd_cmd = f"{args.dpcmd} --auto {str(remote_file_name)} --verify"
        print(f"[INFO] Invoking: {dpcmd_cmd}...")
        ssh.run(dpcmd_cmd)
    except UnexpectedExit:
        print("[ERR ] Cannot flashing SPI binary!")

    # Remove temporary file.
    print(f"[INFO] Removing remote file {remote_file_name}")
    sftp.remove(str(remote_file_name))

    sftp.close()
    ssh.close()

if __name__ == "__main__":
    main()
