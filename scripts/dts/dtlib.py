# Copyright (c) 2019, Nordic Semiconductor
# SPDX-License-Identifier: BSD-3-Clause

# Tip: You can view just the documentation with 'pydoc3 dtlib'

# _init_tokens() builds names dynamically.
#
# pylint: disable=undefined-variable

"""
A library for extracting information from .dts (devicetree) files. See the
documentation for the DT and Node classes for more information.

The top-level entry point of the library is the DT class. DT.__init__() takes a
.dts file to parse and a list of directories to search for any /include/d
files.
"""

import collections
import errno
import os
import re
import sys
import textwrap

# NOTE: testdtlib.py is the test suite for this library.

class DT:
    """
    Represents a devicetree parsed from a .dts file (or from many files, if the
    .dts file /include/s other files). Creating many instances of this class is
    fine. The library has no global state.

    These attributes are available on DT instances:

    root:
      A Node instance representing the root (/) node.

    alias2node:
      A dictionary that maps maps alias strings (from /aliases) to Node
      instances

    label2node:
      A dictionary that maps each node label (a string) to the Node instance
      for the node.

    label2prop:
      A dictionary that maps each property label (a string) to a Property
      instance.

    label2prop_offset:
      A dictionary that maps each label (a string) within a property value
      (e.g., 'x = label_1: < 1 label2: 2 >;') to a (prop, offset) tuple, where
      'prop' is a Property instance and 'offset' the byte offset (0 for label_1
      and 4 for label_2 in the example).

    phandle2node:
      A dictionary that maps each phandle (a number) to a Node instance.

    memreserves:
      A list of (labels, address, length) tuples for the /memreserve/s in the
      .dts file, in the same order as they appear in the file.

      'labels' is a possibly empty set with all labels preceding the memreserve
      (e.g., 'label1: label2: /memreserve/ ...'). 'address' and 'length' are
      numbers.

    filename:
      The filename passed to the DT constructor.
    """

    #
    # Public interface
    #

    def __init__(self, filename, include_path=()):
        """
        Parses a DTS file to create a DT instance. Raises OSError if 'filename'
        can't be opened, and DTError for any parse errors.

        filename:
          Path to the .dts file to parse.

        include_path:
          An iterable (e.g. list or tuple) containing paths to search for
          /include/d and /incbin/'d files. By default, files are only looked up
          relative to the .dts file that contains the /include/ or /incbin/.
        """
        self.filename = filename
        self._include_path = include_path

        with open(filename, encoding="utf-8") as f:
            self._file_contents = f.read()

        self._tok_i = self._tok_end_i = 0
        self._filestack = []

        self.alias2node = {}

        self._lexer_state = _DEFAULT
        self._saved_token = None

        self._lineno = 1

        self._parse_dt()

        self._register_phandles()
        self._fixup_props()
        self._register_aliases()
        self._remove_unreferenced()
        self._register_labels()

    def get_node(self, path):
        """
        Returns the Node instance for the node with path or alias 'path' (a
        string). Raises DTError if the path or alias doesn't exist.

        For example, both dt.get_node("/foo/bar") and dt.get_node("bar-alias")
        will return the 'bar' node below:

          /dts-v1/;

          / {
                  foo {
                          bar_label: bar {
                                  baz {
                                  };
                          };
                  };

                  aliases {
                          bar-alias = &bar-label;
                  };
          };

        Fetching subnodes via aliases is supported:
        dt.get_node("bar-alias/baz") returns the 'baz' node.
        """
        if path.startswith("/"):
            return _root_and_path_to_node(self.root, path, path)

        # Path does not start with '/'. First component must be an alias.
        alias, _, rest = path.partition("/")
        if alias not in self.alias2node:
            _err("no alias '{}' found -- did you forget the leading '/' in "
                 "the node path?".format(alias))

        return _root_and_path_to_node(self.alias2node[alias], rest, path)

    def has_node(self, path):
        """
        Returns True if the path or alias 'path' exists. See Node.get_node().
        """
        try:
            self.get_node(path)
            return True
        except DTError:
            return False

    def node_iter(self):
        """
        Returns a generator for iterating over all nodes in the devicetree.

        For example, this will print the name of each node that has a property
        called 'foo':

          for node in dt.node_iter():
              if "foo" in node.props:
                  print(node.name)
        """
        yield from self.root.node_iter()

    def __str__(self):
        """
        Returns a DTS representation of the devicetree. Called automatically if
        the DT instance is print()ed.
        """
        s = "/dts-v1/;\n\n"

        if self.memreserves:
            for labels, address, offset in self.memreserves:
                # List the labels in a consistent order to help with testing
                for label in labels:
                    s += label + ": "
                s += "/memreserve/ {:#018x} {:#018x};\n" \
                     .format(address, offset)
            s += "\n"

        return s + str(self.root)

    def __repr__(self):
        """
        Returns some information about the DT instance. Called automatically if
        the DT instance is evaluated.
        """
        return "DT(filename='{}', include_path={})" \
               .format(self.filename, self._include_path)

    #
    # Parsing
    #

    def _parse_dt(self):
        # Top-level parsing loop

        self._parse_header()
        self._parse_memreserves()

        self.root = None

        while True:
            tok = self._next_token()

            if tok.val == "/":
                # '/ { ... };', the root node
                if not self.root:
                    self.root = Node(name="/", parent=None, dt=self)
                self._parse_node(self.root)

            elif tok.id in (_T_LABEL, _T_REF):
                # '&foo { ... };' or 'label: &foo { ... };'. The C tools only
                # support a single label here too.

                if tok.id is _T_LABEL:
                    label = tok.val
                    tok = self._next_token()
                    if tok.id is not _T_REF:
                        self._parse_error("expected label reference (&foo)")
                else:
                    label = None

                try:
                    node = self._ref2node(tok.val)
                except DTError as e:
                    self._parse_error(e)
                node = self._parse_node(node)

                if label:
                    _append_no_dup(node.labels, label)

            elif tok.id is _T_DEL_NODE:
                self._next_ref2node()._del()
                self._expect_token(";")

            elif tok.id is _T_OMIT_IF_NO_REF:
                self._next_ref2node()._omit_if_no_ref = True
                self._expect_token(";")

            elif tok.id is _T_EOF:
                if not self.root:
                    self._parse_error("no root node defined")
                return

            else:
                self._parse_error("expected '/' or label reference (&foo)")

    def _parse_header(self):
        # Parses /dts-v1/ (expected) and /plugin/ (unsupported) at the start of
        # files. There may be multiple /dts-v1/ at the start of a file.

        has_dts_v1 = False

        while self._peek_token().id is _T_DTS_V1:
            has_dts_v1 = True
            self._next_token()
            self._expect_token(";")
            # /plugin/ always comes after /dts-v1/
            if self._peek_token().id is _T_PLUGIN:
                self._parse_error("/plugin/ is not supported")

        if not has_dts_v1:
            self._parse_error("expected '/dts-v1/;' at start of file")

    def _parse_memreserves(self):
        # Parses /memreserve/, which appears after /dts-v1/

        self.memreserves = []
        while True:
            # Labels before /memreserve/
            labels = []
            while self._peek_token().id is _T_LABEL:
                _append_no_dup(labels, self._next_token().val)

            if self._peek_token().id is _T_MEMRESERVE:
                self._next_token()
                self.memreserves.append(
                    (labels, self._eval_prim(), self._eval_prim()))
                self._expect_token(";")
            elif labels:
                self._parse_error("expected /memreserve/ after labels at "
                                  "beginning of file")
            else:
                return

    def _parse_node(self, node):
        # Parses the '{ ... };' part of 'node-name { ... };'. Returns the new
        # Node.

        self._expect_token("{")
        while True:
            labels, omit_if_no_ref = self._parse_propnode_labels()
            tok = self._next_token()

            if tok.id is _T_PROPNODENAME:
                if self._peek_token().val == "{":
                    # '<tok> { ...', expect node

                    if tok.val.count("@") > 1:
                        self._parse_error("multiple '@' in node name")

                    # Fetch the existing node if it already exists. This
                    # happens when overriding nodes.
                    child = node.nodes.get(tok.val) or \
                        Node(name=tok.val, parent=node, dt=self)

                    for label in labels:
                        _append_no_dup(child.labels, label)

                    if omit_if_no_ref:
                        child._omit_if_no_ref = True

                    node.nodes[child.name] = child
                    self._parse_node(child)

                else:
                    # Not '<tok> { ...', expect property assignment

                    if omit_if_no_ref:
                        self._parse_error(
                            "/omit-if-no-ref/ can only be used on nodes")

                    prop = node._get_prop(tok.val)

                    if self._check_token("="):
                        self._parse_assignment(prop)
                    elif not self._check_token(";"):
                        # ';' is for an empty property, like 'foo;'
                        self._parse_error("expected '{', '=', or ';'")

                    for label in labels:
                        _append_no_dup(prop.labels, label)

            elif tok.id is _T_DEL_NODE:
                tok2 = self._next_token()
                if tok2.id is not _T_PROPNODENAME:
                    self._parse_error("expected node name")
                if tok2.val in node.nodes:
                    node.nodes[tok2.val]._del()
                self._expect_token(";")

            elif tok.id is _T_DEL_PROP:
                tok2 = self._next_token()
                if tok2.id is not _T_PROPNODENAME:
                    self._parse_error("expected property name")
                node.props.pop(tok2.val, None)
                self._expect_token(";")

            elif tok.val == "}":
                self._expect_token(";")
                return node

            else:
                self._parse_error("expected node name, property name, or '}'")

    def _parse_propnode_labels(self):
        # _parse_node() helpers for parsing labels and /omit-if-no-ref/s before
        # nodes and properties. Returns a (<label list>, <omit-if-no-ref bool>)
        # tuple.

        labels = []
        omit_if_no_ref = False
        while True:
            tok = self._peek_token()
            if tok.id is _T_LABEL:
                _append_no_dup(labels, tok.val)
            elif tok.id is _T_OMIT_IF_NO_REF:
                omit_if_no_ref = True
            elif (labels or omit_if_no_ref) and tok.id is not _T_PROPNODENAME:
                # Got something like 'foo: bar: }'
                self._parse_error("expected node or property name")
            else:
                return labels, omit_if_no_ref

            self._next_token()

    def _parse_assignment(self, prop):
        # Parses the right-hand side of property assignment
        #
        # prop:
        #   'Property' instance being assigned

        # Remove any old value, path/phandle references, and in-value labels,
        # in case the property value is being overridden
        prop.value = b""
        prop._markers = []

        while True:
            # Parse labels before the value (e.g., '..., label: < 0 >')
            self._parse_value_labels(prop)

            tok = self._next_token()

            if tok.val == "<":
                self._parse_cells(prop, 4)

            elif tok.id is _T_BITS:
                n_bits = self._expect_num()
                if n_bits not in {8, 16, 32, 64}:
                    self._parse_error("expected 8, 16, 32, or 64")
                self._expect_token("<")
                self._parse_cells(prop, n_bits//8)

            elif tok.val == "[":
                self._parse_bytes(prop)

            elif tok.id is _T_STRING:
                prop._add_marker(_TYPE_STRING)
                prop.value += self._unescape(tok.val.encode("utf-8")) + b"\0"

            elif tok.id is _T_REF:
                prop._add_marker(_REF_PATH, tok.val)

            elif tok.id is _T_INCBIN:
                self._parse_incbin(prop)

            else:
                self._parse_error("malformed value")

            # Parse labels after the value (e.g., '< 0 > label:, ...')
            self._parse_value_labels(prop)

            tok = self._next_token()
            if tok.val == ";":
                return
            if tok.val == ",":
                continue
            self._parse_error("expected ';' or ','")

    def _parse_cells(self, prop, n_bytes):
        # Parses '<...>'

        prop._add_marker(_N_BYTES_TO_TYPE[n_bytes])

        while True:
            tok = self._peek_token()
            if tok.id is _T_REF:
                self._next_token()
                if n_bytes != 4:
                    self._parse_error("phandle references are only allowed in "
                                      "arrays with 32-bit elements")
                prop._add_marker(_REF_PHANDLE, tok.val)

            elif tok.id is _T_LABEL:
                prop._add_marker(_REF_LABEL, tok.val)
                self._next_token()

            elif self._check_token(">"):
                return

            else:
                # Literal value
                num = self._eval_prim()
                try:
                    prop.value += num.to_bytes(n_bytes, "big")
                except OverflowError:
                    try:
                        # Try again as a signed number, in case it's negative
                        prop.value += num.to_bytes(n_bytes, "big", signed=True)
                    except OverflowError:
                        self._parse_error("{} does not fit in {} bits"
                                          .format(num, 8*n_bytes))

    def _parse_bytes(self, prop):
        # Parses '[ ... ]'

        prop._add_marker(_TYPE_UINT8)

        while True:
            tok = self._next_token()
            if tok.id is _T_BYTE:
                prop.value += tok.val.to_bytes(1, "big")

            elif tok.id is _T_LABEL:
                prop._add_marker(_REF_LABEL, tok.val)

            elif tok.val == "]":
                return

            else:
                self._parse_error("expected two-digit byte or ']'")

    def _parse_incbin(self, prop):
        # Parses
        #
        #   /incbin/ ("filename")
        #
        # and
        #
        #   /incbin/ ("filename", <offset>, <size>)

        prop._add_marker(_TYPE_UINT8)

        self._expect_token("(")

        tok = self._next_token()
        if tok.id is not _T_STRING:
            self._parse_error("expected quoted filename")
        filename = tok.val

        tok = self._next_token()
        if tok.val == ",":
            offset = self._eval_prim()
            self._expect_token(",")
            size = self._eval_prim()
            self._expect_token(")")
        else:
            if tok.val != ")":
                self._parse_error("expected ',' or ')'")
            offset = None

        try:
            with self._open(filename, "rb") as f:
                if offset is None:
                    prop.value += f.read()
                else:
                    f.seek(offset)
                    prop.value += f.read(size)
        except OSError as e:
            self._parse_error("could not read '{}': {}"
                              .format(filename, e))

    def _parse_value_labels(self, prop):
        # _parse_assignment() helper for parsing labels before/after each
        # comma-separated value

        while True:
            tok = self._peek_token()
            if tok.id is not _T_LABEL:
                return
            prop._add_marker(_REF_LABEL, tok.val)
            self._next_token()

    def _node_phandle(self, node):
        # Returns the phandle for Node 'node', creating a new phandle if the
        # node has no phandle, and fixing up the value for existing
        # self-referential phandles (which get set to b'\0\0\0\0' initially).
        # Self-referential phandles must be rewritten instead of recreated, so
        # that labels are preserved.

        if "phandle" in node.props:
            phandle_prop = node.props["phandle"]
        else:
            phandle_prop = Property(node, "phandle")
            phandle_prop._add_marker(_TYPE_UINT32)  # For displaying
            phandle_prop.value = b'\0\0\0\0'

        if phandle_prop.value == b'\0\0\0\0':
            phandle_i = 1
            while phandle_i in self.phandle2node:
                phandle_i += 1
            self.phandle2node[phandle_i] = node

            phandle_prop.value = phandle_i.to_bytes(4, "big")
            node.props["phandle"] = phandle_prop

        return phandle_prop.value

    # Expression evaluation

    def _eval_prim(self):
        tok = self._peek_token()
        if tok.id in (_T_NUM, _T_CHAR_LITERAL):
            return self._next_token().val

        tok = self._next_token()
        if tok.val != "(":
            self._parse_error("expected number or parenthesized expression")
        val = self._eval_ternary()
        self._expect_token(")")
        return val

    def _eval_ternary(self):
        val = self._eval_or()
        if self._check_token("?"):
            if_val = self._eval_ternary()
            self._expect_token(":")
            else_val = self._eval_ternary()
            return if_val if val else else_val
        return val

    def _eval_or(self):
        val = self._eval_and()
        while self._check_token("||"):
            val = 1 if self._eval_and() or val else 0
        return val

    def _eval_and(self):
        val = self._eval_bitor()
        while self._check_token("&&"):
            val = 1 if self._eval_bitor() and val else 0
        return val

    def _eval_bitor(self):
        val = self._eval_bitxor()
        while self._check_token("|"):
            val |= self._eval_bitxor()
        return val

    def _eval_bitxor(self):
        val = self._eval_bitand()
        while self._check_token("^"):
            val ^= self._eval_bitand()
        return val

    def _eval_bitand(self):
        val = self._eval_eq()
        while self._check_token("&"):
            val &= self._eval_eq()
        return val

    def _eval_eq(self):
        val = self._eval_rela()
        while True:
            if self._check_token("=="):
                val = 1 if val == self._eval_rela() else 0
            elif self._check_token("!="):
                val = 1 if val != self._eval_rela() else 0
            else:
                return val

    def _eval_rela(self):
        val = self._eval_shift()
        while True:
            if self._check_token("<"):
                val = 1 if val < self._eval_shift() else 0
            elif self._check_token(">"):
                val = 1 if val > self._eval_shift() else 0
            elif self._check_token("<="):
                val = 1 if val <= self._eval_shift() else 0
            elif self._check_token(">="):
                val = 1 if val >= self._eval_shift() else 0
            else:
                return val

    def _eval_shift(self):
        val = self._eval_add()
        while True:
            if self._check_token("<<"):
                val <<= self._eval_add()
            elif self._check_token(">>"):
                val >>= self._eval_add()
            else:
                return val

    def _eval_add(self):
        val = self._eval_mul()
        while True:
            if self._check_token("+"):
                val += self._eval_mul()
            elif self._check_token("-"):
                val -= self._eval_mul()
            else:
                return val

    def _eval_mul(self):
        val = self._eval_unary()
        while True:
            if self._check_token("*"):
                val *= self._eval_unary()
            elif self._check_token("/"):
                denom = self._eval_unary()
                if not denom:
                    self._parse_error("division by zero")
                val //= denom
            elif self._check_token("%"):
                denom = self._eval_unary()
                if not denom:
                    self._parse_error("division by zero")
                val %= denom
            else:
                return val

    def _eval_unary(self):
        if self._check_token("-"):
            return -self._eval_unary()
        if self._check_token("~"):
            return ~self._eval_unary()
        if self._check_token("!"):
            return 0 if self._eval_unary() else 1
        return self._eval_prim()

    #
    # Lexing
    #

    def _check_token(self, val):
        if self._peek_token().val == val:
            self._next_token()
            return True
        return False

    def _peek_token(self):
        if not self._saved_token:
            self._saved_token = self._next_token()
        return self._saved_token

    def _next_token(self):
        if self._saved_token:
            tmp = self._saved_token
            self._saved_token = None
            return tmp

        while True:
            tok_id = None

            match = _token_re.match(self._file_contents, self._tok_end_i)
            if match:
                tok_id = match.lastindex
                if tok_id is _T_CHAR_LITERAL:
                    val = self._unescape(match.group(tok_id).encode("utf-8"))
                    if len(val) != 1:
                        self._parse_error("character literals must be length 1")
                    tok_val = ord(val)
                else:
                    tok_val = match.group(tok_id)

            elif self._lexer_state is _DEFAULT:
                match = _num_re.match(self._file_contents, self._tok_end_i)
                if match:
                    tok_id = _T_NUM
                    num_s = match.group(1)
                    tok_val = int(num_s,
                                  16 if num_s.startswith(("0x", "0X")) else
                                  8 if num_s[0] == "0" else
                                  10)

            elif self._lexer_state is _EXPECT_PROPNODENAME:
                match = _propnodename_re.match(self._file_contents,
                                               self._tok_end_i)
                if match:
                    tok_id = _T_PROPNODENAME
                    tok_val = match.group(1)
                    self._lexer_state = _DEFAULT

            else:  # self._lexer_state is _EXPECT_BYTE
                match = _byte_re.match(self._file_contents, self._tok_end_i)
                if match:
                    tok_id = _T_BYTE
                    tok_val = int(match.group(), 16)


            if not tok_id:
                match = _misc_re.match(self._file_contents, self._tok_end_i)
                if match:
                    tok_id = _T_MISC
                    tok_val = match.group()
                else:
                    self._tok_i = self._tok_end_i
                    # Could get here due to a node/property naming appearing in
                    # an unexpected context as well as for bad characters in
                    # files. Generate a token for it so that the error can
                    # trickle up to some context where we can give a more
                    # helpful error message.
                    return _Token(_T_BAD, "<unknown token>")

            self._tok_i = match.start()
            self._tok_end_i = match.end()

            if tok_id is _T_SKIP:
                self._lineno += tok_val.count("\n")
                continue

            # /include/ is handled in the lexer in the C tools as well, and can
            # appear anywhere
            if tok_id is _T_INCLUDE:
                # Can have newlines between /include/ and the filename
                self._lineno += tok_val.count("\n")
                # Do this manual extraction instead of doing it in the regex so
                # that we can properly count newlines
                filename = tok_val[tok_val.find('"') + 1:-1]
                self._enter_file(filename)
                continue

            if tok_id is _T_LINE:
                # #line directive
                self._lineno = int(tok_val.split()[0]) - 1
                self.filename = tok_val[tok_val.find('"') + 1:-1]
                continue

            if tok_id is _T_EOF:
                if self._filestack:
                    self._leave_file()
                    continue
                return _Token(_T_EOF, "<EOF>")

            # State handling

            if tok_id in (_T_DEL_PROP, _T_DEL_NODE, _T_OMIT_IF_NO_REF) or \
               tok_val in ("{", ";"):

                self._lexer_state = _EXPECT_PROPNODENAME

            elif tok_val == "[":
                self._lexer_state = _EXPECT_BYTE

            elif tok_id in (_T_MEMRESERVE, _T_BITS) or tok_val == "]":
                self._lexer_state = _DEFAULT

            return _Token(tok_id, tok_val)

    def _expect_token(self, tok_val):
        # Raises an error if the next token does not have the string value
        # 'tok_val'. Returns the token.

        tok = self._next_token()
        if tok.val != tok_val:
            self._parse_error("expected '{}', not '{}'"
                              .format(tok_val, tok.val))

        return tok

    def _expect_num(self):
        # Raises an error if the next token is not a number. Returns the token.

        tok = self._next_token()
        if tok.id is not _T_NUM:
            self._parse_error("expected number")
        return tok.val

    def _parse_error(self, s):
        _err("{}:{} (column {}): parse error: {}".format(
            self.filename, self._lineno,
            # This works out for the first line of the file too, where rfind()
            # returns -1
            self._tok_i - self._file_contents.rfind("\n", 0, self._tok_i + 1),
            s))

    def _enter_file(self, filename):
        # Enters the /include/d file 'filename', remembering the position in
        # the /include/ing file for later

        self._filestack.append((self.filename, self._lineno,
                                self._file_contents, self._tok_end_i))

        # Handle escapes in filenames, just for completeness
        filename = self._unescape(filename.encode("utf-8"))
        try:
            filename = filename.decode("utf-8")
        except UnicodeDecodeError:
            self._parse_error("filename is not valid UTF-8")

        with self._open(filename, encoding="utf-8") as f:
            try:
                self._file_contents = f.read()
            except OSError as e:
                self._parse_error(e)

        # Check for recursive /include/
        for i, parent in enumerate(self._filestack):
            if filename == parent[0]:
                self._parse_error("recursive /include/:\n" + " ->\n".join(
                    ["{}:{}".format(parent[0], parent[1])
                        for parent in self._filestack[i:]] +
                    [filename]))

        self.filename = f.name
        self._lineno = 1
        self._tok_end_i = 0

    def _leave_file(self):
        # Leaves an /include/d file, returning to the file that /include/d it

        self.filename, self._lineno, self._file_contents, self._tok_end_i = \
            self._filestack.pop()

    def _next_ref2node(self):
        # Checks that the next token is a label/path reference and returns the
        # Node it points to. Only used during parsing, so uses _parse_error()
        # on errors to save some code in callers.

        label = self._next_token()
        if label.id is not _T_REF:
            self._parse_error(
                "expected label (&foo) or path (&{/foo/bar}) reference")
        try:
            return self._ref2node(label.val)
        except DTError as e:
            self._parse_error(e)

    def _ref2node(self, s):
        # Returns the Node the label/path reference 's' points to

        if s[0] == "{":
            # Path reference (&{/foo/bar})
            path = s[1:-1]
            if not path.startswith("/"):
                _err("node path '{}' does not start with '/'".format(path))
            # Will raise DTError if the path doesn't exist
            return _root_and_path_to_node(self.root, path, path)

        # Label reference (&foo).

        # label2node hasn't been filled in yet, and using it would get messy
        # when nodes are deleted
        for node in self.node_iter():
            if s in node.labels:
                return node

        _err("undefined node label '{}'".format(s))

    #
    # Post-processing
    #

    def _register_phandles(self):
        # Registers any manually-inserted phandle properties in
        # self.phandle2node, so that we can avoid allocating any phandles from
        # that set. Also checks the format of the phandles and does misc.
        # sanity checking.

        self.phandle2node = {}
        for node in self.node_iter():
            phandle = node.props.get("phandle")
            if phandle:
                if len(phandle.value) != 4:
                    _err("{}: bad phandle length ({}), expected 4 bytes"
                         .format(node.path, len(phandle.value)))

                is_self_referential = False
                for marker in phandle._markers:
                    _, marker_type, ref = marker
                    if marker_type is _REF_PHANDLE:
                        # The phandle's value is itself a phandle reference
                        if self._ref2node(ref) is node:
                            # Alright to set a node's phandle equal to its own
                            # phandle. It'll force a new phandle to be
                            # allocated even if the node is otherwise
                            # unreferenced.
                            is_self_referential = True
                            break

                        _err("{}: {} refers to another node"
                             .format(node.path, phandle.name))

                # Could put on else on the 'for' above too, but keep it
                # somewhat readable
                if not is_self_referential:
                    phandle_val = int.from_bytes(phandle.value, "big")

                    if phandle_val in {0, 0xFFFFFFFF}:
                        _err("{}: bad value {:#010x} for {}"
                             .format(node.path, phandle_val, phandle.name))

                    if phandle_val in self.phandle2node:
                        _err("{}: duplicated phandle {:#x} (seen before at {})"
                             .format(node.path, phandle_val,
                                     self.phandle2node[phandle_val].path))

                    self.phandle2node[phandle_val] = node

    def _fixup_props(self):
        # Fills in node path and phandle references in property values, and
        # registers labels within values. This must be done after parsing,
        # since forwards references are allowed and nodes and properties might
        # be deleted.

        for node in self.node_iter():
            # The tuple() avoids a 'dictionary changed size during iteration'
            # error
            for prop in tuple(node.props.values()):
                # 'prev_pos' and 'pos' are indices in the unpatched
                # property value. The result is built up in 'res'.
                prev_pos = 0
                res = b""

                for marker in prop._markers:
                    pos, marker_type, ref = marker

                    # Add data before the marker, reading from the unpatched
                    # property value
                    res += prop.value[prev_pos:pos]

                    # Fix the marker offset so that it's correct for the
                    # patched property value, for later (not used in this
                    # function). The offset might change due to path
                    # references, which expand to something like "/foo/bar".
                    marker[0] = len(res)

                    if marker_type is _REF_LABEL:
                        # This is a temporary format so that we can catch
                        # duplicate references. prop.offset_labels is changed
                        # to a dictionary that maps labels to offsets in
                        # _register_labels().
                        _append_no_dup(prop.offset_labels, (ref, len(res)))
                    elif marker_type in (_REF_PATH, _REF_PHANDLE):
                        # Path or phandle reference
                        try:
                            ref_node = self._ref2node(ref)
                        except DTError as e:
                            _err("{}: {}".format(prop.node.path, e))

                        # For /omit-if-no-ref/
                        ref_node._is_referenced = True

                        if marker_type is _REF_PATH:
                            res += ref_node.path.encode("utf-8") + b'\0'
                        else:  # marker_type is PHANDLE
                            res += self._node_phandle(ref_node)
                            # Skip over the dummy phandle placeholder
                            pos += 4

                    prev_pos = pos

                # Store the final fixed-up value. Add the data after the last
                # marker.
                prop.value = res + prop.value[prev_pos:]

    def _register_aliases(self):
        # Registers aliases from the /aliases node in self.alias2node. Also
        # checks the format of the alias properties.

        # We copy this to self.alias2node at the end to avoid get_node()
        # looking up paths via other aliases while verifying aliases
        alias2node = {}

        alias_re = re.compile("[0-9a-z-]+$")

        aliases = self.root.nodes.get("aliases")
        if aliases:
            for prop in aliases.props.values():
                if not alias_re.match(prop.name):
                    _err("/aliases: alias property name '{}' should include "
                         "only characters from [0-9a-z-]".format(prop.name))

                # Property.to_path() already checks that the node exists
                alias2node[prop.name] = prop.to_path()

        self.alias2node = alias2node

    def _remove_unreferenced(self):
        # Removes any unreferenced nodes marked with /omit-if-no-ref/ from the
        # tree

        # tuple() is to avoid 'RuntimeError: dictionary changed size during
        # iteration' errors
        for node in tuple(self.node_iter()):
            if node._omit_if_no_ref and not node._is_referenced:
                node._del()

    def _register_labels(self):
        # Checks for duplicate labels and registers labels in label2node,
        # label2prop, and label2prop_offset

        label2things = collections.defaultdict(set)

        self.label2node = {}
        self.label2prop = {}
        self.label2prop_offset = {}

        # Register all labels and the nodes/props they point to in label2things
        for node in self.node_iter():
            for label in node.labels:
                label2things[label].add(node)
                self.label2node[label] = node

            for prop in node.props.values():
                for label in prop.labels:
                    label2things[label].add(prop)
                    self.label2prop[label] = prop

                for label, offset in prop.offset_labels:
                    label2things[label].add((prop, offset))
                    self.label2prop_offset[label] = (prop, offset)

                # See _fixup_props()
                prop.offset_labels = {label: offset for label, offset in
                                      prop.offset_labels}

        for label, things in label2things.items():
            if len(things) > 1:
                strings = []
                for thing in things:
                    if isinstance(thing, Node):
                        strings.append("on " + thing.path)
                    elif isinstance(thing, Property):
                        strings.append("on property '{}' of node {}"
                                       .format(thing.name, thing.node.path))
                    else:
                        # Label within property value
                        strings.append("in the value of property '{}' of node {}"
                                       .format(thing[0].name,
                                               thing[0].node.path))

                # Give consistent error messages to help with testing
                strings.sort()

                _err("Label '{}' appears ".format(label) +
                     " and ".join(strings))


    #
    # Misc.
    #

    def _unescape(self, b):
        # Replaces backslash escapes in the 'bytes' array 'b'. We can't do this at
        # the string level, because the result might not be valid UTF-8 when
        # octal/hex escapes are involved.

        def sub(match):
            esc = match.group(1)
            if esc == b"a": return b"\a"
            if esc == b"b": return b"\b"
            if esc == b"t": return b"\t"
            if esc == b"n": return b"\n"
            if esc == b"v": return b"\v"
            if esc == b"f": return b"\f"
            if esc == b"r": return b"\r"

            if esc[0] in b"01234567":
                # Octal escape
                try:
                    return int(esc, 8).to_bytes(1, "big")
                except OverflowError:
                    self._parse_error("octal escape out of range (> 255)")

            if esc[0] == ord("x") and len(esc) > 1:
                # Hex escape
                return int(esc[1:], 16).to_bytes(1, "big")

            # Return <char> as-is for other \<char>
            return esc[0].to_bytes(1, "big")

        return _unescape_re.sub(sub, b)

    def _open(self, filename, mode="r", **kwargs):
        # Wrapper around standard Python open(), accepting the same params.
        # But searches for a 'filename' file in the directory of the current
        # file and the include path.

        # The C tools support specifying stdin with '-' too
        if filename == "-":
            return sys.stdin.buffer if "b" in mode else sys.stdin

        # Try the directory of the current file first
        dirname = os.path.dirname(self.filename)
        try:
            return open(os.path.join(dirname, filename), mode, **kwargs)
        except OSError as e:
            if e.errno != errno.ENOENT:
                self._parse_error(e)

            # Try each directory from the include path
            for path in self._include_path:
                try:
                    return open(os.path.join(path, filename), mode, **kwargs)
                except OSError as e:
                    if e.errno != errno.ENOENT:
                        self._parse_error(e)
                    continue

            self._parse_error("'{}' could not be found".format(filename))


class Node:
    r"""
    Represents a node in the devicetree ('node-name { ... };').

    These attributes are available on Node instances:

    name:
      The name of the node (a string).

    unit_addr:
      The portion after the '@' in the node's name, or the empty string if the
      name has no '@' in it.

      Note that this is a string. Run int(node.unit_addr, 16) to get an
      integer.

    props:
      A collections.OrderedDict that maps the properties defined on the node to
      their values. 'props' is indexed by property name (a string), and values
      are represented as 'bytes' arrays.

      To convert property values to Python numbers or strings, use
      dtlib.to_num(), dtlib.to_nums(), or dtlib.to_string().

      Property values are represented as 'bytes' arrays to support the full
      generality of DTS, which allows assignments like

        x = "foo", < 0x12345678 >, [ 9A ];

      This gives x the value b"foo\0\x12\x34\x56\x78\x9A". Numbers in DTS are
      stored in big-endian format.

    nodes:
      A collections.OrderedDict containing the subnodes of the node, indexed by
      name.

    labels:
      A list with all labels pointing to the node, in the same order as the
      labels appear, but with duplicates removed.

      'label_1: label_2: node { ... };' gives 'labels' the value
      ["label_1", "label_2"].

    parent:
      The parent Node of the node. 'None' for the root node.

    path:
      The path to the node as a string, e.g. "/foo/bar".

    dt:
      The DT instance this node belongs to.
    """

    #
    # Public interface
    #

    def __init__(self, name, parent, dt):
        """
        Node constructor. Not meant to be called directly by clients.
        """
        self.name = name
        self.parent = parent
        self.dt = dt

        self.props = collections.OrderedDict()
        self.nodes = collections.OrderedDict()
        self.labels = []
        self._omit_if_no_ref = False
        self._is_referenced = False

    @property
    def unit_addr(self):
        """
        See the class documentation.
        """
        return self.name.partition("@")[2]

    @property
    def path(self):
        """
        See the class documentation.
        """
        node_names = []

        cur = self
        while cur.parent:
            node_names.append(cur.name)
            cur = cur.parent

        return "/" + "/".join(reversed(node_names))

    def node_iter(self):
        """
        Returns a generator for iterating over the node and its children,
        recursively.

        For example, this will iterate over all nodes in the tree (like
        dt.node_iter()).

          for node in dt.root.node_iter():
              ...
        """
        yield self
        for node in self.nodes.values():
            yield from node.node_iter()

    def _get_prop(self, name):
        # Returns the property named 'name' on the node, creating it if it
        # doesn't already exist

        prop = self.props.get(name)
        if not prop:
            prop = Property(self, name)
            self.props[name] = prop
        return prop

    def _del(self):
        # Removes the node from the tree

        self.parent.nodes.pop(self.name)

    def __str__(self):
        """
        Returns a DTS representation of the node. Called automatically if the
        node is print()ed.
        """
        s = "".join(label + ": " for label in self.labels)

        s += "{} {{\n".format(self.name)

        for prop in self.props.values():
            s += "\t" + str(prop) + "\n"

        for child in self.nodes.values():
            s += textwrap.indent(child.__str__(), "\t") + "\n"

        s += "};"

        return s

    def __repr__(self):
        """
        Returns some information about the Node instance. Called automatically
        if the Node instance is evaluated.
        """
        return "<Node {} in '{}'>" \
               .format(self.path, self.dt.filename)


class Property:
    """
    Represents a property ('x = ...').

    These attributes are available on Property instances:

    name:
      The name of the property (a string).

    value:
      The value of the property, as a 'bytes' string. Numbers are stored in
      big-endian format, and strings are null-terminated. Putting multiple
      comma-separated values in an assignment (e.g., 'x = < 1 >, "foo"') will
      concatenate the values.

      See the to_*() methods for converting the value to other types.

    type:
      The type of the property, inferred from the syntax used in the
      assignment. This is one of the following constants (with example
      assignments):

        Assignment                  | Property.type
        ----------------------------+------------------------
        foo;                        | dtlib.TYPE_EMPTY
        foo = [];                   | dtlib.TYPE_BYTES
        foo = [01 02];              | dtlib.TYPE_BYTES
        foo = /bits/ 8 <1>;         | dtlib.TYPE_BYTES
        foo = <1>;                  | dtlib.TYPE_NUM
        foo = <>;                   | dtlib.TYPE_NUMS
        foo = <1 2 3>;              | dtlib.TYPE_NUMS
        foo = <1 2>, <3>;           | dtlib.TYPE_NUMS
        foo = "foo";                | dtlib.TYPE_STRING
        foo = "foo", "bar";         | dtlib.TYPE_STRINGS
        foo = <&l>;                 | dtlib.TYPE_PHANDLE
        foo = <&l1 &l2 &l3>;        | dtlib.TYPE_PHANDLES
        foo = <&l1 &l2>, <&l3>;     | dtlib.TYPE_PHANDLES
        foo = <&l1 1 2 &l2 3 4>;    | dtlib.TYPE_PHANDLES_AND_NUMS
        foo = <&l1 1 2>, <&l2 3 4>; | dtlib.TYPE_PHANDLES_AND_NUMS
        foo = &l;                   | dtlib.TYPE_PATH
        *Anything else*             | dtlib.TYPE_COMPOUND

      *Anything else* includes properties mixing phandle (<&label>) and node
      path (&label) references with other data.

      Data labels in the property value do not influence the type.

    labels:
      A list with all labels pointing to the property, in the same order as the
      labels appear, but with duplicates removed.

      'label_1: label2: x = ...' gives 'labels' the value
      {"label_1", "label_2"}.

    offset_labels:
      A dictionary that maps any labels within the property's value to their
      offset, in bytes. For example, 'x = < 0 label_1: 1 label_2: >' gives
      'offset_labels' the value {"label_1": 4, "label_2": 8}.

      Iteration order will match the order of the labels on Python versions
      that preserve dict insertion order.

    node:
      The Node the property is on.
    """

    #
    # Public interface
    #

    def __init__(self, node, name):
        if "@" in name:
            node.dt._parse_error("'@' is only allowed in node names")

        self.name = name
        self.node = node
        self.value = b""
        self.labels = []
        self.offset_labels = []

        # A list of (offset, label, type) tuples (sorted by offset), giving the
        # locations of references within the value. 'type' is either _REF_PATH,
        # for a node path reference, _REF_PHANDLE, for a phandle reference, or
        # _REF_LABEL, for a label on/within data. Node paths and phandles need
        # to be patched in after parsing.
        self._markers = []

    def to_num(self, signed=False):
        """
        Returns the value of the property as a number.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_NUM):

            foo = < 1 >;

        signed (default: False):
          If True, the value will be interpreted as signed rather than
          unsigned.
        """
        if self.type is not TYPE_NUM:
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = < (number) >;', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        return int.from_bytes(self.value, "big", signed=signed)

    def to_nums(self, signed=False):
        """
        Returns the value of the property as a list of numbers.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_NUM or TYPE_NUMS):

            foo = < 1 2 ... >;

        signed (default: False):
          If True, the values will be interpreted as signed rather than
          unsigned.
        """
        if self.type not in (TYPE_NUM, TYPE_NUMS):
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = < (number) (number) ... >;', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        return [int.from_bytes(self.value[i:i + 4], "big", signed=signed)
                for i in range(0, len(self.value), 4)]

    def to_bytes(self):
        """
        Returns the value of the property as a raw 'bytes', like
        Property.value, except with added type checking.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_BYTES):

            foo = [ 01 ... ];
        """
        if self.type is not TYPE_BYTES:
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = [ (byte) (byte) ... ];', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        return self.value

    def to_string(self):
        """
        Returns the value of the property as a string.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_STRING):

            foo = "string";

        This function might also raise UnicodeDecodeError if the string is
        not valid UTF-8.
        """
        if self.type is not TYPE_STRING:
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = \"string\";', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        try:
            return self.value.decode("utf-8")[:-1]  # Strip null
        except UnicodeDecodeError:
            _err("value of property '{}' ({}) on {} in {} is not valid UTF-8"
                 .format(self.name, self.value, self.node.path,
                         self.node.dt.filename))

    def to_strings(self):
        """
        Returns the value of the property as a list of strings.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_STRING or TYPE_STRINGS):

            foo = "string", "string", ... ;

        Also raises DTError if any of the strings are not valid UTF-8.
        """
        if self.type not in (TYPE_STRING, TYPE_STRINGS):
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = \"string\", \"string\", ... ;', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        try:
            return self.value.decode("utf-8").split("\0")[:-1]
        except UnicodeDecodeError:
            _err("value of property '{}' ({}) on {} in {} is not valid UTF-8"
                 .format(self.name, self.value, self.node.path,
                         self.node.dt.filename))

    def to_node(self):
        """
        Returns the Node the phandle in the property points to.

        Raises DTError if the property was not assigned with this syntax (has
        Property.type TYPE_PHANDLE).

            foo = < &bar >;
        """
        if self.type is not TYPE_PHANDLE:
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = < &foo >;', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        return self.node.dt.phandle2node[int.from_bytes(self.value, "big")]

    def to_nodes(self):
        """
        Returns a list with the Nodes the phandles in the property point to.

        Raises DTError if the property value contains anything other than
        phandles. All of the following are accepted:

            foo = < >
            foo = < &bar >;
            foo = < &bar &baz ... >;
            foo = < &bar ... >, < &baz ... >;
        """
        def type_ok():
            if self.type in (TYPE_PHANDLE, TYPE_PHANDLES):
                return True
            # Also accept 'foo = < >;'
            return self.type is TYPE_NUMS and not self.value

        if not type_ok():
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "'{0} = < &foo &bar ... >;', not '{3}'"
                 .format(self.name, self.node.path,
                         self.node.dt.filename, self))

        return [self.node.dt.phandle2node[int.from_bytes(self.value[i:i + 4],
                                                         "big")]
                for i in range(0, len(self.value), 4)]

    def to_path(self):
        """
        Returns the Node referenced by the path stored in the property.

        Raises DTError if the property was not assigned with either of these
        syntaxes (has Property.type TYPE_PATH or TYPE_STRING):

            foo = &bar;
            foo = "/bar";

        For the second case, DTError is raised if the path does not exist.
        """
        if self.type not in (TYPE_PATH, TYPE_STRING):
            _err("expected property '{0}' on {1} in {2} to be assigned with "
                 "either '{0} = &foo' or '{0} = \"/path/to/node\"', not '{3}'"
                 .format(self.name, self.node.path, self.node.dt.filename,
                         self))

        try:
            path = self.value.decode("utf-8")[:-1]
        except UnicodeDecodeError:
            _err("value of property '{}' ({}) on {} in {} is not valid UTF-8"
                 .format(self.name, self.value, self.node.path,
                         self.node.dt.filename))

        try:
            return self.node.dt.get_node(path)
        except DTError:
            _err("property '{}' on {} in {} points to the non-existent node "
                 "\"{}\"".format(self.name, self.node.path,
                                 self.node.dt.filename, path))

    @property
    def type(self):
        """
        See the class docstring.
        """
        # Data labels (e.g. 'foo = label: <3>') are irrelevant, so filter them
        # out
        types = [marker[1] for marker in self._markers
                 if marker[1] != _REF_LABEL]

        if not types:
            return TYPE_EMPTY

        if types == [_TYPE_UINT8]:
            return TYPE_BYTES

        if types == [_TYPE_UINT32]:
            return TYPE_NUM if len(self.value) == 4 else TYPE_NUMS

        # Treat 'foo = <1 2 3>, <4 5>, ...' as TYPE_NUMS too
        if set(types) == {_TYPE_UINT32}:
            return TYPE_NUMS

        if set(types) == {_TYPE_STRING}:
            return TYPE_STRING if len(types) == 1 else TYPE_STRINGS

        if types == [_REF_PATH]:
            return TYPE_PATH

        if types == [_TYPE_UINT32, _REF_PHANDLE] and len(self.value) == 4:
            return TYPE_PHANDLE

        if set(types) == {_TYPE_UINT32, _REF_PHANDLE}:
            if len(self.value) == 4*types.count(_REF_PHANDLE):
                # Array with just phandles in it
                return TYPE_PHANDLES
            # Array with both phandles and numbers
            return TYPE_PHANDLES_AND_NUMS

        return TYPE_COMPOUND

    def __str__(self):
        s = "".join(label + ": " for label in self.labels) + self.name
        if not self.value:
            return s + ";"

        s += " ="

        for i, (pos, marker_type, ref) in enumerate(self._markers):
            if i < len(self._markers) - 1:
                next_marker = self._markers[i + 1]
            else:
                next_marker = None

            # End of current marker
            end = next_marker[0] if next_marker else len(self.value)

            if marker_type is _TYPE_STRING:
                # end - 1 to strip off the null terminator
                s += ' "{}"'.format(_decode_and_escape(
                    self.value[pos:end - 1]))
                if end != len(self.value):
                    s += ","
            elif marker_type is _REF_PATH:
                s += " &" + ref
                if end != len(self.value):
                    s += ","
            else:
                # <> or []

                if marker_type is _REF_LABEL:
                    s += " {}:".format(ref)
                elif marker_type is _REF_PHANDLE:
                    s += " &" + ref
                    pos += 4
                    # Subtle: There might be more data between the phandle and
                    # the next marker, so we can't 'continue' here
                else:  # marker_type is _TYPE_UINT*
                    elm_size = _TYPE_TO_N_BYTES[marker_type]
                    s += _N_BYTES_TO_START_STR[elm_size]

                while pos != end:
                    num = int.from_bytes(self.value[pos:pos + elm_size],
                                         "big")
                    if elm_size == 1:
                        s += " {:02X}".format(num)
                    else:
                        s += " " + hex(num)

                    pos += elm_size

                if pos != 0 and \
                   (not next_marker or
                    next_marker[1] not in (_REF_PHANDLE, _REF_LABEL)):

                    s += _N_BYTES_TO_END_STR[elm_size]
                    if pos != len(self.value):
                        s += ","

        return s + ";"


    def __repr__(self):
        return "<Property '{}' at '{}' in '{}'>" \
               .format(self.name, self.node.path, self.node.dt.filename)

    #
    # Internal functions
    #

    def _add_marker(self, marker_type, data=None):
        # Helper for registering markers in the value that are processed after
        # parsing. See _fixup_props(). 'marker_type' identifies the type of
        # marker, and 'data' has any optional data associated with the marker.

        # len(self.value) gives the current offset. This function is called
        # while the value is built. We use a list instead of a tuple to be able
        # to fix up offsets later (they might increase if the value includes
        # path references, e.g. 'foo = &bar, <3>;', which are expanded later).
        self._markers.append([len(self.value), marker_type, data])

        # For phandle references, add a dummy value with the same length as a
        # phandle. This is handy for the length check in _register_phandles().
        if marker_type is _REF_PHANDLE:
            self.value += b"\0\0\0\0"


