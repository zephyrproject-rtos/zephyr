#!/usr/bin/env python3
# Copyright(c) 2022 Meta
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

from lxml import html
import requests
import re

page = requests.get('https://developer.mozilla.org/en-US/docs/Web/HTTP/Status')
tree = html.fromstring(page.content)

codes = tree.xpath('//code/text()')

codes2 = {}
for c in codes:
    if re.match('[0-9][0-9][0-9] [a-zA-Z].*', c):
        key = int(c[0:3])
        val = c[4:]
        codes2[key] = val

keys = sorted(codes2.keys())
for key in keys:
    val = codes2[key]
    enum_head = 'HTTP'
    enum_body = f'{key}'
    enum_tail = val.upper().replace(' ', '_').replace("'", '').replace('-', '_')
    enum_label = '_'.join([enum_head, enum_body, enum_tail])
    comment = f'/**< {val} */'

    print(f'{enum_label} = {key}, {comment}')
