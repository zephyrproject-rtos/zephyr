# Copyright 2004-2016, Ned Batchelder.
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

import sys

class RedirectableMixin(object):
    __slots__ = []

    def set_standard_streams(self, stdout=None, stderr=None):
        """ Assign new files for standard out and/or standard error.
        """
        if stdout:
            self._stdout = stdout
        if stderr:
            self._stderr = stderr

    def prout(self, s, end="\n"):
        print(s, file=self._stdout, end=end)

    def prerr(self, s, end="\n"):
        print(s, file=self._stderr, end=end)


class Redirectable(RedirectableMixin):
    """ An object with its own stdout and stderr files.
    """
    def __init__(self):
        self._stdout = sys.stdout
        self._stderr = sys.stderr
