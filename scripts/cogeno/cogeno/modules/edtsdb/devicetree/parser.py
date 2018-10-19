#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from collections import defaultdict
from pathlib import Path

##
# @brief DTS parser
#
# Methods for device tree parsing.
#
class DTSParserMixin(object):
    __slots__ = []

    @staticmethod
    def read_until(line, fd, end):
        out = [line]
        while True:
            idx = line.find(end)
            if idx < 0:
                line = DTSParserMixin.clean_line(fd.readline(), fd)
                out.append(line)
            else:
                out.append(line[idx + len(end):])
                return out

    @staticmethod
    def remove_comment(line, fd):
        out = []
        while True:
            idx = line.find('/*')
            if idx < 0:
                idx = line.find('//')
                if idx < 0:
                    out.append(line)
                else:
                    out.append(line[:idx])
                return ' '.join(out)

            out.append(line[:idx])
            line = DTSParserMixin.read_until(line[idx:], fd, '*/')[-1]

    @staticmethod
    def clean_line(line, fd):
        return DTSParserMixin.remove_comment(line, fd).strip()

    @staticmethod
    def _parse_node_name(line):
        line = line[:-1]

        if '@' in line:
            line, addr = line.split('@')
        else:
            addr = None

        if ':' in line:
            if len(line.split(':')) == 3:
                alt_label, label, name = line.split(':')
            else:
                label, name = line.split(':')
                alt_label = None
        else:
            name = line
            label = None
            alt_label = None

        if addr is None:
            return label, name.strip(), None, None, None

        if alt_label is None:
            return label, name.strip(), addr, int(addr, 16), None

        return label, name.strip(), addr, int(addr, 16), alt_label

    @staticmethod
    def _parse_values_internal(value, start, end, separator):
        out = []

        inside = False
        accum = []
        for ch in value:
            if not inside:
                if ch == start:
                    inside = True
                    accum = []
            else:
                if ch == end:
                    inside = False
                    out.append(''.join(accum))
                    accum = []
                else:
                    accum.append(ch)

        if separator == ' ':
            out = [v.split() for v in out]

        if len(out) == 1:
            return DTSParserMixin._parse_value(out[0])

        return [DTSParserMixin._parse_value(v) for v in out]

    @staticmethod
    def _parse_values(value, start, end, separator):
        out = DTSParserMixin._parse_values_internal(value, start, end, separator)
        if isinstance(out, list) and \
        all(isinstance(v, str) and len(v) == 1 and not v.isalpha() for v in out):
            return bytearray(out)

        return out

    @staticmethod
    def _parse_value(value):
        if value == '':
            return value

        if isinstance(value, list):
            out = [DTSParserMixin._parse_value(v) for v in value]
            return out[0] if len(out) == 1 else out

        if value[0] == '<':
            return DTSParserMixin._parse_values(value, '<', '>', ' ')
        if value[0] == '"':
            return DTSParserMixin._parse_values(value, '"', '"', ',')
        if value[0] == '[':
            return DTSParserMixin._parse_values(value, '[', ']', ' ')

        if value[0] == '&':
            return {'ref': value[1:]}

        if value[0].isdigit():
            if value.startswith("0x"):
                return int(value, 16)
            if value[0] == '0':
                return int(value, 8)
            return int(value, 10)

        return value

    @staticmethod
    def _parse_property(property, fd):
        if '=' in property:
            key, value = property.split('=', 1)
            value = ' '.join(DTSParserMixin.read_until(value, fd, ';')).strip()
            if not value.endswith(';'):
                raise SyntaxError("parse_property: missing semicolon: %s" % value)
            return key.strip(), DTSParserMixin._parse_value(value[:-1])

        property = property.strip()
        if not property.endswith(';'):
            raise SyntaxError("parse_property: missing semicolon: %s" % property)

        return property[:-1].strip(), True

    @staticmethod
    def _parse_build_node_name(name, addr):
        if addr is None:
            return name
        elif isinstance(addr, int):
            return '%s@%x' % (name, addr)

        return '%s@%s' % (name, addr.strip())

    @staticmethod
    def _parse_node(line, fd):
        label, name, addr, numeric_addr, alt_label = DTSParserMixin._parse_node_name(line)

        node = {
            'label': label,
            'type': type,
            'addr': numeric_addr,
            'children': {},
            'props': {},
            'name': DTSParserMixin._parse_build_node_name(name, addr)
        }
        if alt_label:
            node['alt_name'] = alt_label

        while True:
            line = fd.readline()
            if not line:
                raise SyntaxError("parse_node: Missing } while parsing node")

            line = DTSParserMixin.clean_line(line, fd)
            if not line:
                continue

            if line == "};":
                break

            if line.endswith('{'):
                new_node = DTSParserMixin._parse_node(line, fd)
                node['children'][new_node['name']] = new_node
            else:
                key, value = DTSParserMixin._parse_property(line, fd)
                node['props'][key] = value

        return node

    ##
    # Parse compiled DTS file
    def _parse_file(self, fd, ignore_dts_version=False):
        has_v1_tag = False
        while True:
            line = fd.readline()
            if not line:
                break

            line = DTSParserMixin.clean_line(line, fd)
            if not line:
                continue

            if line.startswith('/include/ '):
                tag, filename = line.split()
                with open(filename.strip()[1:-1], "r") as new_fd:
                    self._parse_file(new_fd, True)
            elif line == '/dts-v1/;':
                has_v1_tag = True
            elif line.startswith('/memreserve/ ') and line.endswith(';'):
                tag, start, end = line.split()
                start = int(start, 16)
                end = int(end[:-1], 16)
                label = "reserved_memory_0x%x_0x%x" % (start, end)
                self._dts[label] = {
                    'type': 'memory',
                    'reg': [start, end],
                    'label': label,
                    'addr': start,
                    'name': DTSParserMixin._parse_build_node_name(name, start)
                }
            elif line.endswith('{'):
                if not has_v1_tag and not ignore_dts_version:
                    raise SyntaxError("parse_file: Missing /dts-v1/ tag")

                new_node = DTSParserMixin._parse_node(line, fd)
                self._dts[new_node['name']] = new_node
            else:
                raise SyntaxError("parse_file: Couldn't understand the line: %s" % line)

    def parse(self, dts_file_path):
        self._dts = {}
        self.compatibles = {}
        self.phandles = {}
        self.aliases = defaultdict(list)
        self.chosen = {}
        self.reduced = {}

        # load and parse DTS file
        with Path(dts_file_path).open(mode="r", encoding="utf-8") as dts_fd:
            self._parse_file(dts_fd)

        # build up useful lists - reduced first as it used by the build of others
        self._init_reduced(self._dts['/'])
        self._init_phandles(self._dts['/'])
        self._init_aliases(self._dts['/'])
        self._init_chosen(self._dts['/'])
        self._init_compatibles(self._dts['/'])
