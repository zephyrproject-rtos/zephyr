# Copyright 2004-2016, Ned Batchelder.
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

class NumberedFileReader:
    """ A decorator for files that counts the readline()'s called.
    """
    def __init__(self, f):
        self.f = f
        self.n = 0

    def readline(self):
        l = self.f.readline()
        if l:
            self.n += 1
        return l

    def linenumber(self):
        return self.n
