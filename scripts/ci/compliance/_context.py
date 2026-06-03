# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Shared API for compliance linter modules loaded from the compliance/
# sub-directory.  Linter modules import this module as:
#
#   import _compliance_context as _ctx
#
# Two categories of symbols are exposed:
#
# Static symbols — populated by the loader (_load_compliance_linters in
# check_compliance.py) immediately after this module is registered in
# sys.modules, before any linter module is imported.  They are safe to
# use at class-definition time (e.g. as default arguments or class
# attributes).
#
# Dynamic symbols — populated by _main() after the argument parser runs.
# They must only be accessed inside method bodies, not at module or
# class-definition time.

# --- Static symbols (available at linter import time) ---

# Base class all linter checks must subclass.
ComplianceTest = None

# Exception raised by ComplianceTest.error() / .skip().
EndTest = None

# Returns the list of files added/modified by the commit range.
get_files = None

# Returns the list of Git SHAs for a refspec.
get_shas = None

# Runs a git command and returns stdout.
git = None

# Loads a file into a set, one element per line (comments stripped).
get_set_from_file = None

# Builds a "See <url> for more details." documentation string.
zephyr_doc_detail_builder = None

# Absolute path to the Zephyr base directory.
ZEPHYR_BASE = None

# python-magic file-type detection library (may be the bundled Windows shim).
magic = None

# --- Dynamic symbols (available only inside method bodies at run time) ---

# Absolute path of the top-level git directory (set by _main()).
GIT_TOP = None

# Commit range passed via --commits, e.g. "HEAD~3" (set by _main()).
COMMIT_RANGE = None
