"""
Copyright (c) 2021 Zephyr Project members and individual contributors
SPDX-License-Identifier: Apache-2.0

This module contains a variable with a list of tuples (old_url, new_url) for
pages to redirect. This list allows redirecting old URLs (caused by reorganizing
doc directories)

Notes:
    - Please keep this list sorted alphabetically.
    - URLs must be relative to document root (with NO leading slash), and
      without the html extension).
"""

REDIRECTS = [
    ('guides/debugging/coredump', 'guides/flash_debug/coredump'),
    ('guides/debugging/gdbstub', 'guides/flash_debug/gdbstub'),
    ('guides/debugging/host-tools', 'guides/flash_debug/host-tools'),
    ('guides/debugging/index', 'guides/flash_debug/index'),
    ('guides/debugging/probes', 'guides/flash_debug/probes'),
    ('guides/debugging/thread-analyzer', 'guides/flash_debug/thread-analyzer'),
    ('guides/c_library', 'reference/libc/index'),
    ('guides/west/repo-tool', 'guides/west/basics'),
]