#
# Public functions
#


def to_num(data, length=None, signed=False):
    """
    Converts the 'bytes' array 'data' to a number. The value is expected to be
    in big-endian format, which is standard in devicetree.

    length (default: None):
      The expected length of the value in bytes, as a simple type check. If
      None, the length check is skipped.

    signed (default: False):
      If True, the value will be interpreted as signed rather than unsigned.
    """
    _check_is_bytes(data)
    if length is not None:
        _check_length_positive(length)
        if len(data) != length:
            _err("{} is {} bytes long, expected {}"
                 .format(data, len(data), length))

    return int.from_bytes(data, "big", signed=signed)


def to_nums(data, length=4, signed=False):
    """
    Like Property.to_nums(), but takes an arbitrary 'bytes' array. The values
    are assumed to be in big-endian format, which is standard in devicetree.
    """
    _check_is_bytes(data)
    _check_length_positive(length)

    if len(data) % length:
        _err("{} is {} bytes long, expected a length that's a a multiple of {}"
             .format(data, len(data), length))

    return [int.from_bytes(data[i:i + length], "big", signed=signed)
            for i in range(0, len(data), length)]


#
# Public constants
#

# See Property.type
TYPE_EMPTY = 0
TYPE_BYTES = 1
TYPE_NUM = 2
TYPE_NUMS = 3
TYPE_STRING = 4
TYPE_STRINGS = 5
TYPE_PATH = 6
TYPE_PHANDLE = 7
TYPE_PHANDLES = 8
TYPE_PHANDLES_AND_NUMS = 9
TYPE_COMPOUND = 10


