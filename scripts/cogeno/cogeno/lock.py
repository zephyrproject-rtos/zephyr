# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from filelock import Timeout, FileLock
else:
    # use current package visibility
    from .filelock import Timeout, FileLock

class LockMixin(object):
    __slots__ = []

    _lock = None

    ##
    # @brief lock
    #
    # @return lock
    #
    def lock(self):
        if self._lock is None:
            self._lock = FileLock(self._context._lock_file)
        return self._lock

    @property
    def LockTimout(self):
        return Timeout

