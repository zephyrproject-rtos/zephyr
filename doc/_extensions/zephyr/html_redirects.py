# Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
#
# SPDX-License-Identifier: Apache-2.0

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


# Mechanism to generate static HTML redirect pages in the output
#
# Uses redirect_template.html and the list of pages given in
# conf.html_redirect_pages
#
# Adapted from ideas in https://tech.signavio.com/2017/managing-sphinx-redirects
import os.path

from sphinx.builders.html import StandaloneHTMLBuilder
from sphinx.util import logging

logger = logging.getLogger(__name__)

REDIRECT_TEMPLATE = r"""
<html>
  <head>
    <meta http-equiv="refresh" content="0; url=$NEWURL" />
    <script>
     var id=window.location.href.split("#")[1];

     if (id && (/^[a-zA-Z\:\/0-9\_\-\.]+$/.test(id))) {
        window.location.href = "$NEWURL"+"#"+id;
        }
     else {
        window.location.href = "$NEWURL";
     };
    </script>
  </head>
  <body>
  <p>Page has moved <a href="$NEWURL">here</a>.</p>
  </body>
</html>
"""


def setup(app):
    app.add_config_value('html_redirect_pages', (), 'html')
    app.connect('build-finished', create_redirect_pages)

    # Since we're just setting up a build-finished hook, which runs
    # after both reading and writing, this extension is safe for both.
    return {
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }


def create_redirect_pages(app, exception):
    if not isinstance(app.builder, StandaloneHTMLBuilder):
        return  # only relevant for standalone HTML output

    for (old_url, new_url) in app.config.html_redirect_pages:
        if old_url.startswith('/'):
            old_url = old_url[1:]

        # check that new_url is a valid docname, or if not that it is at least
        # covered as the "old" part of another redirect rule
        if new_url not in app.env.all_docs and not any(
            old == new_url for (old, _) in app.config.html_redirect_pages
        ):
            logger.warning(
                f"{new_url} is not a valid destination for a redirect."
                "Check that both the source and destination are complete docnames."
            )

        new_url = app.builder.get_relative_uri(old_url, new_url)
        out_file = app.builder.get_outfilename(old_url)

        out_dir = os.path.dirname(out_file)
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)

        content = REDIRECT_TEMPLATE.replace("$NEWURL", new_url)

        if not os.path.exists(out_file):
            with open(out_file, "w") as rp:
                rp.write(content)
