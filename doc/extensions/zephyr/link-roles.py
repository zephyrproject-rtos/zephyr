# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# based on http://protips.readthedocs.io/link-roles.html

from __future__ import print_function
from __future__ import unicode_literals
import re
from docutils import nodes
from local_util import run_cmd_get_output


def get_github_rev():
    tag = run_cmd_get_output('git describe --exact-match')
    if tag:
        return tag.decode("utf-8")
    else:
        return 'master'


def setup(app):
    rev = get_github_rev()

    # links to files or folders on the GitHub
    baseurl = 'https://github.com/zephyrproject-rtos/zephyr'
    app.add_role('zephyr_file', autolink('{}/blob/{}/%s'.format(baseurl, rev)))
    app.add_role('zephyr_raw', autolink('{}/raw/{}/%s'.format(baseurl, rev)))

    # The role just creates new nodes based on information in the
    # arguments; its behavior doesn't depend on any other documents.
    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }


def autolink(pattern):
    def role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        m = re.search(r'(.*)\s*<(.*)>', text)
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
