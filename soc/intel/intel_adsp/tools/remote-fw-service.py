#!/usr/bin/env python3
# Copyright(c) 2022 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
import os
import sys
import struct
import logging
import time
import subprocess
import argparse
import socketserver
import threading
import hashlib
import queue
from urllib.parse import urlparse

# Global variable use to sync between log and request services.
runner = None

# pylint: disable=duplicate-code

# INADDR_ANY as default
HOST = ''
PORT_LOG = 9999
PORT_REQ = PORT_LOG + 1
BUF_SIZE = 4096

# Define the command and the max size
CMD_LOG_START = "start_log"
CMD_DOWNLOAD = "download"
MAX_CMD_SZ = 16

# Define the return value in handle function
ERR_FAIL = 1

# Define the header format and size for
# transmiting the firmware
PACKET_HEADER_FORMAT_FW = 'I 42s 32s'
HEADER_SZ = 78

logging.basicConfig(level=logging.INFO)
log = logging.getLogger("remote-fw")


class adsp_request_handler(socketserver.BaseRequestHandler):
    """
    The request handler class for control the actions of server.
    """

    def receive_fw(self):
        log.info("Receiving...")
        # Receive the header first
        d = self.request.recv(HEADER_SZ)

        # Unpacked the header data
        # Include size(4), filename(42) and MD5(32)
        header = d[:HEADER_SZ]
        total = d[HEADER_SZ:]
        s = struct.Struct(PACKET_HEADER_FORMAT_FW)
        fsize, fname, md5_tx_b = s.unpack(header)
        log.info(f'size:{fsize}, filename:{fname}, MD5:{md5_tx_b}')

        # Receive the firmware. We only receive the specified amount of bytes.
        while len(total) < fsize:
            data = self.request.recv(min(BUF_SIZE, fsize - len(total)))
            if not data:
                raise EOFError("truncated firmware file")
            total += data

        log.info(f"Done Receiving {len(total)}.")

        try:
            with open(fname,'wb') as f:
                f.write(total)
        except Exception as e:
            log.error(f"Get exception {e} during FW transfer.")
            return None

        # Check the MD5 of the firmware
        md5_rx = hashlib.md5(total, usedforsecurity=False).hexdigest()
        md5_tx = md5_tx_b.decode('utf-8')

        if md5_tx != md5_rx:
            log.error(f'MD5 mismatch: {md5_tx} vs. {md5_rx}')
            return None

        return fname

    def do_download(self):
        recv_file = self.receive_fw()

        if recv_file:
            recv_file = recv_file.decode('utf-8')

            if os.path.exists(recv_file):
                runner.set_fw_ready(recv_file)
                return 0

        log.error("Cannot find the FW file.")
        return ERR_FAIL

    def handle(self):
        cmd = self.request.recv(MAX_CMD_SZ)
        log.info(f"{self.client_address[0]} wrote: {cmd}")
        action = cmd.decode("utf-8")
        log.debug(f'load {action}')
        ret = ERR_FAIL

        if action == CMD_DOWNLOAD:
            self.request.sendall(cmd)
            ret = self.do_download()
        else:
            log.error("incorrect load communitcation!")
            return

        if not ret:
            self.request.sendall("success".encode('utf-8'))
            log.info("Firmware well received. Ready to download.")
        else:
            self.request.sendall("failed".encode('utf-8'))
            log.error("Receive firmware failed.")

class adsp_log_handler(socketserver.BaseRequestHandler):
    """
    The log handler class for grabbing output messages of server.
    """

    def handle(self):
        cmd = self.request.recv(MAX_CMD_SZ)
        log.info(f"{self.client_address[0]} wrote: {cmd}")
        action = cmd.decode("utf-8")
        log.debug(f'monitor {action}')

        if action == CMD_LOG_START:
            self.request.sendall(cmd)
        else:
            log.error("incorrect monitor communitcation!")

        log.info("wait for FW ready...")
        while not runner.is_fw_ready():
            if not self.is_connection_alive():
                return

            time.sleep(1)

        log.info("FW is ready...")

        # start_new_session=True in order to get a different Process Group
        # ID. When the PGID is the same, sudo does NOT propagate signals out of
        # fear of "accidentally killing itself" (man sudo).
        # Compare:
        #
        # - Different PGID: signal is propagated and sleep is terminated
        #
        #    sudo sleep 15 & kill $!
        #
        # - Same PGID, sleep is NOT terminated
        #
        #    sudo bash -c 'sleep 15 & killall sudo'
        #
        #    ps  xfao pid,ppid,pgid,sid,comm | grep -C 5 -e PID -e sleep -e sudo

        with subprocess.Popen(runner.get_script(), stdout=subprocess.PIPE,
                              start_new_session=True) as proc:
            # Thread for monitoring the conntection
            t = threading.Thread(target=self.check_connection, args=(proc,))
            t.start()

            while True:
                try:
                    out = proc.stdout.readline()
                    self.request.sendall(out)
                    ret = proc.poll()
                    if ret:
                        log.info(f"retrun code: {ret}")
                        break

                except (BrokenPipeError, ConnectionResetError):
                    log.info("Client is disconnect.")
                    break

            t.join()

        log.info("service complete.")

    def finish(self):
        runner.cleanup()
        log.info("Wait for next service...")

    def is_connection_alive(self):
        try:
            self.request.sendall(b'\x00')
        except (BrokenPipeError, ConnectionResetError):
            log.info("Client is disconnect.")
            return False

        return True

    def check_connection(self, proc):
        # Not to check connection alive for
        # the first 10 secs.
        time.sleep(10)

        poll_interval = 1
        log.info("Now checking client connection every %ds", poll_interval)
        while True:
            if not self.is_connection_alive():
                # cavstool
                child_desc = " ".join(runner.script) + f", PID={proc.pid}"
                log.info("Terminating %s", child_desc)

                try:
                    # sudo does _not_ propagate SIGKILL (man sudo)
                    proc.terminate()
                    try:
                        proc.wait(timeout=0.5)
                    except subprocess.TimeoutExpired:
                        log.error("SIGTERM failed on child %s", child_desc)
                        if os.geteuid() == 0: # sudo not needed and not used
                            log.error("Sending %d SIGKILL", proc.pid)
                            proc.kill()
                        else:
                            log.error("Try: sudo pkill -9 -f %s", runner.load_cmd)

                except PermissionError:
                    log.info("cannot kill proc due to it start with sudo...")
                    os.system(f"sudo kill -9 {proc.pid} ")
                return

            time.sleep(poll_interval)

