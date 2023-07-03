# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import sys
import threading
import time
from argparse import ArgumentParser

content = """
The Zen of Python, by Tim Peters

Beautiful is better than ugly.
Explicit is better than implicit.
Simple is better than complex.
Complex is better than complicated.
Flat is better than nested.
Sparse is better than dense.
Readability counts.
Special cases aren't special enough to break the rules.
Although practicality beats purity.
Errors should never pass silently.
Unless explicitly silenced.
In the face of ambiguity, refuse the temptation to guess.
There should be one-- and preferably only one --obvious way to do it.
Although that way may not be obvious at first unless you're Dutch.
Now is better than never.
Although never is often better than *right* now.
If the implementation is hard to explain, it's a bad idea.
If the implementation is easy to explain, it may be a good idea.
Namespaces are one honking great idea -- let's do more of those!
"""


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

    with FifoFile(read_path, 'rb'), FifoFile(write_path, 'wb') as wf:
        for line in content.split('\n'):
            wf.write(f'{line}\n'.encode('utf-8'))
            time.sleep(0.1)
    return 0


if __name__ == '__main__':
    sys.exit(main())
