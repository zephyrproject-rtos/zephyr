# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.rich.svg module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring

import pytest

from dtsh.rich.svg import SVGContentsFmt, SVGContents


def test_svgfmt_get_rect_width_height() -> None:
    rect: str = '<rect attr="value" width="699.59" height="380" attr="value"'
    assert (700, 380) == SVGContentsFmt.get_rect_width_heigt(rect)
    # Leading spaces.
    assert (700, 380) == SVGContentsFmt.get_rect_width_heigt("  " + rect)

    with pytest.raises(SVGContentsFmt.Error):
        # Not a <rect>.
        SVGContentsFmt.get_rect_width_heigt('<foo width="699.59" height="380"')

    with pytest.raises(SVGContentsFmt.Error):
        # Not a number.
        SVGContentsFmt.get_rect_width_heigt('<rect width="xxx" height="380"')


def test_svgfmt_set_rect_width_height() -> None:
    rect = '<rect attr="value" width="0" height="0" attr="value">'
    expect_rect = '<rect attr="value" width="100" height="200" attr="value">'
    assert expect_rect == SVGContentsFmt.set_rect_width_height(rect, 100, 200)


def test_sfgfmt_get_gtranslate_xy() -> None:
    gtranslate = '<g transform="translate(9, 41) attr="value"'
    assert (9, 41) == SVGContentsFmt.get_gtranslate_xy(gtranslate)
    # Leading spaces.
    assert (9, 41) == SVGContentsFmt.get_gtranslate_xy(" " + gtranslate)

    with pytest.raises(SVGContentsFmt.Error):
        # Not a translation.
        SVGContentsFmt.get_gtranslate_xy('<g attr="value"')

    with pytest.raises(SVGContentsFmt.Error):
        # Invalid coordinates.
        SVGContentsFmt.get_gtranslate_xy('<g transform="translate(x, y)')


def test_sfgfmt_set_gtranslate_xy() -> None:
    gtranslate = '<g transform="translate(0, 0)>'
    expect_gtranslate = '<g transform="translate(9, 41)>'
    assert expect_gtranslate == SVGContentsFmt.set_gtranslate_xy(
        gtranslate, 9, 41
    )


def test_svgcontents() -> None:
    contents = SVG_FMT_TEST_CONTENTS.splitlines()
    svg = SVGContents(contents)

    assert 7 == len(svg.styles)
    assert 6 == len(svg.defs)

    i_chrome = SVGContentsFmt.find_chrome(contents)
    assert 21 == i_chrome
    expect_rect = contents[i_chrome + 1]
    assert expect_rect == svg.rect

    assert 700 == svg.width
    assert 390 == svg.height
    assert svg.rect.lstrip().startswith(
        '<rect fill="#292929" stroke="rgba(255,255,255,0.35)" width="700" height="389.6"'
    )

    svg.set_rect_width_height(666, 777)
    assert 666 == svg.width
    assert 777 == svg.height
    assert svg.rect.lstrip().startswith(
        '<rect fill="#292929" stroke="rgba(255,255,255,0.35)" width="666" height="777"'
    )

    assert 1 == len(svg.gterms)
    gterm = svg.gterms[0]
    assert 9 == gterm.x
    assert 41 == gterm.y
    assert (
        gterm.contents[0].lstrip().startswith('<g transform="translate(9, 41)"')
    )

    svg.set_gterm_xy(666, 777)
    assert 666 == gterm.x == svg.term_x
    assert 777 == gterm.y == svg.term_y
    assert (
        gterm.contents[0]
        .lstrip()
        .startswith('<g transform="translate(666, 777)"')
    )


def test_svgcontents_top_padding_correction() -> None:
    contents = SVG_FMT_TEST_CONTENTS.splitlines()
    svg = SVGContents(contents)
    assert 700 == svg.width
    assert 390 == svg.height
    assert 9 == svg.term_x
    assert 41 == svg.term_y

    svg.top_padding_correction()
    assert 700 == svg.width
    assert 390 - SVGContentsFmt.PAD_TOP_CORRECTION == svg.height
    assert 9 == svg.term_x
    assert 41 - SVGContentsFmt.PAD_TOP_CORRECTION == svg.term_y


