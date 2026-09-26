#!/usr/bin/env python3
# Copyright (c) 2026 Clovis Corde
# SPDX-License-Identifier: Apache-2.0

"""Generate bsdiffhs test vectors for tests/lib/delta.

Emits vectors.inc next to this script. Each vector is the (src, dst,
patch) triple needed to exercise delta_backend_bsdiffhs end-to-end.
The patch carries the 18-byte extended BSDIFFHS header expected by the
backend (16 bytes from the bsdiffhs library + 2 bytes for window_sz2
and lookahead_sz2).

Regenerate:
    pip install bsdiffhs
    python3 tests/lib/delta/vectors/gen_vectors.py
"""

import os
import sys
import textwrap

import bsdiffhs
import heatshrink2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "vectors.inc")

# Heatshrink parameters used for every generated patch. In static
# allocation mode the decoder must be built with exactly these values:
# CONFIG_HEATSHRINK_STATIC_WINDOW_BITS == WINDOW_SZ2 and
# CONFIG_HEATSHRINK_STATIC_LOOKAHEAD_BITS == LOOKAHEAD_SZ2.
WINDOW_SZ2 = 8
LOOKAHEAD_SZ2 = 4


def make_patch(src: bytes, dst: bytes) -> bytes:
    """Return the 18-byte-header bsdiffhs patch reconstructing dst from src."""
    raw = bsdiffhs.diff(src, dst, WINDOW_SZ2, LOOKAHEAD_SZ2)
    if raw[:8] != b"BSDIFFHS":
        raise RuntimeError(f"unexpected magic: {raw[:8]!r}")
    # Library produces 16-byte header (magic + target_size LE64);
    # splice in [window_sz2, lookahead_sz2] before the compressed body.
    return raw[:16] + bytes([WINDOW_SZ2, LOOKAHEAD_SZ2]) + raw[16:]


def verify(src: bytes, dst: bytes, patch: bytes) -> None:
    """Round-trip sanity check against the host bsdiffhs library."""
    raw = patch[:16] + patch[18:]
    got = bsdiffhs.patch(src, raw, WINDOW_SZ2, LOOKAHEAD_SZ2)
    if got != dst:
        raise RuntimeError("host round-trip mismatch")


def encode_sign_magnitude(value: int) -> bytes:
    """Encode a signed int64 in bsdiff sign-magnitude little-endian form."""
    neg = value < 0
    mag = -value if neg else value
    if mag >= (1 << 63):
        raise ValueError("magnitude exceeds 63 bits")
    out = bytearray(mag.to_bytes(8, "little"))
    if neg:
        out[7] |= 0x80
    return bytes(out)


def synthesize_negative_seek_patch() -> tuple[bytes, bytes]:
    """Hand-craft a bsdiffhs patch with a negative source_seek.

    We build the ctrl/diff streams ourselves to pin source_seek = -16 in
    tuple 1, so the test exercises sign-magnitude decoding of the seek.
    Rebuilds dst = 'B'*16 + 'A'*16 from src = 'A'*16 + 'B'*16.
    """
    src = b"A" * 16 + b"B" * 16
    dst = b"B" * 16 + b"A" * 16

    # One heatshrink frame per sub-payload (ctrl, then diff, then extra if
    # any); the backend resets the decoder at each boundary. Both tuples
    # here have extra_count == 0, so no extra frames.
    # Each control tuple is (diff_count, extra_count, source_seek), all
    # sign-magnitude int64. source_seek = -16 rewinds the source pointer.
    ctrl1 = encode_sign_magnitude(16) + encode_sign_magnitude(0) + encode_sign_magnitude(-16)
    diff1 = bytes([0x01] * 16)
    ctrl2 = encode_sign_magnitude(16) + encode_sign_magnitude(0) + encode_sign_magnitude(0)
    diff2 = bytes([0x00] * 16)

    def hs(b: bytes) -> bytes:
        return heatshrink2.compress(b, window_sz2=WINDOW_SZ2, lookahead_sz2=LOOKAHEAD_SZ2)

    header = b"BSDIFFHS" + len(dst).to_bytes(8, "little") + bytes([WINDOW_SZ2, LOOKAHEAD_SZ2])
    return src, dst, header + hs(ctrl1) + hs(diff1) + hs(ctrl2) + hs(diff2)


