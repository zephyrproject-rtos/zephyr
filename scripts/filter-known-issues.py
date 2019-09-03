#! /usr/bin/env python3
"""
Filters a file, classifying output in errors, warnings and discarding
the rest.

Given a set of regular expressions read from files named *.conf in the
given configuration path(s), of the format:

  #
  # Comments for multiline regex 1...
  #
  MULTILINEPYTHONREGEX
  MULTILINEPYTHONREGEX
  MULTILINEPYTHONREGEX
  #
  # Comments for multiline regex 2...
  #
  #WARNING
  MULTILINEPYTHONREGEX2
  MULTILINEPYTHONREGEX2

Anything matched by MULTILINEPYTHONREGEX will be considered something
to be filtered out and not printed.

Anything matched by MULTILINEPYHONREGEX with a #WARNING tag in the
comment means (optional) means that it describes something that is
considered to be a warning. Print it to stderr.

Anything leftover is considred to be errors, printed to stdout.

"""
import argparse
import logging
import mmap
import os
import re
import sre_constants
import sys
import traceback

exclude_regexs = []

# first is a list of one or more comment lines
# followed by a list of non-comments which describe a multiline regex
config_regex = \
    b"(?P<comment>(^\\s*#.*\n)+)" \
    b"(?P<regex>(^[^#].*\n)+)"


def config_import_file(filename):
    """
    Imports regular expression from any file *.conf in the given path,
    format as given in the main doc
    """
    try:
        with open(filename, "rb") as f:
            mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            # That regex basically selects any block of
            # lines that is not a comment block. The
            # finditer() finds all the blocks and selects
            # the bits of mmapped-file that comprises
            # each--we compile it into a regex and append.
            for m in re.finditer(config_regex, mm, re.MULTILINE):
                origin = "%s:%s-%s" % (filename, m.start(), m.end())
                gd = m.groupdict()
                comment = gd['comment']
                regex = gd['regex']
                try:
                    r = re.compile(regex, re.MULTILINE)
                except sre_constants.error as e:
                    logging.error("%s: bytes %d-%d: bad regex: %s",
                                  filename, m.start(), m.end(), e)
                    raise
                logging.debug("%s: found regex at bytes %d-%d: %s",
                              filename, m.start(), m.end(), regex)
                if b'#WARNING' in comment:
                    exclude_regexs.append((r, origin, ('warning',)))
                else:
                    exclude_regexs.append((r, origin, ()))
            logging.debug("%s: loaded", filename)
    except Exception as e:
        logging.error("E: %s: can't load config file: %s" % (filename, e))
        raise


def config_import_path(path):
    """
    Imports regular expression from any file *.conf in the given path
    """
    file_regex = re.compile(r".*\.conf$")
    try:
        for dirpath, _, filenames in os.walk(path):
            for _filename in sorted(filenames):
                filename = os.path.join(dirpath, _filename)
                if not file_regex.search(_filename):
                    logging.debug("%s: ignored", filename)
                    continue
                config_import_file(filename)
    except Exception as e:
        raise Exception(
            "E: %s: can't load config files: %s %s" %
            (path, e, traceback.format_exc()))


def config_import(paths):
    """
    Imports regular expression from any file *.conf in the list of paths.

    If a path is "" or None, the list of paths until then is flushed
    and only the new ones are considered.
    """
    _paths = []
    # Go over the list, flush it if the user gave an empty path ("")
    for path in paths:
        if path == "" or path is None:
            logging.debug("flushing current config list: %s", _paths)
            _paths = []
        else:
            _paths.append(path)
    logging.debug("config list: %s", _paths)
    for path in _paths:
        config_import_path(path)


arg_parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter)
arg_parser.add_argument("-v", "--verbosity", action="count", default=0,
                        help="increase verbosity")
arg_parser.add_argument("-q", "--quiet", action="count", default=0,
                        help="decrease verbosity")
arg_parser.add_argument("-e", "--errors", action="store", default=None,
                        help="file where to store errors")
arg_parser.add_argument("-w", "--warnings", action="store", default=None,
                        help="file where to store warnings")
