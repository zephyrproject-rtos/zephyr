# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import sys
import threading
import time
from argparse import ArgumentParser

from zen_of_python import zen_of_python


class FifoFile:
    def __init__(self, filename, mode):
        self.filename = filename
        self.mode = mode
        self.thread = None
        self.file = None
        self.logger = logging.getLogger(__name__)

    def _open(self):
        self.logger.info(f'Creating fifo file: {self.filename}')
        end_time = time.time() + 2
        while not os.path.exists(self.filename):
            time.sleep(0.1)
            if time.time() > end_time:
                self.logger.error(f'Did not able create fifo file: {self.filename}')
                return
        self.file = open(self.filename, self.mode, buffering=0)
        self.logger.info(f'File created: {self.filename}')

    def open(self):
        self.thread = threading.Thread(target=self._open(), daemon=True)
        self.thread.start()

    def write(self, data):
        if self.file:
            self.file.write(data)

    def read(self):
        if self.file:
            return self.file.readline()

    def close(self):
        if self.file:
            self.file.close()
        self.thread.join(1)
        self.logger.info(f'Closed file: {self.filename}')

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


def main():
    logging.basicConfig(level='DEBUG')
    parser = ArgumentParser()
    parser.add_argument('file')
    args = parser.parse_args()
    read_path = args.file + '.in'
    write_path = args.file + '.out'
    logger = logging.getLogger(__name__)
    logger.info('Start')

    with FifoFile(write_path, 'wb') as wf, FifoFile(read_path, 'rb'):
        for line in zen_of_python:
            wf.write(f'{line}\n'.encode('utf-8'))
    time.sleep(1)  # give a moment for external programs to collect all outputs
    return 0


if __name__ == '__main__':
    sys.exit(main())
