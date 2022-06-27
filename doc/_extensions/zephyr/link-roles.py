# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# based on http://protips.readthedocs.io/link-roles.html

from __future__ import print_function
from __future__ import unicode_literals
import re
import subprocess
from docutils import nodes
try:
    import west.manifest
    try:
        west_manifest = west.manifest.Manifest.from_file()
    except west.util.WestNotFound:
        west_manifest = None
except ImportError:
    west_manifest = None


def get_github_rev():
    try:
        output = subprocess.check_output('git describe --exact-match',
                                         shell=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return 'main'

    return output.strip().decode('utf-8')


def setup(app):
    rev = get_github_rev()

    # Try to get the zephyr repository's GitHub URL from the manifest.
    #
    # This allows building the docs in downstream Zephyr-based
    # software with forks of the zephyr repository, and getting
    # :zephyr_file: / :zephyr_raw: output that links to the fork,
    # instead of mainline zephyr.
    baseurl = None
    if west_manifest is not None:
        try:
            # This search tries to look up a project named 'zephyr'.
            # If zephyr is the manifest repository, this raises
            # ValueError, since there isn't any such project.
            baseurl = west_manifest.get_projects(['zephyr'],
                                                 allow_paths=False)[0].url
            # Spot check that we have a non-empty URL.
            assert baseurl
        except ValueError:
            pass

    # If the search failed, fall back on the mainline URL.
    if baseurl is None:
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