arg_parser.add_argument("-c", "--config-dir", action="append", nargs="?",
                        default=[".known-issues/"],
                        help="configuration directory (multiple can be "
                        "given; if none given, clears the current list) "
                        "%(default)s")
arg_parser.add_argument("FILENAMEs", nargs="+",
                        help="files to filter")
args = arg_parser.parse_args()

logging.basicConfig(level=40 - 10 * (args.verbosity - args.quiet),
                    format="%(levelname)s: %(message)s")

path = ".known-issues/"
logging.debug("Reading configuration from directory `%s`", path)
config_import(args.config_dir)

exclude_ranges = []

if args.warnings:
    warnings = open(args.warnings, "w")
else:
    warnings = None
if args.errors:
    errors = open(args.errors, "w")
else:
    errors = None

def report_error(data):
    sys.stdout.write(data.decode('utf-8'))
    if errors:
        errors.write(data.decode('utf-8'))


def report_warning(data):
    sys.stderr.write(data.decode('utf-8'))
    if warnings:
        warnings.write(data.decode('utf-8'))

for filename in args.FILENAMEs:
    if os.stat(filename).st_size == 0:
        continue  # skip empty log files
    try:
        with open(filename, "r+b") as f:
            logging.info("%s: filtering", filename)
            # Yeah, this should be more protected in case of exception
            # and such, but this is a short running program...
            mm = mmap.mmap(f.fileno(), 0)
            for ex, origin, flags in exclude_regexs:
                logging.info("%s: searching from %s: %s",
                             filename, origin, ex.pattern)
                for m in re.finditer(ex.pattern, mm, re.MULTILINE):
                    logging.info("%s: %s-%s: match from from %s %s",
                                 filename, m.start(), m.end(), origin, flags)
                    if 'warning' in flags:
                        exclude_ranges.append((m.start(), m.end(), True))
                    else:
                        exclude_ranges.append((m.start(), m.end(), False))

            exclude_ranges = sorted(exclude_ranges, key=lambda r: r[0])
            logging.warning(
                "%s: ranges excluded: %s",
                filename,
                exclude_ranges)

            # Decide what to do with what has been filtered; warnings
            # go to stderr and warnings file, errors to stdout, what
            # is ignored is just dumped.
            offset = 0
            for b, e, warning in exclude_ranges:
                mm.seek(offset)
                if b > offset:
                    # We have something not caught by a filter, an error
                    logging.info("%s: error range (%d, %d), from %d %dB",
                                 filename, offset, b, offset, b - offset)
                    report_error(mm.read(b - offset))
                    mm.seek(b)
                if warning:			# A warning, print it
                    mm.seek(b)
                    logging.info("%s: warning range (%d, %d), from %d %dB",
                                 filename, b, e, offset, e - b)
                    report_warning(mm.read(e - b))
                else:				# Exclude, ignore it
                    d = b - offset
                    logging.info("%s: exclude range (%d, %d), from %d %dB",
                                 filename, b, e, offset, d)
                offset = e
            mm.seek(offset)
            if len(mm) != offset:
                logging.info("%s: error final range from %d %dB",
                             filename, offset, len(mm))
                report_error(mm.read(len(mm) - offset - 1))
            del mm
    except Exception as e:
        logging.error("%s: cannot load: %s", filename, e)
        raise

if warnings or errors:
    if warnings:
        warnings.flush()
    if errors:
        errors.flush()
    if ((os.path.isfile(args.warnings) and os.path.getsize(args.warnings) > 0) or
        (os.path.isfile(args.errors) and os.path.getsize(args.errors) > 0)):
        print('''\n\n ---- New errors/warnings not tracked as .known-issues/, \
please fix them ----\n''')
        if args.warnings:
            print(open(args.warnings, "r").read())
        if args.errors and (args.errors != args.warnings):
            print(open(args.errors, "r").read())
    else:
        print("\n\nNo new errors/warnings.\n")

    print('''\nTo see *all* new error/warnings you must make/ninja clean and \
rebuild from scratch.''')
