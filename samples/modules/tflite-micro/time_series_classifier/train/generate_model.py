# SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import numpy as np
import tensorflow as tf

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

WINDOW_SIZE = 64
NUM_CLASSES = 3

LABELS = ["normal", "low_frequency", "transient"]


def extract_features(samples: np.ndarray) -> np.ndarray:
    samples = samples.astype(np.float32)
    mean = np.mean(samples)
    rms = np.sqrt(np.mean(samples * samples))
    peak = np.max(np.abs(samples))
    zcr = np.mean(np.signbit(samples[:-1]) != np.signbit(samples[1:]))

    return np.array(
        [
            mean / 3200.0,
            rms / 1200.0,
            peak / 3200.0,
            zcr / 0.10,
        ],
        dtype=np.float32,
    )


def sine_window(cycles: float, amp: float = 1000.0, spike: bool = False) -> np.ndarray:
    x = np.arange(WINDOW_SIZE, dtype=np.float32)
    y = amp * np.sin(2.0 * np.pi * cycles * x / WINDOW_SIZE)

    if spike:
        y[16] += 2200.0
        y[48] -= 1800.0

    noise = np.random.normal(0.0, 25.0, size=WINDOW_SIZE)
    return y + noise


def make_dataset(n: int = 600):
    xs = []
    ys = []

    for _ in range(n):
        xs.append(extract_features(sine_window(cycles=2.0, amp=np.random.uniform(850, 1100))))
        ys.append(0)

        xs.append(extract_features(sine_window(cycles=1.0, amp=np.random.uniform(850, 1100))))
        ys.append(1)

        xs.append(
            extract_features(sine_window(cycles=1.0, amp=np.random.uniform(850, 1100), spike=True))
        )
        ys.append(2)

    return np.stack(xs), np.array(ys, dtype=np.int64)


def write_model_cpp(model_bytes: bytes):
    items = [f"0x{byte:02x}" for byte in model_bytes]
    wrapped = []

    for index in range(0, len(items), 12):
        wrapped.append("\t" + ", ".join(items[index : index + 12]))

    model_body = ",\n".join(wrapped)

    model_cpp = f"""/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model.hpp"

#include <cstdint>

alignas(16) const unsigned char g_model[] = {{
{model_body}
}};

const int g_model_len = sizeof(g_model);
"""

    model_hpp = """/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

extern const unsigned char g_model[];
extern const int g_model_len;
"""

    (SRC / "model.cpp").write_text(model_cpp, newline="\n")
    (SRC / "model.hpp").write_text(model_hpp, newline="\n")


def main():
    np.random.seed(7)
    tf.random.set_seed(7)

    x_train, y_train = make_dataset()

    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(4,)),
            tf.keras.layers.Dense(8, activation="relu"),
            tf.keras.layers.Dense(NUM_CLASSES),
        ]
    )

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.01),
        loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
        metrics=["accuracy"],
    )

    _, acc = model.evaluate(x_train, y_train, verbose=0)
    model.fit(x_train, y_train, epochs=40, batch_size=32, verbose=0)
    _, acc = model.evaluate(x_train, y_train, verbose=0)
    print(f"train_acc={acc:.4f}")

    def representative_dataset():
        for index in range(min(300, len(x_train))):
            yield [x_train[index : index + 1].astype(np.float32)]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_model = converter.convert()
    print(f"model_size={len(tflite_model)} bytes")

    write_model_cpp(tflite_model)


if __name__ == "__main__":
    main()
