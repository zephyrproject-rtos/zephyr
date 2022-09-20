# ADSP Tools

A library and suite of cli tools for working with Intel ADSP devices from userland
written in Rust.

## Tools

* adsp_loader Firmware loader
* adsp_logger A (winstream) logger reading logs off the DSP

## Why Rust

Well simply... its faster than python while still being easier (mostly) to write than C
or C++, creates rsync/scp friendly binaries, enables code reuse across a suite of tools.

## Building

Install rust with rustup https://rustup.rs

Then run

``` sh
cargo build
```

A few binaries are now in target/debug/ that can be used or run.

Running them with backtraces and logging is useful to see what they
are doing. LOG may be debug, info, warn, or error.

``` sh
sudo RUST_LOG=debug RUST_BACKTRACE=full ./adsp_logger
sudo RUST_LOG=debug RUST_BACKTRACE=full ./adsp_loader zephyr.ri
```

## Testing

A helper script to run the loader/logger remotely exists using ssh
in .cargo/remote_runner.sh which expects an ADSP_HOST environment variable
to point at a host you wish to try the tools on (you can just copy them there too).

The runner expects a zephyr.ri file to exist on the remote host to load and run.
  
  
