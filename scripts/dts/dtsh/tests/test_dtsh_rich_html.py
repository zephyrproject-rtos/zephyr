# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.rich.svg module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring

from typing import List

from dtsh.rich.html import HtmlStyle, HtmlPreformatted, HtmlDocument

SAMPLE_HTML_CAPTURE = """\
<!DOCTYPE html>
<html>

<head>
<meta charset="UTF-8">

<style>
.r1 {font-weight: bold}
.r2 {color: #afaf5f; text-decoration-color: #afaf5f}
body {
    color: #c5c8c6;
    background-color: #292929;
}
</style>

<body>

    <pre style="..."><code style="..."><span class="r1">DESCRIPTION</span>
    <span class="r2">Nordic NVMC (Non-Volatile Memory Controller)</span>

</code></pre>

</body>

</html>
"""

SAMPLE_HTML_DOCUMENT = """\
<!DOCTYPE html>
<html>

<head>
<meta charset="UTF-8">

<style>
.r1 {font-weight: italic}
.r2 {color: #afaf5f; text-decoration-color: #afaf5f}
.r3 {color: #c5c8c6; text-decoration-color: #c5c8c6}
.r4 {color: #8787d7; text-decoration-color: #8787d7}
body {
    color: #c5c8c6;
    background-color: #292929;
}
</style>

<body>

    <pre style="..."><code style="..."><span class="r1">DESCRIPTION</span>
    <span class="r2">Nordic NVMC (Non-Volatile Memory Controller)</span>

</code></pre>

    <pre style="..."><code style="..."><span class="r3">DESCRIPTION</span>
    <span class="r4">Nordic NVMC (Non-Volatile Memory Controller)</span>

</code></pre>

</body>

</html>
"""


def test_html_style() -> None:
    html_style: HtmlStyle = HtmlStyle.ifind(SAMPLE_HTML_CAPTURE.splitlines())
    assert 13 == html_style.i_end
    assert 8 == len(html_style.content)
    assert (
        """\
<style>
.r1 {font-weight: bold}
.r2 {color: #afaf5f; text-decoration-color: #afaf5f}
body {
    color: #c5c8c6;
    background-color: #292929;
}
</style>
""".splitlines()
        == html_style.content
    )

    assert 2 == len(html_style.rclasses)
    assert html_style.rclasses[0].startswith(".r1")
    assert html_style.rclasses[1].startswith(".r2")

    assert 4 == len(html_style.body)
    assert (
        """\
body {
    color: #c5c8c6;
    background-color: #292929;
}
""".splitlines()
        == html_style.body
    )

    html_style.shift_rclasses(5)
    assert 2 == len(html_style.rclasses)
    assert html_style.rclasses[0].startswith(".r6")
    assert html_style.rclasses[1].startswith(".r7")


def test_html_preformatted() -> None:
    pre_code: HtmlPreformatted = HtmlPreformatted.ifind(
        SAMPLE_HTML_CAPTURE.splitlines()
    )
    assert 20 == pre_code.i_end
    assert 4 == len(pre_code.content)
    assert "r1" in pre_code.content[0]
    assert "r2" in pre_code.content[1]

    pre_code.shift_rclasses(2, 5)
    assert "r6" in pre_code.content[0]
    assert "r7" in pre_code.content[1]

    pre_codes: List[HtmlPreformatted] = HtmlPreformatted.ifind_list(
        SAMPLE_HTML_DOCUMENT.splitlines()
    )
    assert 2 == len(pre_codes)

    assert 4 == len(pre_codes[0].content)
    assert "r1" in pre_codes[0].content[0]
    assert "r2" in pre_codes[0].content[1]
    assert "r3" in pre_codes[1].content[0]
    assert "r4" in pre_codes[1].content[1]


def test_html_doc() -> None:
    doc: HtmlDocument = HtmlDocument(SAMPLE_HTML_DOCUMENT)
    other_doc: HtmlDocument = HtmlDocument(SAMPLE_HTML_CAPTURE)

    doc.append(other_doc)
    assert 6 == len(doc.style.rclasses)
    assert doc.style.rclasses[0].startswith(".r1")
    assert doc.style.rclasses[5].startswith(".r6")

    assert 3 == len(doc.codes)
    assert "r1" in doc.codes[0].content[0]
    assert "r2" in doc.codes[0].content[1]
    assert "r3" in doc.codes[1].content[0]
    assert "r4" in doc.codes[1].content[1]
    assert "r5" in doc.codes[2].content[0]
    assert "r6" in doc.codes[2].content[1]

    print()
    print(doc.content)


ISSUE_CONTENT = """\
<!DOCTYPE html>
<html>

<head>
<meta charset="UTF-8">

<style>
.r1 {color: #00d7d7; text-decoration-color: #00d7d7; background-color: #292929; font-weight: bold}
.r2 {color: #c5c8c6; text-decoration-color: #c5c8c6; background-color: #292929}
.r3 {color: #00d7d7; text-decoration-color: #00d7d7; background-color: #292929; font-style: italic}
.r4 {color: #00d7d7; text-decoration-color: #00d7d7; font-style: italic}
.r5 {color: #afafd7; text-decoration-color: #afafd7; background-color: #292929}
.r6 {color: #8787ff; text-decoration-color: #8787ff; background-color: #292929; font-weight: bold}
.r7 {color: #c5c8c6; text-decoration-color: #c5c8c6; background-color: #292929; font-weight: bold}
body {
    color: #c5c8c6;
    background-color: #292929;
}
</style>
</head>

<body>

        <pre style="font-family:'DejaVu Sans Mono',monospace; font-size:medium"><code style="font-family:inherit"><span class="r1">nrf52840dk</span><span class="r2"> </span><span class="r3">nrf52840</span> — <span class="r4">nrf52840</span> — <span class="r2">Zephyr-RTOS </span><span class="r5">v3.7.0 (36940db938a)</span><span class="r2"> </span><span class="r6">HWMv2</span> — <span class="r7">hello_world</span><span class="r2"> 3.7.0</span>
</code></pre>

</body>

</html>
"""
