# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

class LogMixin(object):
    __slots__ = []

    _log_fd = None

    def log(self, message, message_type=None, end="\n", logonly=True):
        if self._log_fd is None:
            if self.options.log_file is not None:
                log_file = Path(self.options.log_file)
                if not log_file.is_file():
                    # log file will be created
                    # add preamble
                    preamble = "My preamble\n{}".format(message)
                self._log_fd = open(str(log_file), 'a')
        if message_type is None:
            message_type = ''
        else:
            message_type = message_type+': '
        if self._log_fd is not None:
            for line in message.splitlines():
                self._log_fd.write("{}{}{}".format(message_type, line, end))
        if not logonly:
            print(message_type+message, file=self._stderr, end=end)

    def msg(self, s):
        self.log(s, message_type='message', logonly=False)

    def warning(self, s):
        self.log(s, message_type='warning', logonly=False)

    def prout(self, s, end="\n"):
        self.log(s, message_type=None, end=end, logonly=False)

    def prerr(self, s, end="\n"):
        self.log(s, message_type='error', end=end, logonly=False)
