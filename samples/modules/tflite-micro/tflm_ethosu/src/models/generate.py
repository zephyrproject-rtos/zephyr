#!/usr/bin/env python3
#
# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

"""Generate the sample's model headers.

Each model is built as an int8 TFLite flatbuffer with the TFLite schema
bindings shipped with Vela, so TensorFlow is not needed. The expected output
is computed with TFLite's int8 arithmetic, independently of TFLM and the NPU.
Vela then compiles the model for each entry in NPU_CONFIGS. Each model gets one
header with the plain model, the Vela-compiled variants, the input and the
expected output.

Usage: generate.py [--vela VELA] [OUTPUT_DIR]
"""

import argparse
import math
import os
import subprocess
import tempfile

import flatbuffers
import numpy as np
from ethosu.vela.tflite import (
    Buffer,
    Conv2DOptions,
    FullyConnectedOptions,
    Model,
    Operator,
    OperatorCode,
    QuantizationParameters,
    SubGraph,
    Tensor,
)
from ethosu.vela.tflite.ActivationFunctionType import ActivationFunctionType
from ethosu.vela.tflite.BuiltinOperator import BuiltinOperator
from ethosu.vela.tflite.BuiltinOptions import BuiltinOptions
from ethosu.vela.tflite.Padding import Padding
from ethosu.vela.tflite.TensorType import TensorType

# Kconfig symbol, Vela accelerator config and Vela system config
NPU_CONFIGS = [
    ("CONFIG_ETHOS_U55_128", "ethos-u55-128", "Ethos_U55_High_End_Embedded"),
    ("CONFIG_ETHOS_U85_256", "ethos-u85-256", "Ethos_U85_SYS_DRAM_Mid"),
]
MEMORY_MODE = "Shared_Sram"

# TFLite int8 reference arithmetic (tensorflow/lite/kernels/internal/common.h)


def quantize_multiplier(real):
    """Return the Q31 multiplier and shift that represent a real multiplier."""
    q, shift = math.frexp(real)
    q_fixed = round(q * (1 << 31))
    if q_fixed == (1 << 31):
        q_fixed //= 2
        shift += 1
    return q_fixed, shift


def saturating_rounding_doubling_high_mul(a, b):
    """Model gemmlowp's SaturatingRoundingDoublingHighMul() on int32 values."""
    if a == b == -(1 << 31):
        return (1 << 31) - 1
    ab = a * b
    nudge = (1 << 30) if ab >= 0 else 1 - (1 << 30)
    v = ab + nudge
    # C integer division truncates toward zero
    return abs(v) >> 31 if v >= 0 else -(abs(v) >> 31)


def rounding_divide_by_pot(x, exponent):
    """Model gemmlowp's RoundingDivideByPOT()."""
    mask = (1 << exponent) - 1
    threshold = (mask >> 1) + (1 if x < 0 else 0)
    return (x >> exponent) + (1 if (x & mask) > threshold else 0)


def requantize(acc, real_multiplier, zero_point, relu):
    """Scale an int32 accumulator to int8 like MultiplyByQuantizedMultiplier()."""
    q_fixed, shift = quantize_multiplier(real_multiplier)
    v = saturating_rounding_doubling_high_mul(acc * (1 << max(shift, 0)), q_fixed)
    v = rounding_divide_by_pot(v, max(-shift, 0)) + zero_point
    return max(zero_point if relu else -128, min(127, v))


