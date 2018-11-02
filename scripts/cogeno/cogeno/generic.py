# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

class GenericMixin(object):
    __slots__ = []

    @staticmethod
    def path_walk(top, topdown = False, followlinks = False):
        """
            See Python docs for os.walk, exact same behavior but it yields Path()
            instances instead

            Form: http://ominian.com/2016/03/29/os-walk-for-pathlib-path/
        """
        names = list(top.iterdir())

        dirs = (node for node in names if node.is_dir() is True)
        nondirs = (node for node in names if node.is_dir() is False)

        if topdown:
            yield top, dirs, nondirs

        for name in dirs:
            if followlinks or name.is_symlink() is False:
                for x in path_walk(name, topdown, followlinks):
                    yield x

        if topdown is not True:
            yield top, dirs, nondirs

    #
    # @param marker Marker as b'my-marker'
    #
    @staticmethod
    def template_files(top, marker, suffix='.c'):
        sources = []
        for path, directory_names, file_names in CoGen.path_walk(top):
            sources.extend([x for x in file_names if x.suffix == suffix])

        templates = []
        for source_file in sources:
            if os.stat(source_file).st_size == 0:
                continue
            with open(source_file, 'rb', 0) as source_file_fd:
                s = mmap.mmap(source_file_fd.fileno(), 0, access=mmap.ACCESS_READ)
                if s.find(marker) != -1:
                    templates.append(source_file)
        return templates
