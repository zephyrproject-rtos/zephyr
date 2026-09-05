#!/bin/sh
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Download the third party browser libraries that the dashboard uses for the
# host console and the temperature graph. They are not distributed with
# Zephyr, see README.md.

set -eu

vendor_dir="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)/vendor"
base_url="https://cdn.jsdelivr.net/npm"

# Pinned as "package path|sha256", one asset per two source lines. The name to
# save the asset under is the last element of the package path.
assets="
xterm@5.3.0/css/xterm.min.css|\
64ee6c4db69b4224d3362aced0fd4cdd620e0e60b3d01566450ae2d4b9e81849
xterm@5.3.0/lib/xterm.min.js|\
fc1dd31b221e3e5f929486e07a80b477a8aaf9dce2b4f9c3ffe7dd25f370655d
xterm-addon-attach@0.9.0/lib/xterm-addon-attach.min.js|\
5997ddeccd2a8285909fb9101f81d0bd3cd582f4287c0217de03960720852f88
xterm-addon-fit@0.8.0/lib/xterm-addon-fit.min.js|\
c78e5795c6487acdf24ab436798d4a9cec3848101b281bc65b061f39db714be1
chart.js@4.5.1/dist/chart.umd.min.js|\
48444a82d4edcb5bec0f1965faacdde18d9c17db3063d042abada2f705c9f54a
"

download()
{
	if command -v curl >/dev/null 2>&1; then
		curl -sSfL -o "$1" "$2"
	elif command -v wget >/dev/null 2>&1; then
		wget -q -O "$1" "$2"
	else
		echo "Need either curl or wget to download the assets" >&2
		exit 1
	fi
}

checksum()
{
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | cut -d ' ' -f 1
	elif command -v shasum >/dev/null 2>&1; then
		shasum -a 256 "$1" | cut -d ' ' -f 1
	else
		echo "Need either sha256sum or shasum to verify the assets" >&2
		exit 1
	fi
}

mkdir -p "$vendor_dir"

for asset in $assets; do
	path="${asset%|*}"
	want="${asset##*|}"
	name="${path##*/}"
	out="$vendor_dir/$name"

	if [ -f "$out" ] && [ "$(checksum "$out")" = "$want" ]; then
		echo "$name: up to date"
		continue
	fi

	echo "$name: downloading"
	download "$out.tmp" "$base_url/$path"

	got="$(checksum "$out.tmp")"
	if [ "$got" != "$want" ]; then
		rm -f "$out.tmp"
		echo "$name: checksum mismatch" >&2
		echo "  expected $want" >&2
		echo "  got      $got" >&2
		exit 1
	fi

	mv "$out.tmp" "$out"
done

echo "Assets are in $vendor_dir, rebuild the sample to serve them."
