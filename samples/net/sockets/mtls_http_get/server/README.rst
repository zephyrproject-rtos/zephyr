Mutual TLS auth example server
=============================

This requires Rust.

Run with:

    cargo run

Connect with:

    curl -v --cacert ca.crt --key client.key --cert client.crt https://localhost:8443

