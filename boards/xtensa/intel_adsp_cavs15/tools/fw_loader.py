#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import os
import argparse
import logging

from colorama import Fore, Style
from lib.loader import FirmwareLoader, FirmwareStatus
import lib.platforms as plat_def


def check_args(args):
    # Check if firmware exists
    firmware_abs = os.path.abspath(args.firmware)
    if not os.path.exists(firmware_abs):
        raise ValueError("File not found: %s" % firmware_abs)


def parse_args():
    arg_parser = argparse.ArgumentParser(description="ADSP firmware loader")
    arg_parser.add_argument("-f", "--firmware", required=True,
                            help="ADSP firmware file to load")
    arg_parser.add_argument("-d", "--debug", default=False, action='store_true',
                            help="Display debug information")
    args = arg_parser.parse_args()
    check_args(args)
    return args


def main():
    """ Main Entry Point """

    args = parse_args()

    log_level = logging.INFO
    if args.debug:
        log_level = logging.DEBUG
    logging.basicConfig(level=log_level, format="%(message)s")

    logging.info("Start firmware downloading...")
    fw_loader = FirmwareLoader()

    # Use Stream #7 for firmware download DMA
    fw_loader.init(plat_def.DMA_ID)

    logging.info("Reset DSP...")
    fw_loader.reset_dsp()
    FirmwareStatus(fw_loader.dev.fw_status.value).print()
    logging.info("   IPC CMD  : %s" % fw_loader.dev.ipc_cmd)
    logging.info("   IPC LEN  : %s" % fw_loader.dev.ipc_len)

    logging.info("Booting up DSP...")
    fw_loader.boot_dsp()
    FirmwareStatus(fw_loader.dev.fw_status.value).print()

    fw_loader.wait_for_fw_boot_status(plat_def.BOOT_STATUS_INIT_DONE)

    logging.info("Downloading firmware...")
    fw_loader.download_firmware(args.firmware)

    logging.info("Checking firmware status...")
    if fw_loader.check_fw_boot_status(plat_def.BOOT_STATUS_FW_ENTERED):
        logging.info(Fore.LIGHTGREEN_EX +
                     "Firmware download completed successfully"
                     + Style.RESET_ALL)
        logging.info("Reading IPC FwReady Message...")
        fw_loader.ipc.read_fw_ready()
    else:
        logging.error(Fore.RED +
                      "!!!!! Failed to download firmware !!!!!"
                      + Style.RESET_ALL)
    fw_loader.close()


if __name__ == "__main__":
    try:
        main()
        os._exit(0)
    except KeyboardInterrupt:  # Ctrl-C
        os._exit(14)
    except SystemExit:
        raise
    except BaseException:
        import traceback
        traceback.print_exc()
        os._exit(16)