def _check_is_bytes(data):
    if not isinstance(data, bytes):
        _err("'{}' has type '{}', expected 'bytes'"
             .format(data, type(data).__name__))


def _check_length_positive(length):
    if length < 1:
        _err("'length' must be greater than zero, was " + str(length))


def _append_no_dup(lst, elm):
    # Appends 'elm' to 'lst', but only if it isn't already in 'lst'. Lets us
    # preserve order, which a set() doesn't.

    if elm not in lst:
        lst.append(elm)


def _decode_and_escape(b):
    # Decodes the 'bytes' array 'b' as UTF-8 and backslash-escapes special
    # characters

    # Hacky but robust way to avoid double-escaping any '\' spit out by
    # 'backslashreplace' bytes.translate() can't map to more than a single
    # byte, but str.translate() can map to more than one character, so it's
    # nice here. There's probably a nicer way to do this.
    return b.decode("utf-8", "surrogateescape") \
            .translate(_escape_table) \
            .encode("utf-8", "surrogateescape") \
            .decode("utf-8", "backslashreplace")


def _root_and_path_to_node(cur, path, fullpath):
    # Returns the node pointed at by 'path', relative to the Node 'cur'. For
    # example, if 'cur' has path /foo/bar, and 'path' is "baz/qaz", then the
    # node with path /foo/bar/baz/qaz is returned. 'fullpath' is the path as
    # given in the .dts file, for error messages.

    for component in path.split("/"):
        # Collapse multiple / in a row, and allow a / at the end
        if not component:
            continue

        if component not in cur.nodes:
            _err("component '{}' in path '{}' does not exist"
                 .format(component, fullpath))

        cur = cur.nodes[component]

    return cur


