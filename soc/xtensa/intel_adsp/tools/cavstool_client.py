#!/usr/bin/env python3
# Copyright(c) 2022 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
import os
import sys
import logging
import time
import argparse
import socket
import struct
import hashlib

RET = 0
HOST = None
PORT_LOG = 9999
PORT_REQ = PORT_LOG + 1
BUF_SIZE = 4096

# Define the command and its
# possible max size
CMD_LOG_START = "start_log"
CMD_DOWNLOAD = "download"
MAX_CMD_SZ = 16

# Define the header format and size for
# transmiting the firmware
PACKET_HEADER_FORMAT_FW = 'I 42s 32s'

logging.basicConfig()
log = logging.getLogger("cavs-client")
log.setLevel(logging.INFO)

class cavstool_client():
    def __init__(self, host, port, args):
        self.host = host
        self.port = port
        self.args = args
        self.sock = None
        self.cmd = None

    def send_cmd(self, cmd):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            self.sock = sock
            self.cmd = cmd
            self.sock.connect((self.host, self.port))
            self.sock.sendall(cmd.encode("utf-8"))
            log.info(f"Sent:     {cmd}")
            ack = str(self.sock.recv(MAX_CMD_SZ), "utf-8")
            log.info(f"Receive: {ack}")

            if ack == CMD_LOG_START:
                self.monitor_log()
            elif ack == CMD_DOWNLOAD:
                self.run()
            else:
                log.error(f"Receive incorrect msg:{ack} expect:{cmd}")

    def uploading(self, filename):
        # Send the FW to server
        fname = os.path.basename(filename)
        fsize = os.path.getsize(filename)

        md5_tx = hashlib.md5(open(filename,'rb').read()).hexdigest()

        # Pack the header and the expecting packed size is 78 bytes.
        # The header by convention includes:
        # size(4), filename(42), MD5(32)
        values = (fsize, fname.encode('utf-8'), md5_tx.encode('utf-8'))
        log.info(f'filename:{fname}, size:{fsize}, md5:{md5_tx}')

        s = struct.Struct(PACKET_HEADER_FORMAT_FW)
        header_data = s.pack(*values)
        header_size = s.size
        log.info(f'header size: {header_size}')

        with open(filename,'rb') as f:
            log.info(f'Sending...')

            total = self.sock.send(header_data)
            total += self.sock.sendfile(f)

            log.info(f"Done Sending ({total}).")

            rck = self.sock.recv(MAX_CMD_SZ).decode("utf-8")
            log.info(f"RCK ({rck}).")
            if not rck == "success":
                global RET
                RET = -1
                log.error(f"Firmware uploading failed")

    def run(self):
        filename = str(self.args.fw_file)
        self.uploading(filename)

    def monitor_log(self):
        log.info(f"Start to monitor log output...")
        while True:
            # Receive data from the server and print out
            receive_log = str(self.sock.recv(BUF_SIZE).strip(), "utf-8")
            if receive_log:
                print(f"{receive_log}")
                time.sleep(0.1)

    def __del__(self):
        self.sock.close()


def main():
    if args.log_only:
        log.info("Monitor process")

        try:
            client = cavstool_client(HOST, PORT_LOG, args)
            client.send_cmd(CMD_LOG_START)
        except KeyboardInterrupt:
            pass

    else:
        log.info("Uploading process")
        client = cavstool_client(HOST, PORT_REQ, args)
        client.send_cmd(CMD_DOWNLOAD)

ap = argparse.ArgumentParser(description="DSP loader/logger client tool")
ap.add_argument("-q", "--quiet", action="store_true",
                help="No loader output, just DSP logging")
ap.add_argument("-l", "--log-only", action="store_true",
                help="Don't load firmware, just show log output")
ap.add_argument("-s", "--server-addr", default="localhost",
                help="Specify the adsp server address")
ap.add_argument("fw_file", nargs="?", help="Firmware file")
args = ap.parse_args()

if args.quiet:
    log.setLevel(logging.WARN)

HOST = args.server_addr

if __name__ == "__main__":
    main()

    sys.exit(RET)
