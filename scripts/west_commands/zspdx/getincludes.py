# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

from subprocess import run, PIPE

from west import log

# Given a path to the applicable C compiler, a C source file, and the
# corresponding TargetCompileGroup, determine which include files would
# be used.
# Arguments:
#   1) path to applicable C compiler
#   2) C source file being analyzed
#   3) TargetCompileGroup for the current target
# Returns: list of paths to include files, or [] on error or empty findings.
def getCIncludes(compilerPath, srcFile, tcg):
    log.dbg(f"    - getting includes for {srcFile}")

    # prepare fragments
    fragments = [fr for fr in tcg.compileCommandFragments if fr.strip() != ""]

    # prepare include arguments
    includes = ["-I" + incl.path for incl in tcg.includes]

    # prepare defines
    defines = ["-D" + d.define for d in tcg.defines]

    # prepare command invocation
    cmd = [compilerPath, "-E", "-H"] + fragments + includes + defines + [srcFile]

    cp = run(cmd, stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if cp.returncode != 0:
        log.dbg(f"    - calling {compilerPath} failed with error code {cp.returncode}")
        return []
    else:
        # response will be in cp.stderr, not cp.stdout
        return extractIncludes(cp.stderr)

# Parse the response from the CC -E -H call, to extract the include file paths
def extractIncludes(resp):
    includes = set()

    # lines we want will start with one or more periods, followed by
    # a space and then the include file path, e.g.:
    # .... /home/steve/programming/zephyr/zephyrproject/zephyr/include/kernel.h
    # the number of periods indicates the depth of nesting (for transitively-
    # included files), but here we aren't going to care about that. We'll
    # treat everything as tied to the corresponding source file.

    # once we hit the line "Multiple include guards may be useful for:",
    # we're done; ignore everything after that

    for rline in resp.splitlines():
        if rline.startswith("Multiple include guards"):
            break
        if rline[0] == ".":
            sline = rline.split(" ", maxsplit=1)
            if len(sline) != 2:
                continue
            includes.add(sline[1])

    includesList = list(includes)
    includesList.sort()
    return includesList
