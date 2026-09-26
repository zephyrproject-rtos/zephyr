#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

img_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
output=${2:-"$img_dir/m5stack_coremp135.webp"}

if [[ $# -ge 1 ]]; then
	source_image=$1
else
	scratch_dir=$(mktemp -d)
	trap 'rm -rf -- "$scratch_dir"' EXIT
	source_image=$scratch_dir/source.webp
	curl --fail --location --silent --show-error \
		'https://shop.m5stack.com/cdn/shop/files/3_cf6ad1f3-1874-40fb-9705-d4a8fc5cbd5f_1200x1200.webp' \
		--output "$source_image"
fi

# Crop around the device, then remove only the connected exterior white background.
magick "$source_image" -crop 600x600+100+100 +repage \
	-alpha on -fuzz 12% -fill none -draw 'color 0,0 floodfill' \
	-strip -quality 80 "$output"