class device_runner():
    def __init__(self, args):
        self.fw_file = None
        self.lock = threading.Lock()
        self.fw_queue = queue.Queue()

        # Board specific config
        self.board = board_config(args)
        self.load_cmd = self.board.get_cmd()

    def set_fw_ready(self, fw_recv):
        if fw_recv:
            self.fw_queue.put(fw_recv)

    def is_fw_ready(self):
        self.fw_file = self.fw_queue.get()
        log.info(f"Current FW is {self.fw_file}")

        return bool(self.fw_file)

    def cleanup(self):
        self.lock.acquire()
        self.script = None
        if self.fw_file:
            os.remove(self.fw_file)
        self.fw_file = None
        self.lock.release()

    def get_script(self):
        if os.geteuid() != 0:
            self.script = [f'sudo', f'{self.load_cmd}']
        else:
            self.script = [f'{self.load_cmd}']

        self.script.append(f'{self.fw_file}')

        if self.board.params:
            for param in self.board.params:
                self.script.append(param)

        log.info(f'run script: {self.script}')
        return self.script

class board_config():
    def __init__(self, args):

        self.load_cmd = args.load_cmd    # cmd for loading
        self.params = []            # params of loading cmd

        if not self.load_cmd:
            self.load_cmd = "./cavstool.py"

        if not self.load_cmd or not os.path.exists(self.load_cmd):
            log.error(f'Cannot find load cmd {self.load_cmd}.')
            sys.exit(1)

    def get_cmd(self):
        return self.load_cmd

    def get_params(self):
        return self.params


ap = argparse.ArgumentParser(description="RemoteHW service tool", allow_abbrev=False)
ap.add_argument("-q", "--quiet", action="store_true",
                help="No loader output, just DSP logging")
ap.add_argument("-v", "--verbose", action="store_true",
                help="More loader output, DEBUG logging level")
ap.add_argument("-s", "--server-addr",
                help="Specify the only IP address the log server will LISTEN on")
ap.add_argument("-p", "--log-port",
                help="Specify the PORT that the log server to active")
ap.add_argument("-r", "--req-port",
                help="Specify the PORT that the request server to active")
ap.add_argument("-c", "--load-cmd",
                help="Specify loading command of the board")

args = ap.parse_args()

if args.quiet:
    log.setLevel(logging.WARN)
elif args.verbose:
    log.setLevel(logging.DEBUG)

if args.server_addr:
    url = urlparse("//" + args.server_addr)

    if url.hostname:
        HOST = url.hostname

    if url.port:
        PORT_LOG = int(url.port)

if args.log_port:
    PORT_LOG = int(args.log_port)

if args.req_port:
    PORT_REQ = int(args.req_port)

log.info(f"Serve on LOG PORT: {PORT_LOG} REQ PORT: {PORT_REQ}")


if __name__ == "__main__":

    # Do board configuration setup
    runner = device_runner(args)

    # Launch the command request service
    socketserver.TCPServer.allow_reuse_address = True
    req_server = socketserver.TCPServer((HOST, PORT_REQ), adsp_request_handler)
    req_t = threading.Thread(target=req_server.serve_forever, daemon=True)

    # Activate the log service which output board's execution result
    log_server = socketserver.TCPServer((HOST, PORT_LOG), adsp_log_handler)
    log_t = threading.Thread(target=log_server.serve_forever, daemon=True)

    try:
        log.info("Req server start...")
        req_t.start()
        log.info("Log server start...")
        log_t.start()
        req_t.join()
        log_t.join()
    except KeyboardInterrupt:
        log_server.shutdown()
        req_server.shutdown()