def _err(msg):
    raise DTError(msg)


_escape_table = str.maketrans({
    "\\": "\\\\",
    '"': '\\"',
    "\a": "\\a",
    "\b": "\\b",
    "\t": "\\t",
    "\n": "\\n",
    "\v": "\\v",
    "\f": "\\f",
    "\r": "\\r"})


class DTError(Exception):
    "Exception raised for devicetree-related errors"


_Token = collections.namedtuple("Token", "id val")

# Lexer states
_DEFAULT = 0
_EXPECT_PROPNODENAME = 1
_EXPECT_BYTE = 2

_num_re = re.compile(r"(0[xX][0-9a-fA-F]+|[0-9]+)(?:ULL|UL|LL|U|L)?")

# A leading \ is allowed property and node names, probably to allow weird node
# names that would clash with other stuff
_propnodename_re = re.compile(r"\\?([a-zA-Z0-9,._+*#?@-]+)")

# Misc. tokens that are tried after a property/node name. This is important, as
# there's overlap with the allowed characters in names.
_misc_re = re.compile(
    "|".join(re.escape(pat) for pat in (
        "==", "!=", "!", "=", ",", ";", "+", "-", "*", "/", "%", "~", "?", ":",
        "^", "(", ")", "{", "}", "[", "]", "<<", "<=", "<", ">>", ">=", ">",
        "||", "|", "&&", "&")))