class Graph:
    """Single subgraph int8 TFLite model under construction."""

    def __init__(self):
        self.tensors = []
        self.ops = []
        self.opcodes = []

    def tensor(self, name, shape, ttype, scale, zero_point, data=None):
        """Add a tensor and return its index. A list of scales means per-channel."""
        self.tensors.append(
            {
                "name": name,
                "shape": list(shape),
                "type": ttype,
                "scale": [float(s) for s in np.ravel(scale)],
                "zero_point": [int(z) for z in np.ravel(zero_point)],
                "data": data,
            }
        )
        return len(self.tensors) - 1

    def operator(self, opcode, inputs, outputs, options_type, options):
        """Add an operator. options(builder) adds its option table and returns its offset."""
        if opcode not in self.opcodes:
            self.opcodes.append(opcode)
        self.ops.append((self.opcodes.index(opcode), inputs, outputs, options_type, options))

    def serialize(self, inputs, outputs):
        """Return the model as a TFLite flatbuffer."""
        b = flatbuffers.Builder(4096)

        def vector(values, size, prepend):
            b.StartVector(size, len(values), size)
            for v in reversed(values):
                prepend(v)
            return b.EndVector()

        def table_vector(offsets):
            return vector(offsets, 4, b.PrependUOffsetTRelative)

        # Buffer 0 is the empty buffer that tensors without data refer to.
        buffers = [None] + [t["data"] for t in self.tensors if t["data"] is not None]
        buffer_offsets = []
        for data in buffers:
            raw = None if data is None else vector(list(data.tobytes()), 1, b.PrependUint8)
            Buffer.BufferStart(b)
            if raw is not None:
                Buffer.BufferAddData(b, raw)
            buffer_offsets.append(Buffer.BufferEnd(b))

        tensor_offsets = []
        next_buffer = 1
        for t in self.tensors:
            name = b.CreateString(t["name"])
            shape = vector(t["shape"], 4, b.PrependInt32)
            scale = vector(t["scale"], 4, b.PrependFloat32)
            zero_point = vector(t["zero_point"], 8, b.PrependInt64)
            QuantizationParameters.QuantizationParametersStart(b)
            QuantizationParameters.QuantizationParametersAddScale(b, scale)
            QuantizationParameters.QuantizationParametersAddZeroPoint(b, zero_point)
            quant = QuantizationParameters.QuantizationParametersEnd(b)
            Tensor.TensorStart(b)
            Tensor.TensorAddShape(b, shape)
            Tensor.TensorAddType(b, t["type"])
            if t["data"] is not None:
                Tensor.TensorAddBuffer(b, next_buffer)
                next_buffer += 1
            Tensor.TensorAddName(b, name)
            Tensor.TensorAddQuantization(b, quant)
            tensor_offsets.append(Tensor.TensorEnd(b))

        op_offsets = []
        for opcode_index, op_inputs, op_outputs, options_type, options in self.ops:
            opts = options(b)
            ins = vector(op_inputs, 4, b.PrependInt32)
            outs = vector(op_outputs, 4, b.PrependInt32)
            Operator.OperatorStart(b)
            Operator.OperatorAddOpcodeIndex(b, opcode_index)
            Operator.OperatorAddInputs(b, ins)
            Operator.OperatorAddOutputs(b, outs)
            Operator.OperatorAddBuiltinOptionsType(b, options_type)
            Operator.OperatorAddBuiltinOptions(b, opts)
            op_offsets.append(Operator.OperatorEnd(b))

        tensors = table_vector(tensor_offsets)
        ops = table_vector(op_offsets)
        ins = vector(inputs, 4, b.PrependInt32)
        outs = vector(outputs, 4, b.PrependInt32)
        SubGraph.SubGraphStart(b)
        SubGraph.SubGraphAddTensors(b, tensors)
        SubGraph.SubGraphAddInputs(b, ins)
        SubGraph.SubGraphAddOutputs(b, outs)
        SubGraph.SubGraphAddOperators(b, ops)
        subgraph = SubGraph.SubGraphEnd(b)

        opcode_offsets = []
        for opcode in self.opcodes:
            OperatorCode.OperatorCodeStart(b)
            OperatorCode.OperatorCodeAddDeprecatedBuiltinCode(b, opcode)
            OperatorCode.OperatorCodeAddBuiltinCode(b, opcode)
            OperatorCode.OperatorCodeAddVersion(b, 1)
            opcode_offsets.append(OperatorCode.OperatorCodeEnd(b))

        opcodes = table_vector(opcode_offsets)
        subgraphs = table_vector([subgraph])
        buffer_vector = table_vector(buffer_offsets)
        Model.ModelStart(b)
        Model.ModelAddVersion(b, 3)
        Model.ModelAddOperatorCodes(b, opcodes)
        Model.ModelAddSubgraphs(b, subgraphs)
        Model.ModelAddBuffers(b, buffer_vector)
        b.Finish(Model.ModelEnd(b), file_identifier=b"TFL3")
        return bytes(b.Output())


def activation(relu):
    """Return the TFLite fused activation code."""
    return ActivationFunctionType.RELU if relu else ActivationFunctionType.NONE


def fully_connected(g, x_index, x, weights, w_scale, out_scale, relu, name):
    """Add a FULLY_CONNECTED layer with zero bias.

    weights is int8 [outputs, inputs]. Returns the output tensor index and
    the reference output for input x.
    """
    x_t = g.tensors[x_index]
    s_in, z_in = x_t["scale"][0], x_t["zero_point"][0]
    w = g.tensor(
        name + "_weights", weights.shape, TensorType.INT8, w_scale, 0, weights.astype(np.int8)
    )
    bias = g.tensor(
        name + "_bias",
        [weights.shape[0]],
        TensorType.INT32,
        s_in * w_scale,
        0,
        np.zeros(weights.shape[0], np.int32),
    )
    out = g.tensor(name, [1, weights.shape[0]], TensorType.INT8, out_scale, 0)

    def options(b):
        FullyConnectedOptions.FullyConnectedOptionsStart(b)
        FullyConnectedOptions.FullyConnectedOptionsAddFusedActivationFunction(b, activation(relu))
        return FullyConnectedOptions.FullyConnectedOptionsEnd(b)

    g.operator(
        BuiltinOperator.FULLY_CONNECTED,
        [x_index, w, bias],
        [out],
        BuiltinOptions.FullyConnectedOptions,
        options,
    )
    multiplier = s_in * w_scale / out_scale
    acc = weights.astype(np.int64) @ (x.astype(np.int64) - z_in)
    y = np.array([requantize(int(a), multiplier, 0, relu) for a in acc], np.int8)
    return out, y