def test_svgcontents_append() -> None:
    contents = SVG_FMT_TEST_CONTENTS.splitlines()
    svg = SVGContents(contents)
    svg_styles = list(svg.styles)
    svg_defs = list(svg.defs)
    svg_width = svg.width
    svg_height = svg.height
    svg_x = svg.term_x
    svg_y = svg.term_y
    # Number of contents line.
    svg_gterm_size = len(svg.gterm.contents)

    new_contents = SVG_FMT_APPEND_CONTENTS.splitlines()
    svgnew = SVGContents(new_contents)
    svgnew_y = svgnew.term_y

    svg.append(svgnew)
    # The translation is updated upon append().
    assert svgnew_y != svgnew.term_y

    assert len(svg_styles) + len(svgnew.styles) == len(svg.styles)
    assert len(svg_defs) + len(svgnew.defs) == len(svg.defs)

    assert max(svg_width, svgnew.width) == svg.width
    assert svg_height + svgnew.height == svg.height

    # Old Terminal group unchanged.
    assert svg_x == svg.term_x
    assert svg_y == svg.term_y

    # New group must be translated verticaly.
    assert svgnew.term_x == svg_x == svg.term_x
    assert svgnew.term_y == svgnew_y + svg_height

    assert 2 == len(svg.gterms)
    assert len(svgnew.gterm.contents) + svg_gterm_size == len(
        svg.gterms[0].contents
    ) + len(svg.gterms[1].contents)


SVG_FMT_TEST_CONTENTS = """\
<svg xmlns="http://www.w3.org/2000/svg">

    <style>
    .terminal-3360587291-matrix {
        font-family: Source Code Pro, monospace;
        font-size: 20px;
        line-height: 24.4px;
    }

    .terminal-3360587291-r1 { fill: #c5c8c6 }
    </style>

    <defs>
        <clipPath id="terminal-3360587291-clip-terminal">
            <rect x="0" y="0" width="682.1999999999999" height="340.59999999999997" />
        </clipPath>
        <clipPath id="terminal-3360587291-line-0">
    <rect x="0" y="1.5" width="683.2" height="24.65"/>
            </clipPath>
    </defs>

    <!-- :chrome: -->
    <rect fill="#292929" stroke="rgba(255,255,255,0.35)" width="700" height="389.6" rx="8"/>
            <g transform="translate(26,22)">
            <circle cx="0" cy="0" r="7" fill="#ff5f57"/>
            <circle cx="22" cy="0" r="7" fill="#febc2e"/>
            <circle cx="44" cy="0" r="7" fill="#28c840"/>
            </g>


    <!-- :gterm_begin: -->
    <g transform="translate(9, 41)" clip-path="url(#terminal-3360587291-clip-terminal)">
        <g class="terminal-3360587291-matrix">
        </g>
    </g>
    <!-- :gterm_end: -->

</svg>
"""


SVG_FMT_APPEND_CONTENTS = """\
<svg xmlns="http://www.w3.org/2000/svg">

    <style>
    .terminal-3360587291-r1 { fill: #c5c8c6 }
    </style>

    <defs>
        <clipPath id="terminal-3360587291-clip-terminal">
            <rect x="0" y="0" width="682.1999999999999" height="340.59999999999997" />
        </clipPath>
    </defs>

    <!-- :chrome: -->
    <rect fill="#292929" stroke="rgba(255,255,255,0.35)" width="720" height="300" rx="8"/>

    <!-- :gterm_begin: -->
    <g transform="translate(9, 41)" clip-path="url(#terminal-3360587291-clip-terminal)">
        <g class="terminal-3360587291-matrix">
        </g>
    </g>
    <!-- :gterm_end: -->

</svg>
"""
