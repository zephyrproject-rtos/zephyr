#!/usr/bin/env python3
#
# Copyright (c) 2017, Linaro Limited
# Copyright (c) 2018, Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

import os, fnmatch
import re
import yaml
import collections

from pathlib import Path

class Binder(yaml.Loader):

    ##
    # List of all yaml files available for yaml loaders
    # of this class. Must be preset before the first
    # load operation.
    _files = []

    ##
    # Files that are already included.
    # Must be reset on the load of every new binding
    _included = []

    @staticmethod
    def dict_merge(dct, merge_dct):
        # from https://gist.github.com/angstwad/bf22d1822c38a92ec0a9

        """ Recursive dict merge. Inspired by :meth:``dict.update()``, instead of
        updating only top-level keys, dict_merge recurses down into dicts nested
        to an arbitrary depth, updating keys. The ``merge_dct`` is merged into
        ``dct``.
        :param dct: dict onto which the merge is executed
        :param merge_dct: dct merged into dct
        :return: None
        """
        for k, v in merge_dct.items():
            if (k in dct and isinstance(dct[k], dict)
                    and isinstance(merge_dct[k], collections.Mapping)):
                Binder.dict_merge(dct[k], merge_dct[k])
            else:
                if k in dct and dct[k] != merge_dct[k]:
                    print("binder.py: Merge of '{}': '{}'  overwrites '{}'.".format(
                            k, merge_dct[k], dct[k]))
                dct[k] = merge_dct[k]

    @classmethod
    def _traverse_inherited(cls, node):
        """ Recursive overload procedure inside ``node``
        ``inherits`` section is searched for and used as node base when found.
        Base values are then overloaded by node values
        and some consistency checks are done.
        :param node:
        :return: node
        """

        # do some consistency checks. Especially id is needed for further
        # processing. title must be first to check.
        if 'title' not in node:
            # If 'title' is missing, make fault finding more easy.
            # Give a hint what node we are looking at.
            print("binder.py: node without 'title' -", node)
        for prop in ('title', 'version', 'description'):
            if prop not in node:
                node[prop] = "<unknown {}>".format(prop)
                print("binder.py: WARNING:",
                      "'{}' property missing in".format(prop),
                      "'{}' binding. Using '{}'.".format(node['title'],
                                                         node[prop]))

        # warn if we have an 'id' field
        if 'id' in node:
            print("binder.py: WARNING: id field set",
                  "in '{}', should be removed.".format(node['title']))

        if 'inherits' in node:
            if isinstance(node['inherits'], list):
                inherits_list  = node['inherits']
            else:
                inherits_list  = [node['inherits'],]
            node.pop('inherits')
            for inherits in inherits_list:
                if 'inherits' in inherits:
                    inherits = cls._traverse_inherited(inherits)
                if 'type' in inherits:
                    if 'type' not in node:
                        node['type'] = []
                    if not isinstance(node['type'], list):
                        node['type'] = [node['type'],]
                    if isinstance(inherits['type'], list):
                        node['type'].extend(inherits['type'])
                    else:
                        node['type'].append(inherits['type'])

                # type, title, description, version of inherited node
                # are overwritten by intention. Remove to prevent dct_merge to
                # complain about duplicates.
                inherits.pop('type', None)
                inherits.pop('title', None)
                inherits.pop('version', None)
                inherits.pop('description', None)
                cls.dict_merge(inherits, node)
                node = inherits
        return node

    @classmethod
    def _collapse_inherited(cls, bindings_list):
        collapsed = dict(bindings_list)

        for k, v in collapsed.items():
            v = cls._traverse_inherited(v)
            collapsed[k]=v

        return collapsed

    ##
    # @brief Get bindings for given compatibles.
    #
    # @param compatibles
    # @param bindings_paths directories to search for binding files
    # @return dictionary of bindings found
    @classmethod
    def bindings(cls, compatibles, bindings_paths):
        # find unique set of compatibles across all active nodes
        s = set()
        for k, v in compatibles.items():
            if isinstance(v, list):
                for item in v:
                    s.add(item)
            else:
                s.add(v)

        # scan YAML files and find the ones we are interested in
        # We add our own bindings directory last (lowest priority)
        # We only allow one binding file with the same name
        bindings_paths.append(Path(Path(__file__).resolve().parent,
                                   'bindings'))
        cls._files = []
        binding_files = []
        for path in bindings_paths:
            for root, dirnames, filenames in os.walk(str(path)):
                for filename in fnmatch.filter(filenames, '*.yaml'):
                    if not filename in binding_files:
                        binding_files.append(filename)
                        cls._files.append(os.path.join(root, filename))

        bindings_list = {}
        file_load_list = set()
        for file in cls._files:
            for line in open(file, 'r', encoding='utf-8'):
                if re.search('^\s+constraint:*', line):
                    c = line.split(':')[1].strip()
                    c = c.strip('"')
                    if c in s:
                        if file not in file_load_list:
                            file_load_list.add(file)
                            with open(file, 'r', encoding='utf-8') as yf:
                                cls._included = []
                                bindings_list[c] = yaml.load(yf, cls)

        # collapse the bindings inherited information before return
        return cls._collapse_inherited(bindings_list)

    def __init__(self, stream):
        filepath = os.path.realpath(stream.name)
        if filepath in self._included:
            print("Error:: circular inclusion for file name '{}'".
                  format(stream.name))
            raise yaml.constructor.ConstructorError
        self._included.append(filepath)
        super(Binder, self).__init__(stream)
        Binder.add_constructor('!include', Binder._include)
        Binder.add_constructor('!import',  Binder._include)

    def _include(self, node):
        if isinstance(node, yaml.ScalarNode):
            return self._extract_file(self.construct_scalar(node))

        elif isinstance(node, yaml.SequenceNode):
            result = []
            for filename in self.construct_sequence(node):
                result.append(self._extract_file(filename))
            return result

        elif isinstance(node, yaml.MappingNode):
            result = {}
            for k, v in self.construct_mapping(node).iteritems():
                result[k] = self._extract_file(v)
            return result

        else:
            print("Error: unrecognised node type in !include statement")
            raise yaml.constructor.ConstructorError

    def _extract_file(self, filename):
        filepaths = [filepath for filepath in self._files if filepath.endswith(filename)]
        if len(filepaths) == 0:
            print("Error: unknown file name '{}' in !include statement".
                  format(filename))
            raise yaml.constructor.ConstructorError
        elif len(filepaths) > 1:
            # multiple candidates for filename
            files = []
            for filepath in filepaths:
                if os.path.basename(filename) == os.path.basename(filepath):
                    files.append(filepath)
            if len(files) > 1:
                print("Error: multiple candidates for file name '{}' in !include statement".
                      format(filename), filepaths)
                raise yaml.constructor.ConstructorError
            filepaths = files
        with open(filepaths[0], 'r', encoding='utf-8') as f:
            return yaml.load(f, Binder)
