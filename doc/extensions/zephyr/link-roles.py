# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# based on http://protips.readthedocs.io/link-roles.html

from __future__ import print_function
from __future__ import unicode_literals
import re
import os
from docutils import nodes
from local_util import run_cmd_get_output


def get_github_rev():
    tag = run_cmd_get_output('git describe --exact-match')
    if len(tag):
        return(tag)
    else:
        return 'master'


def setup(app):
    rev = get_github_rev()

    # links to files or folders on the GitHub
    baseurl = 'https://github.com/zephyrproject-rtos/zephyr'
    app.add_role('zephyr_file', autolink('{}/blob/{}/%s'.format(baseurl, rev)))
    app.add_role('zephyr_raw', autolink('{}/raw/{}/%s'.format(baseurl, rev)))


def autolink(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        m = re.search('(.*)\s*<(.*)>', text)  # noqa: W605 - regular expression
        if m:
            link_text = m.group(1)
            link = m.group(2)
        else:
            link_text = text
            link = text
        url = pattern % (link,)
        node = nodes.reference(rawtext, link_text, refuri=url, **options)
        return [node], []
    return role
