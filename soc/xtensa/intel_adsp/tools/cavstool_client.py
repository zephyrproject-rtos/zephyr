#!/usr/bin/env python3
# Copyright(c) 2022 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
import os
import logging
import time
import argparse
import socket
import signal

HOST = None
PORT_LOG = 9999
PORT_REQ = PORT_LOG + 1
BUF_SIZE = 4096

CMD_LOG_START = "start_log"
CMD_LOG_STOP = "stop_log"
CMD_DOWNLOAD = "download"

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
            ack = str(self.sock.recv(BUF_SIZE), "utf-8")
            log.info(f"Receive: {ack}")

            if ack == CMD_LOG_START:
                self.monitor_log()
            elif ack == CMD_LOG_STOP:
                log.info(f"Stop output.")
            elif ack == CMD_DOWNLOAD:
                self.run()
            else:
                log.error(f"Receive incorrect msg:{ack} expect:{cmd}")

    def download(self, filename):
        # Send the FW to server
        with open(filename,'rb') as f:
            log.info('Sending...')
            ret = self.sock.sendfile(f)
            log.info(f"Done Sending ({ret}).")

    def run(self):
        filename = str(self.args.fw_file)
        send_fn = os.path.basename(filename)
        self.sock.sendall(send_fn.encode("utf-8"))
        log.info(f"Sent fw:     {send_fn}, {filename}")

        self.download(filename)

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

def cleanup():
    client = cavstool_client(HOST, PORT_REQ, args)
    client.send_cmd(CMD_LOG_STOP)

def main():
    if args.log_only:
        log.info("Monitor process")
        signal.signal(signal.SIGTERM, cleanup)

        try:
            client = cavstool_client(HOST, PORT_LOG, args)
            client.send_cmd(CMD_LOG_START)
        except KeyboardInterrupt:
            pass
        finally:
            cleanup()

    elif args.kill_cmd:
        log.info("Stop monitor log")
        cleanup()
    else:
        log.info("Download process")
        client = cavstool_client(HOST, PORT_REQ, args)
        client.send_cmd(CMD_DOWNLOAD)

ap = argparse.ArgumentParser(description="DSP loader/logger client tool")
ap.add_argument("-q", "--quiet", action="store_true",
                help="No loader output, just DSP logging")
ap.add_argument("-l", "--log-only", action="store_true",
                help="Don't load firmware, just show log output")
ap.add_argument("-s", "--server-addr", default="localhost",
                help="Specify the adsp server address")
ap.add_argument("-k", "--kill-cmd", action="store_true",
                help="No current log buffer at start, just new output")
ap.add_argument("fw_file", nargs="?", help="Firmware file")
args = ap.parse_args()

if args.quiet:
    log.setLevel(logging.WARN)

HOST = args.server_addr

if __name__ == "__main__":
    main()
