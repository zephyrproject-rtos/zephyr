# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

class LogMixin(object):
    __slots__ = []

    _log_file = None

    def log(self, message, message_type=None, end="\n", logonly=True):
        if self._log_file is None:
            if self._context._log_file is None:
                # No file logging specified
                pass
            elif self._context._log_file == '-':
                # log to stdout
                pass
            else:
                log_file = Path(self._context._log_file)
                try:
                    with self.lock().acquire(timeout = 10):
                        if not log_file.is_file():
                            # log file will be created
                            # add preamble
                            preamble = "My preamble\n{}".format(message)
                            with log_file.open(mode = 'a', encoding = 'utf-8') as log_fd:
                                log_fd.write(preamble)
                    self._log_file = log_file
                except self.LockTimeout:
                    # Something went really wrong - we did not get the lock
                    self.error(
                        "Log file '{}' no access."
                        .format(log_file), frame_index = 2)
                except:
                    raise
        if message_type is None:
            message_type = ''
        else:
            message_type = message_type+': '
        if not self._log_file is None:
            # Write message to log file
            try:
                with self.lock().acquire(timeout = 10):
                    with self._log_file.open(mode = 'a', encoding = 'utf-8') as log_fd:
                        for line in message.splitlines():
                            log_fd.write("{}{}{}".format(message_type, line, end))
                        log_fd.flush()
            except Timeout:
                # Something went really wrong - we did not get the lock
                self.error(
                    "Log file '{}' no access."
                    .format(log_file), frame_index = 2)
            except:
                raise
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
