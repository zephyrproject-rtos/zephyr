<!--
SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
SPDX-License-Identifier: Apache-2.0
-->

# Dashboard assets

Static assets served by the BMC sample dashboard.

**index.html** - the dashboard itself. Gzipped at build time and served from
`/` by `src/webui.c`.

**logo.jpeg** - WallaBMC logo. Licensed under Apache-2.0. See the repository
root `LICENSE-DOCS` file for details.

**favicon.png** - dashboard icon.

## Vendor assets

The host console and the temperature graph need [xterm.js][xterm] and
[Chart.js][chartjs], both MIT licensed. They are not distributed with Zephyr,
and the dashboard does not fetch them from a CDN because a BMC is typically on
an isolated management network. Instead they are an optional build input, and
the BMC serves them itself from `/vendor/`.

Download them into a `vendor/` subdirectory of this one:

    ./fetch_vendor.sh

The script pins the versions and verifies a SHA-256 for each file. It writes:

    vendor/xterm.min.css
    vendor/xterm.min.js
    vendor/xterm-addon-attach.min.js
    vendor/xterm-addon-fit.min.js
    vendor/chart.umd.min.js

Rebuild the sample afterwards, the build picks the files up automatically and
gzips them. The directory is ignored by Git, so the downloads stay out of the
tree.

When the files are absent, the dashboard builds and runs without them, the
build prints a warning and the affected panels say what is missing. This only
affects how the browser renders the host console. The BMC side of it, the
websocket bridge selected by `CONFIG_BMC_CONSOLE_BRIDGE_WS`, is built either
way and can be used with any websocket client.

[xterm]: https://xtermjs.org/
[chartjs]: https://www.chartjs.org/
