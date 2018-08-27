# Copyright 2004-2016, Ned Batchelder.
#           http://nedbatchelder.com/code/cog
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

import re

def b(s):
    return s.encode("latin-1")

def whitePrefix(strings):
    """ Determine the whitespace prefix common to all non-blank lines
        in the argument list.
    """
    # Remove all blank lines from the list
    strings = [s for s in strings if s.strip() != '']

    if not strings: return ''

    # Find initial whitespace chunk in the first line.
    # This is the best prefix we can hope for.
    pat = r'\s*'
    if isinstance(strings[0], (bytes, )):
        pat = pat.encode("utf8")
    prefix = re.match(pat, strings[0]).group(0)

    # Loop over the other strings, keeping only as much of
    # the prefix as matches each string.
    for s in strings:
        for i in range(len(prefix)):
            if prefix[i] != s[i]:
                prefix = prefix[:i]
                break
    return prefix

def reindentBlock(lines, newIndent=''):
    """ Take a block of text as a string or list of lines.
        Remove any common whitespace indentation.
        Re-indent using newIndent, and return it as a single string.
    """
    sep, nothing = '\n', ''
    if isinstance(lines, (bytes, )):
        sep, nothing = b('\n'), b('')
    if isinstance(lines, (str, bytes)):
        lines = lines.split(sep)
    oldIndent = whitePrefix(lines)
    outLines = []
    for l in lines:
        if oldIndent:
            l = l.replace(oldIndent, nothing, 1)
        if l and newIndent:
            l = newIndent + l
        outLines.append(l)
    return sep.join(outLines)

def commonPrefix(strings):
    """ Find the longest string that is a prefix of all the strings.
    """
    if not strings:
        return ''
    prefix = strings[0]
    for s in strings:
        if len(s) < len(prefix):
            prefix = prefix[:len(s)]
        if not prefix:
            return ''
        for i in range(len(prefix)):
            if prefix[i] != s[i]:
                prefix = prefix[:i]
                break
    return prefix