_byte_re = re.compile(r"[0-9a-fA-F]{2}")

# Matches a backslash escape within a 'bytes' array. Captures the 'c' part of
# '\c', where c might be a single character or an octal/hex escape.
_unescape_re = re.compile(br'\\([0-7]{1,3}|x[0-9A-Fa-f]{1,2}|.)')

# #line directive (this is the regex the C tools use)
_line_re = re.compile(
    r'^#(?:line)?[ \t]+([0-9]+)[ \t]+"((?:[^\\"]|\\.)*)"(?:[ \t]+[0-9]+)?',
    re.MULTILINE)


def _init_tokens():
    # Builds a (<token 1>)|(<token 2>)|... regex and assigns the index of each
    # capturing group to a corresponding _T_<TOKEN> variable. This makes the
    # token type appear in match.lastindex after a match.

    global _token_re
    global _T_NUM
    global _T_PROPNODENAME
    global _T_MISC
    global _T_BYTE
    global _T_BAD

    # Each pattern must have exactly one capturing group, which can capture any
    # part of the pattern. This makes match.lastindex match the token type.
    # _Token.val is based on the captured string.
    token_spec = (("_T_INCLUDE",        r'(/include/\s*"(?:[^\\"]|\\.)*")'),
                  ("_T_LINE",  # #line directive
                   r'^#(?:line)?[ \t]+([0-9]+[ \t]+"(?:[^\\"]|\\.)*")(?:[ \t]+[0-9]+)?'),
                  ("_T_STRING",         r'"((?:[^\\"]|\\.)*)"'),
                  ("_T_DTS_V1",         r"(/dts-v1/)"),
                  ("_T_PLUGIN",         r"(/plugin/)"),
                  ("_T_MEMRESERVE",     r"(/memreserve/)"),
                  ("_T_BITS",           r"(/bits/)"),
                  ("_T_DEL_PROP",       r"(/delete-property/)"),
                  ("_T_DEL_NODE",       r"(/delete-node/)"),
                  ("_T_OMIT_IF_NO_REF", r"(/omit-if-no-ref/)"),
                  ("_T_LABEL",          r"([a-zA-Z_][a-zA-Z0-9_]*):"),
                  ("_T_CHAR_LITERAL",   r"'((?:[^\\']|\\.)*)'"),
                  ("_T_REF",
                   r"&([a-zA-Z_][a-zA-Z0-9_]*|{[a-zA-Z0-9,._+*#?@/-]*})"),
                  ("_T_INCBIN",         r"(/incbin/)"),
                  # Whitespace, C comments, and C++ comments
                  ("_T_SKIP", r"(\s+|(?:/\*(?:.|\n)*?\*/)|//.*$)"),
                  # Return a token for end-of-file so that the parsing code can
                  # always assume that there are more tokens when looking
                  # ahead. This simplifies things.
                  ("_T_EOF",            r"(\Z)"))

    # MULTILINE is needed for C++ comments and #line directives
    _token_re = re.compile("|".join(spec[1] for spec in token_spec),
                           re.MULTILINE | re.ASCII)

    for i, spec in enumerate(token_spec, 1):
        globals()[spec[0]] = i

    # pylint: disable=undefined-loop-variable
    _T_NUM = i + 1
    _T_PROPNODENAME = i + 2
    _T_MISC = i + 3
    _T_BYTE = i + 4
    _T_BAD = i + 5


