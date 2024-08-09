// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

include!(env!("RUST_DTS"));

pub const fn root() -> &'static DtNode0 { &DT_NODE_0 }

#[macro_export]
macro_rules! dt_alias {
    ($alias:ident) => { $crate::devicetree::root().aliases.$alias };
}

#[macro_export]
macro_rules! device_dt_get {
    ($node:expr) => { $node.device() };
}