def conv2d(g, x_index, x, weights, bias, out_zero_point, relu, name):
    """Add a CONV_2D layer, stride 1, SAME padding, all scales 1.0.

    weights is int8 [out_channels, height, width, in_channels] and x is
    int8 [height, width, channels]. Returns the output tensor index and
    the reference output for input x.
    """
    x_t = g.tensors[x_index]
    z_in = x_t["zero_point"][0]
    channels, kh, kw, _ = weights.shape
    height, width, _ = x.shape
    w = g.tensor(
        name + "_weights",
        weights.shape,
        TensorType.INT8,
        [1.0] * channels,
        [0] * channels,
        weights.astype(np.int8),
    )
    b = g.tensor(
        name + "_bias",
        [channels],
        TensorType.INT32,
        [1.0] * channels,
        [0] * channels,
        bias.astype(np.int32),
    )
    out = g.tensor(name, [1, height, width, channels], TensorType.INT8, 1.0, out_zero_point)

    def options(fb):
        Conv2DOptions.Conv2DOptionsStart(fb)
        Conv2DOptions.Conv2DOptionsAddPadding(fb, Padding.SAME)
        Conv2DOptions.Conv2DOptionsAddStrideW(fb, 1)
        Conv2DOptions.Conv2DOptionsAddStrideH(fb, 1)
        Conv2DOptions.Conv2DOptionsAddDilationWFactor(fb, 1)
        Conv2DOptions.Conv2DOptionsAddDilationHFactor(fb, 1)
        Conv2DOptions.Conv2DOptionsAddFusedActivationFunction(fb, activation(relu))
        return Conv2DOptions.Conv2DOptionsEnd(fb)

    g.operator(
        BuiltinOperator.CONV_2D, [x_index, w, b], [out], BuiltinOptions.Conv2DOptions, options
    )
    padded = np.pad(x.astype(np.int64) - z_in, ((kh // 2,), (kw // 2,), (0,)))
    y = np.zeros((height, width, channels), np.int8)
    for c in range(channels):
        for r in range(height):
            for col in range(width):
                window = padded[r : r + kh, col : col + kw, :]
                acc = int(np.sum(window * weights[c].astype(np.int64))) + int(bias[c])
                y[r, col, c] = requantize(acc, 1.0, out_zero_point, relu)
    return out, y


def dft_model():
    """16-point DFT of a noisy two-tone signal, as one fully connected layer.

    The output holds the 16 real parts, then the 16 imaginary parts. The
    tones put peaks in bins 3 and 13 (real) and 5 and 11 (imaginary).
    """
    n = 16
    k = np.arange(n)[:, None] * np.arange(n)[None, :]
    basis = np.vstack([np.cos(2 * np.pi * k / n), -np.sin(2 * np.pi * k / n)])
    w_scale = 1.0 / 127
    weights = np.round(basis / w_scale).astype(np.int64)

    s_in = 1.0 / 100
    t = np.arange(n)
    signal = np.cos(2 * np.pi * 3 * t / n) + 0.25 * np.sin(2 * np.pi * 5 * t / n)
    noise = np.random.default_rng(1).integers(-12, 13, n)
    x = (np.round(signal / s_in) + noise).astype(np.int8)

    g = Graph()
    x_index = g.tensor("signal", [1, n], TensorType.INT8, s_in, 0)
    # The output scale covers +-10. A unit amplitude tone peaks at n / 2 = 8.
    out, y = fully_connected(g, x_index, x, weights, w_scale, 10.0 / 127, False, "spectrum")
    return g.serialize([x_index], [out]), x, y


def life_step(board):
    """Advance a Game of Life board by one generation, outside cells dead."""
    padded = np.pad(board, 1)
    h, w = board.shape
    neighbors = sum(
        padded[1 + dr : 1 + dr + h, 1 + dc : 1 + dc + w]
        for dr in (-1, 0, 1)
        for dc in (-1, 0, 1)
        if (dr, dc) != (0, 0)
    )
    return ((neighbors == 3) | ((board == 1) & (neighbors == 2))).astype(np.int64)


def life_model(generations=4, size=8):
    """Conway's Game of Life on an 8x8 board, 2 convolutions per generation.

    The first 3x3 convolution computes v = 2 * neighbors + cell, which is 5,
    6 or 7 exactly when the cell lives in the next generation. Its 4 output
    channels hold relu(v - 4), relu(v - 5), relu(v - 7) and relu(v - 8). The
    1x1 convolution then adds them with weights 1, -1, -1 and 1, which gives
    1 for v in 5..7 and 0 otherwise. All scales are 1.0, so the result is
    exact. The input is a glider, which moves one cell diagonally every 4
    generations.
    """
    kernel = np.full((3, 3), 2)
    kernel[1, 1] = 1
    count_weights = np.stack([kernel] * 4)[..., None]
    count_bias = np.array([-4, -5, -7, -8])
    rule_weights = np.array([1, -1, -1, 1]).reshape((1, 1, 1, 4))

    board = np.zeros((size, size), np.int64)
    for r, c in ((0, 1), (1, 2), (2, 0), (2, 1), (2, 2)):
        board[r, c] = 1

    g = Graph()
    x_index = g.tensor("board", [1, size, size, 1], TensorType.INT8, 1.0, 0)
    t, y = x_index, board.astype(np.int8)[..., None]
    for i in range(generations):
        t, y = conv2d(g, t, y, count_weights, count_bias, -128, True, f"gen{i}_count")
        t, y = conv2d(g, t, y, rule_weights, np.zeros(1), 0, False, f"gen{i}")

    expected = board
    for _ in range(generations):
        expected = life_step(expected)
    if not np.array_equal(y[..., 0].astype(np.int64), expected):
        raise RuntimeError("Game of Life model does not follow the rules")
    return g.serialize([x_index], [t]), board.astype(np.int8).ravel(), y.ravel()


MODELS = {"dft": dft_model, "life": life_model}


def vela_compile(vela, tflite, accelerator, system_config):
    """Compile a model with Vela and return the result."""
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "model.tflite")
        with open(src, "wb") as f:
            f.write(tflite)
        subprocess.run(
            [
                vela,
                src,
                "--output-dir",
                tmp,
                "--accelerator-config",
                accelerator,
                "--system-config",
                system_config,
                "--memory-mode",
                MEMORY_MODE,
            ],
            check=True,
            stdout=subprocess.DEVNULL,
        )
        with open(os.path.join(tmp, "model_vela.tflite"), "rb") as f:
            return f.read()


def c_bytes(data):
    """Format bytes as a C initializer, 16 per line to fit in 100 columns."""
    if not isinstance(data, bytes):
        data = np.asarray(data, np.int8).tobytes()
    values = [f"0x{v:02x}" for v in data]
    return ",\n".join(", ".join(values[i : i + 16]) for i in range(0, len(values), 16))


def header(name, vela_version, tflite, compiled, x, y):
    """Return the C header for one model."""
    out = [
        "/*",
        " * Copyright (c) 2026 BayLibre SAS",
        " * SPDX-License-Identifier: Apache-2.0",
        " */",
        "",
        f"/* Generated by generate.py with Vela {vela_version}. Do not edit. */",
        "",
        "/* clang-format off */",
        "",
        f"MODEL_ATTR uint8_t {name}_model[] = {{",
        "#if !defined(CONFIG_TAINT_BLOBS_TFLM_ETHOSU)",
        c_bytes(tflite),
    ]
    for symbol, blob in compiled:
        out += [f"#elif defined({symbol})", c_bytes(blob)]
    out += [
        "#endif",
        "};",
        "",
        f"INPUT_ATTR uint8_t {name}_input[] = {{",
        c_bytes(x),
        "};",
        "",
        f"OUTPUT_ATTR uint8_t {name}_expected_output[] = {{",
        c_bytes(y),
        "};",
        "",
        "/* clang-format on */",
        "",
    ]
    return "\n".join(out)


def main():
    """Write one header per model."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0], allow_abbrev=False)
    parser.add_argument("--vela", default="vela", help="Vela executable")
    parser.add_argument("output_dir", nargs="?", default=os.path.dirname(__file__))
    args = parser.parse_args()

    vela_version = subprocess.run(
        [args.vela, "--version"], check=True, capture_output=True, text=True
    ).stdout.strip()
    for name, build in MODELS.items():
        tflite, x, y = build()
        compiled = [
            (symbol, vela_compile(args.vela, tflite, accel, system))
            for symbol, accel, system in NPU_CONFIGS
        ]
        with open(os.path.join(args.output_dir, name + ".h"), "w") as f:
            f.write(header(name, vela_version, tflite, compiled, x, y))


if __name__ == "__main__":
    main()