_init_tokens()

# Markers in property values

# References
_REF_PATH = 0     # &foo
_REF_PHANDLE = 1  # <&foo>
_REF_LABEL = 2    # foo: <1 2 3>
# Start of data blocks of specific type
_TYPE_UINT8 = 3   # [00 01 02] (and also used for /incbin/)
_TYPE_UINT16 = 4  # /bits/ 16 <1 2 3>
_TYPE_UINT32 = 5  # <1 2 3>
_TYPE_UINT64 = 6  # /bits/ 64 <1 2 3>
_TYPE_STRING = 7  # "foo"

_TYPE_TO_N_BYTES = {
    _TYPE_UINT8: 1,
    _TYPE_UINT16: 2,
    _TYPE_UINT32: 4,
    _TYPE_UINT64: 8,
}

_N_BYTES_TO_TYPE = {
    1: _TYPE_UINT8,
    2: _TYPE_UINT16,
    4: _TYPE_UINT32,
    8: _TYPE_UINT64,
}

_N_BYTES_TO_START_STR = {
    1: " [",
    2: " /bits/ 16 <",
    4: " <",
    8: " /bits/ 64 <",
}

_N_BYTES_TO_END_STR = {
    1: " ]",
    2: " >",
    4: " >",
    8: " >",
}
