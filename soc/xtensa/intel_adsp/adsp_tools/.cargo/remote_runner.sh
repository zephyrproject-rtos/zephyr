#!/bin/sh

scp target/debug/adsp_loader $ADSP_HOST:.
scp target/debug/adsp_logger $ADSP_HOST:.
ssh $ADSP_HOST "RUST_BACKTRACE=1 RUST_LOG=debug sudo ./adsp_logger && ./adsp_loader zephyr.ri"
