# Copyright (c) 2019, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import ast
import fnmatch
import os

import docutils
import sphinx

docs_to_include = set()
docs_to_remove = set()
external_links = {}
fp_patterns = []

if "ZEPHYR_BASE" not in os.environ:
    sys.stderr.write("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)
ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]

#
# This reads in the file path patterns from doc/scopes/${scope}.patterns
# Each line in the file contains one pattern which is used to match
# against the relative path of document files from Sphinx using fnmatch().
#
# The pattern is relative to document source directory.
# For example, to match "doc/_build/rst/doc/kernel/kernel.rst",
# the pattern is "kernel/kernel.rst" since "doc/_build/rst/doc"
# is the root of document source directory.
#
# If the pattern has a file extension that is defined in Sphinx
# configuration 'source_suffix', the file extension will be removed
# from the pattern as Sphinx removes them also before storing the paths.
#
# Note that fnmatch() performs path normalization before matching
# which allows the patterns in the file to be Unix style path.
# The normalization will transform the pattern into one suitable
# for the operating system.
#
# For example, doc/scopes/kernel.patterns:
#
#     index.rst
#     api/api.rst
#     api/kernel_api.rst
#     application/index.rst
#     kernel/*
#     reference/kconfig/*
#
def _get_file_path_patterns(app, scope):
    global fp_patterns

    filepath = os.path.join(ZEPHYR_BASE, "doc", "scopes", scope + ".patterns")
    with open(filepath) as f:
        for line in f:
            # Normalize path so it works cross-platform
            path = os.path.normcase(line.strip())

            # Skip empty lines
            if not path:
                continue

            # Strip file extensions
            for suffix in app.config.source_suffix:
                idx = path.endswith(suffix)
                if idx > 0:
                    path = path[:-len(suffix)]
                    break

            if path:
                fp_patterns.append(path)

    if not fp_patterns:
        raise Exception("No usable pattern defined in {}!".format(filepath))


#
# This reads in the reference (:ref:) to external URL mapping
# from doc/scopes/${scope}.extlinks if this file exists.
#
# The content of the file is a Python dictionary, and will be parsed
# as literal. Each item must have two strings, "name" and "url".
# The "name" is the text which will be put in the output document
# replacing the :ref:<...> text. The "url" will be used to generate
# the necessary hyperlink in the output.
#
# When generating a subset of document, some RST files are no longer
# included in the build. The references (:ref:) to labels within
# these files can no longer be resolved. External links are needed
# to point them to the online documentation to avoid broken references.
#
# For example, doc/scopes/kernel.extlinks
#
#    {
#        'application':
#            {
#                'name': 'Application Development Primer',
#                'url' : 'https://docs.zephyrproject.org/latest/application/application.html'
#            },
#        'glossary':
#            {
#                'name': 'Glossary',
#                'url' : 'https://docs.zephyrproject.org/latest/glossary.html#glossary'
#            },
#        'zephyr_licensing':
#            {
#                'name': 'Licensing of Zephyr Project components',
#                'url' : 'https://docs.zephyrproject.org/latest/LICENSING.html'
#            },
#        'zephyr_release_notes':
#            {
#                'name': 'Release Notes',
#                'url' : 'https://docs.zephyrproject.org/latest/release-notes.html'
#            }
#    }
#
def _get_external_links(app, scope):
    global external_links

    filepath = os.path.join(ZEPHYR_BASE, "doc", "scopes", scope + ".extlinks")

    if os.path.isfile(filepath):
        with open(os.path.join(filepath)) as f:
            external_links = ast.literal_eval(f.read())


def _init(app, scope):
    _get_file_path_patterns(app, scope)
    _get_external_links(app, scope)


#
# This filters the list of document found by Sphinx using
# the file path patterns. Sphinx then only processes
# the files in the filtered list. This also populates
# a list of removed files which will be used to filter
# the TOC trees in later build stage.
#
def builder_inited(app):
    global docs_to_include
    global docs_to_remove
    global fp_patterns

    # Only manipulate documents list if scope is specified
    scope = app.config.zephyr_doc_scope
    if scope is None:
        return

    _init(app, scope)

    # Trim the document list according to file path patterns
    for pattern in fp_patterns:
        docs_to_include.update(fnmatch.filter(app.env.found_docs, pattern))

    docs_to_remove = app.env.found_docs.copy()
    docs_to_remove.difference_update(docs_to_include)

    app.env.found_docs.intersection_update(docs_to_include)


#
# After Sphinx figures out what files to be added,
# which changes, and what to remove, we need to update
# those lists with our patterns.
#
# Note this is also called on first run, since technically
# the environment is "out-dated".
#
def env_get_outdated(app, env, added, changed, removed):
    global docs_to_remove

    # Only manipulate documents list if scope is specified
    scope = app.config.zephyr_doc_scope
    if scope is None:
        return []

    added.difference_update(docs_to_remove)
    changed.difference_update(docs_to_remove)
    removed.update(docs_to_remove)

    return []


#
# For each parsed document, goes through the TOC tree (.. toctree::)
# and removes the entries that are out of scope.
#
def doctree_read(app, doctree):
    global docs_to_remove

    # Only process files if scope is specified
    scope = app.config.zephyr_doc_scope
    if scope is None:
        return

    # Traverse the TOC nodes and removes any entries and
    # references to out-of-scope documents
    for toctreenode in doctree.traverse(sphinx.addnodes.toctree):
        to_remove = []

        for e in toctreenode['entries']:
            ref = str(e[1])
            if ref in docs_to_remove:
                to_remove.append(e)

        if to_remove:
            for e in to_remove:
                toctreenode['entries'].remove(e)
                toctreenode['includefiles'].remove(e[1])


#
# Sphinx calls this when a docutil node has unresolved
# references. This function replaces these references
# with external links.
#
def missing_reference(app, env, node, contnode):
    global external_links

    # Only process missing refs if scope is specified
    scope = app.config.zephyr_doc_scope
    if scope is None:
        return

    # External link list is empty of does not exist
    if not external_links:
        return

    # References (:ref:) that are in out-of-scope documents
    # need to be replaced with links to online document
    if node['reftype'] == 'ref':
        tgt = node['reftarget']
        if tgt in external_links:
            newnode = docutils.nodes.reference(
                          external_links[tgt]['name'],
                          external_links[tgt]['name'],
                          refuri = external_links[tgt]['url'],
                          internal = False)

            return newnode

    return None


def setup(app):
    app.add_config_value('zephyr_doc_scope', None, 'env')

    app.connect('builder-inited', builder_inited)
    app.connect('env-get-outdated', env_get_outdated)
    app.connect('doctree-read', doctree_read)
    app.connect('missing-reference', missing_reference)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True
    }
