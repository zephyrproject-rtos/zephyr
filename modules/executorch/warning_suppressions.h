/*
 * Global warning suppressions for ExecuTorch
 * Copyright (c) 2025 Petri Oksanen
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// Globally suppress the problematic warnings that ExecuTorch generates
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wint-in-bool-context" 