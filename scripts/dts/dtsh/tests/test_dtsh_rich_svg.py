# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.rich.svg module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring

from dtsh.rich.svg import (
    SVGFragmentViewBox,
    SVGFragmentStyle,
    SVGFragmentDefs,
    SVGFragmentChrome,
    SVGFragmentGCircles,
    SVGFragmentGTerminal,
)


def test_svg_fragment_viewbox() -> None:
    fragment = SVGFragmentViewBox.ifind(
        [
            "<!-- before -->",
            '<svg class="rich-terminal" viewBox="0 0 1434 2563.2" xmlns="http://www.w3.org/2000/svg">',
            "<!-- after -->",
        ]
    )
    assert 1 == fragment.i_end
    assert 1 == len(fragment.content)
    assert 1434 == fragment.width
    assert 2563.2 == fragment.height
    assert [
        '<svg class="rich-terminal" viewBox="0 0 1434.0 2563.2" xmlns="http://www.w3.org/2000/svg">',
    ] == fragment.content

    fragment.set_width_height(1024, 768)
    assert 1024 == fragment.width
    assert 768 == fragment.height
    assert [
        '<svg class="rich-terminal" viewBox="0 0 1024 768" xmlns="http://www.w3.org/2000/svg">',
    ] == fragment.content


def test_svg_fragment_style() -> None:
    fragment = SVGFragmentStyle.ifind(
        [
            "<!-- before -->",
            '<style type="">',
            "...",
            "</style>",
            "<!-- after -->",
        ]
    )
    assert 3 == fragment.i_end
    assert 3 == len(fragment.content)
    assert [
        '<style type="">',
        "...",
        "</style>",
    ] == fragment.content


def test_svg_fragment_defs() -> None:
    fragment = SVGFragmentDefs.ifind(
        [
            "<!-- before -->",
            "<defs>",
            '<clipPath id="terminal-257872858-clip-terminal">',
            '<rect x="0" y="0" width="755.4" height="299.4" />',
            "</clipPath>",
            "...",
            "</defs>",
            "<!-- after -->",
        ]
    )
    assert 6 == fragment.i_end
    assert 6 == len(fragment.content)
    assert [
        "<defs>",
        '<clipPath id="terminal-257872858-clip-terminal">',
        # 307.4 = 299.4 + 8 (rich library issue 3576)
        '<rect x="0" y="0" width="755.4" height="307.4" />',
        "</clipPath>",
        "...",
        "</defs>",
    ] == fragment.content


def test_svg_fragment_rect() -> None:
    fragment = SVGFragmentChrome.ifind(
        [
            "<!-- before -->",
            '<rect fill="#292929" stroke="rgba(255,255,255,0.35)" stroke-width="1" x="1" y="1" width="1432" height="2561.2" rx="8"/>',
            "<!-- after -->",
        ]
    )
    assert 1 == fragment.i_end
    assert 1 == len(fragment.content)
    assert 1432 == fragment.width
    assert 2561.2 == fragment.height
    assert [
        '<rect fill="#292929" stroke="rgba(255,255,255,0.35)" stroke-width="1" x="1" y="1" width="1432.0" height="2561.2" rx="8"/>'
    ] == fragment.content

    fragment.set_width_height(1024, 768)
    assert 1024 == fragment.width
    assert 768 == fragment.height
    assert [
        '<rect fill="#292929" stroke="rgba(255,255,255,0.35)" stroke-width="1" x="1" y="1" width="1024" height="768" rx="8"/>'
    ] == fragment.content


def test_svg_fragment_gcircles() -> None:
    fragment = SVGFragmentGCircles.ifind(
        [
            "<!-- before -->",
            '<g transform="translate(26,22)">',
            "...",
            "</g>",
            "<!-- after -->",
        ]
    )
    assert 3 == fragment.i_end
    assert 3 == len(fragment.content)
    assert [
        '<g transform="translate(26,22)">',
        "...",
        "</g>",
    ] == fragment.content


def test_svg_fragment_gterminal() -> None:
    fragment = SVGFragmentGTerminal.ifind(
        [
            "<!-- before -->",
            '<g transform="translate(9, 41)" clip-path="url(#terminal-3423481079-clip-terminal)">',
            "",
            '<g class="terminal-3423481079-matrix">',
            "...",
            "</g>",
            "</g>",
            "<!-- after -->",
        ]
    )
    assert 6 == fragment.i_end
    assert 6 == len(fragment.content)
    assert [
        '<g transform="translate(9.0, 41.0)" clip-path="url(#terminal-3423481079-clip-terminal)">',
        "",
        '<g class="terminal-3423481079-matrix">',
        "...",
        "</g>",
        "</g>",
    ] == fragment.content