def vectors() -> list[tuple[str, bytes, bytes, bytes | None]]:
    """Return test vectors as (name, src, dst, patch_or_None) tuples.

    When patch_or_None is None, the patch is produced by bsdiffhs.diff.
    When it is bytes, it is used verbatim (hand-crafted vector).
    """
    out: list[tuple[str, bytes, bytes, bytes | None]] = []

    # 1. Identity: src == dst.
    s = b"identity-source-buffer-32-bytes!"
    out.append(("identity", s, s, None))

    # 2. 1->1 byte: smallest non-trivial.
    out.append(("one_byte", b"A", b"B", None))

    # 3. Shrink: target shorter than source.
    s = b"prefix-keep-this-but-drop-the-tail-completely"
    d = b"prefix-keep-this"
    out.append(("shrink", s, d, None))

    # 4. Grow: target longer than source (exercises extra block).
    s = b"short"
    d = b"short-with-a-long-trailing-tail-appended"
    out.append(("grow", s, d, None))

    # 5. Scattered edits across a firmware-shaped blob.
    base = bytes((i * 131 + 7) & 0xFF for i in range(256))
    edited = bytearray(base)
    for off, val in [(3, 0xAA), (37, 0x55), (101, 0x11), (200, 0xFE), (255, 0x00)]:
        edited[off] = val
    out.append(("scattered", bytes(base), bytes(edited), None))

    # 6. Hand-crafted negative source_seek: bypasses bsdiff's algorithmic
    #    choices to guarantee the sign-magnitude decode path is exercised.
    src, dst, patch = synthesize_negative_seek_patch()
    out.append(("negative_seek", src, dst, patch))

    return out


def hexdump(name: str, data: bytes) -> str:
    body = ",\n".join(
        "\t" + ", ".join(f"0x{b:02x}" for b in data[i : i + 12]) for i in range(0, len(data), 12)
    )
    return f"static const uint8_t {name}[] = {{\n{body}\n}};\n"


def main() -> int:
    triples = vectors()
    parts: list[str] = []
    parts.append(
        textwrap.dedent(
            """\
            /*
             * Auto-generated by tests/lib/delta/vectors/gen_vectors.py
             * Copyright (c) 2026 Clovis Corde
             * SPDX-License-Identifier: Apache-2.0
             *
             * Do not edit by hand. Regenerate with:
             *     python3 tests/lib/delta/vectors/gen_vectors.py
             *
             * Header window_sz2 / lookahead_sz2 used for every vector:
             *     window_sz2 = {w}, lookahead_sz2 = {l}
             */

            """
        ).format(w=WINDOW_SZ2, l=LOOKAHEAD_SZ2)
    )

    parts.append("struct delta_test_vector {\n")
    parts.append("\tconst char *name;\n")
    parts.append("\tconst uint8_t *src;\n\tsize_t src_len;\n")
    parts.append("\tconst uint8_t *dst;\n\tsize_t dst_len;\n")
    parts.append("\tconst uint8_t *patch;\n\tsize_t patch_len;\n")
    parts.append("};\n\n")

    for name, src, dst, patch_override in triples:
        if patch_override is None:
            patch = make_patch(src, dst)
            verify(src, dst, patch)
        else:
            patch = patch_override
        parts.append(hexdump(f"vec_{name}_src", src))
        parts.append(hexdump(f"vec_{name}_dst", dst))
        parts.append(hexdump(f"vec_{name}_patch", patch))

    parts.append("static const struct delta_test_vector delta_test_vectors[] = {\n")
    for name, _src, _dst, _p in triples:
        parts.append(
            f"\t{{ \"{name}\","
            f" vec_{name}_src,  sizeof(vec_{name}_src),"
            f" vec_{name}_dst,  sizeof(vec_{name}_dst),"
            f" vec_{name}_patch, sizeof(vec_{name}_patch) }},\n"
        )
    parts.append("};\n")

    with open(OUT, "w", encoding="utf-8") as fp:
        fp.write("".join(parts))

    print(f"Wrote {len(triples)} vectors to {OUT}")
    for name, src, dst, _p in triples:
        print(f"  - {name}: src={len(src)}B dst={len(dst)}B")
    return 0


if __name__ == "__main__":
    sys.exit(main())
