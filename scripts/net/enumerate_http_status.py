#!/usr/bin/env python3
# Copyright (c) 2022 Meta
# SPDX-License-Identifier: Apache-2.0

"""Format HTTP Status codes for use in a C header

This script extracts HTTP status codes from mozilla.org
and formats them to fit inside of a C enum along with
comments.

The output may appear somewhat redundant but it strives
to
a) be human readable
b) eliminate the need to look up status manually,
c) be machine parseable for table generation

The output is sorted for convenience.

Usage:
    ./scripts/net/enumerate_http_status.py
	HTTP_100_CONTINUE = 100, /**< Continue */
    ...
    HTTP_418_IM_A_TEAPOT = 418, /**< I'm a teapot */
	...
	HTTP_511_NETWORK_AUTHENTICATION_REQUIRED = 511, /**< Network Authentication Required */
"""

from html.parser import HTMLParser
import requests
import re

class HTTPStatusParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.status_codes = {}
        self.in_code_tag = False
        self.current_data = ""

    def handle_starttag(self, tag, attrs):
        if tag == 'code':
            self.in_code_tag = True
            self.current_data = ""

    def handle_endtag(self, tag):
        if tag == 'code' and self.in_code_tag:
            self.in_code_tag = False
            if self.current_data.strip():
                match = re.match(r'([0-9]{3}) ([a-zA-Z].*)', self.current_data.strip())
                if match:
                    code = int(match.group(1))
                    description = match.group(2)
                    self.status_codes[code] = description

    def handle_data(self, data):
        if self.in_code_tag:
            self.current_data += data

page = requests.get('https://developer.mozilla.org/en-US/docs/Web/HTTP/Status')

parser = HTTPStatusParser()
parser.feed(page.text)

for key in sorted(parser.status_codes.keys()):
    val = parser.status_codes[key]
    enum_head = 'HTTP'
    enum_body = f'{key}'
    enum_tail = val.upper().replace(' ', '_').replace("'", '').replace('-', '_')
    enum_label = '_'.join([enum_head, enum_body, enum_tail])
    comment = f'/**< {val} */'

    print(f'{enum_label} = {key}, {comment}')
