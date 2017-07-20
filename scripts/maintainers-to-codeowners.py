#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#
# This script converts the MAINTAINERS file to the CODEOWNERS format
# as used by GitHub.
#     Blog post about the feature:
#		https://github.com/blog/2392-introducing-code-owners
#     CODEOWNERS format:
#		https://help.github.com/articles/about-codeowners/
#

import os
import re

email_re = re.compile('([a-zA-Z0-9\.\-]+@[a-zA-Z0-9\.\-]+)')

maint_path = os.path.join(os.getenv('ZEPHYR_BASE'), 'MAINTAINERS')
maintainers = open(maint_path, 'r')

reading_comments = True
in_entry = False
entry_path = []
entry_owner = []

def dump_entries():
  for path in entry_path:
    if path.endswith('/'):
      path = path + '*'

    print('%s\t%s' % (path, '    '.join(entry_owner)))

for line in maintainers:
  line = line.strip()

  if '-------------' in line:
    reading_comments = False
    continue
  if reading_comments:
    continue

  if not in_entry:
    if line:
      print('\n# %s' % line)
      in_entry = True
    continue

  if not line:
    dump_entries()
    in_entry = False
    entry_path = []
    entry_owner = []
    continue

  if line.startswith('M:'):
    for addr in email_re.findall(line):
      entry_owner.append(addr)
  elif line.startswith('F:'):
    entry_path.append(line.split(':')[1].strip())

dump_entries()
