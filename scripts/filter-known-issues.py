#! /usr/bin/python
import argparse
import logging
import mmap
import os
import re
import sys

exclude_regexs = []

noncomment_regex = re.compile(
        "(^[ \t][^#]+.*\n)+"
        , re.MULTILINE)

def config_import_file(filename):
    """
    Imports regular expresions from any file *.conf in the given path

    Each file follows the format::

      #
      # Comments for multiline regex 1...
      #
      multilineregex
      multilineregex
      multilineregex
      #
      # Comments for multiline regex 2...
      #
      multilineregex
      multilineregex
      multilineregex

    etc.
    """
    try:
        with open(filename, "rb") as f:
            mm = mmap.mmap(f.fileno(), 0, access = mmap.ACCESS_READ)
            # That regex basically selects any block of
            # lines that is not a comment block. The
            # finditer() finds all the blocks and selects
            # the bits of mmapped-file that comprises
            # each--we compile it into a regex and append.
            for m in re.finditer("(^\s*[^#].*\n)+", mm, re.MULTILINE):
                origin = "%s:%s-%s" % (filename, m.start(), m.end())
                try:
                    r = re.compile(mm[m.start():m.end()], re.MULTILINE)
                except Exception as e:
                    logging.error("%s: bytes %d-%d: bad regex: %s",
                                  filename, m.start(), m.end(), e)
                    raise
                logging.debug("%s: found regex at bytes %d-%d: %s",
                              filename, m.start(), m.end(),
                              mm[m.start():m.end()])
                exclude_regexs.append((r, origin))
            logging.debug("%s: loaded", filename)
    except Exception as e:
        raise Exception("E: %s: can't load config file: %s" % (filename, e))

def config_import_path(path):
    """
    Imports regular expresions from any file *.conf in the given path
    """
    file_regex = re.compile(".*\.conf$")
    try:
        for dirpath, dirnames, filenames in os.walk(path):
            for _filename in sorted(filenames):
                filename = os.path.join(dirpath, _filename)
                if not file_regex.search(_filename):
                    logging.debug("%s: ignored", filename)
                    continue
                config_import_file(filename)
    except Exception as e:
        raise Exception("E: %s: can't load config files: %s" % (path, e))

def config_import(paths):
    """
    Imports regular expresions from any file *.conf in the list of paths.

    If a path is "" or None, the list of paths until then is flushed
    and only the new ones are considered.
    """
    _paths = []
    # Go over the list, flush it if the user gave an empty path ("")
    for path in paths:
        if path == "" or path == None:
            logging.debug("flushing current config list: %s", _paths)
            _paths = []
        else:
            _paths.append(path)
    logging.debug("config list: %s", _paths)
    for path in _paths:
        config_import_path(path)

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-v", "--verbosity", action = "count", default = 0,
                        help = "increase verbosity")
arg_parser.add_argument("-q", "--quiet", action = "count", default = 0,
                        help = "decrease verbosity")
arg_parser.add_argument("-c", "--config-dir", action = "append", nargs = "?",
                        default = [ ".known-issues/" ],
                        help = "configuration directory (multiple can be "
                        "given; if none given, clears the current list) "
                        "%(default)s")
arg_parser.add_argument("FILENAMEs", nargs = "+",
                        help = "files to filter")
args = arg_parser.parse_args()

logging.basicConfig(level = 40 - 10 * (args.verbosity - args.quiet),
                    format = "%(levelname)s: %(message)s")

path = ".known-issues/"
logging.debug("Reading configuration from directory `%s`", path)
config_import(args.config_dir)

exclude_ranges = []

for filename in args.FILENAMEs:
    try:
        with open(filename, "r+b") as f:
            logging.info("%s: filtering", filename)
            # Yeah, this should be more protected in case of exception
            # and such, but this is a short running program...
            mm = mmap.mmap(f.fileno(), 0)
            for ex, origin in exclude_regexs:
                logging.info("%s: searching from %s: %s",
                             filename, origin, ex.pattern)
                for m in re.finditer(ex.pattern, mm, re.MULTILINE):
                    logging.debug("%s: %s-%s: match from from %s",
                                  filename, m.start(), m.end(), origin)
                    exclude_ranges.append((m.start(), m.end()))

            exclude_ranges = sorted(exclude_ranges, key=lambda r: r[0])
            logging.warning("%s: ranges excluded: %s", filename, exclude_ranges)

            # Printd what has not been filtered
            offset = 0
            for b, e in exclude_ranges:
                mm.seek(offset)
                d = b - offset
                logging.debug("%s: exclude range (%d, %d), from %d %dB",
                              filename, b, e, offset, d)
                if b > offset:
                    print(mm.read(d - 1))
                offset = e
            mm.seek(offset)
            if len(mm) != offset:
                print mm.read(len(mm) - offset - 1)
            del mm
    except Exception as e:
        logging.error("%s: cannot load: %s", filename, e)
