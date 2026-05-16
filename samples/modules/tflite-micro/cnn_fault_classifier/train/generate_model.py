# SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

from pathlib import Path

import numpy as np
import tensorflow as tf

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

WINDOW_SIZE = 64
NUM_CLASSES = 3
LABELS = ["normal", "high_frequency_fault", "impulse_fault"]


def normal_window(amp: float = 1000.0, phase: float = 0.0, noise: float = 0.0):
    x = np.arange(WINDOW_SIZE, dtype=np.float32)
    y = amp * np.sin((2.0 * np.pi * 2.0 * x / WINDOW_SIZE) + phase)

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=WINDOW_SIZE)

    return y.astype(np.float32)


def high_frequency_window(
    amp: float = 900.0,
    phase: float = 0.0,
    noise: float = 0.0,
):
    x = np.arange(WINDOW_SIZE, dtype=np.float32)
    y = amp * np.sin((2.0 * np.pi * 9.0 * x / WINDOW_SIZE) + phase)

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=WINDOW_SIZE)

    return y.astype(np.float32)


def impulse_window(amp: float = 800.0, noise: float = 0.0):
    x = np.arange(WINDOW_SIZE, dtype=np.float32)
    y = amp * np.sin(2.0 * np.pi * 2.0 * x / WINDOW_SIZE)
    y[16] += 2600.0
    y[47] -= 2400.0

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=WINDOW_SIZE)

    return y.astype(np.float32)


def normalize_window(samples: np.ndarray):
    return (samples / 3500.0).astype(np.float32)


def make_dataset(samples_per_class: int = 800):
    xs = []
    ys = []

    for _ in range(samples_per_class):
        windows = [
            normal_window(
                amp=np.random.uniform(850.0, 1150.0),
                phase=np.random.uniform(0.0, 2.0 * np.pi),
                noise=25.0,
            ),
            high_frequency_window(
                amp=np.random.uniform(750.0, 1050.0),
                phase=np.random.uniform(0.0, 2.0 * np.pi),
                noise=25.0,
            ),
            impulse_window(
                amp=np.random.uniform(650.0, 900.0),
                noise=25.0,
            ),
        ]

        for label, window in enumerate(windows):
            xs.append(normalize_window(window).reshape(WINDOW_SIZE, 1, 1))
            target = np.zeros((1, 1, NUM_CLASSES), dtype=np.float32)
            target[0, 0, label] = 1.0
            ys.append(target)

    return np.stack(xs), np.stack(ys).astype(np.float32)


def eval_windows():
    return [
        ("normal", 0, normal_window(amp=1000.0)),
        ("high_frequency_fault", 1, high_frequency_window(amp=900.0)),
        ("impulse_fault", 2, impulse_window(amp=800.0)),
    ]


def label_symbol(label: int):
    return {
        0: "CNN_FAULT_NORMAL",
        1: "CNN_FAULT_HIGH_FREQUENCY",
        2: "CNN_FAULT_IMPULSE",
    }[label]


def write_input_data():
    arrays = []
    entries = []

    for name, label, samples in eval_windows():
        array_name = f"{name}_window"
        values = np.round(samples).astype(np.int16)
        parts = [str(int(value)) for value in values]
        lines = []

        for index in range(0, len(parts), 8):
            lines.append("\t" + ", ".join(parts[index : index + 8]))

        arrays.append(
            f"static const int16_t {array_name}[CNN_FAULT_WINDOW_SIZE] = {{\n"
            + ",\n".join(lines)
            + "\n};\n"
        )
        entries.append(
            "\t{\n"
            f'\t\t.name = "{name}",\n'
            f"\t\t.label = {label_symbol(label)},\n"
            f"\t\t.samples = {array_name},\n"
            "\t\t.len = CNN_FAULT_WINDOW_SIZE,\n"
            "\t}"
        )

    array_body = "\n".join(arrays)
    entries_body = ",\n".join(entries)

    text = f"""/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "input_data.h"

#include <stddef.h>
#include <stdint.h>

{array_body}
static const struct cnn_fault_window windows[CNN_FAULT_NUM_CLASSES] = {{
{entries_body}
}};

const struct cnn_fault_window *cnn_fault_get_window(size_t index)
{{
return &windows[index % CNN_FAULT_NUM_CLASSES];
}}

const char *cnn_fault_label_name(enum cnn_fault_label label)
{{
switch (label) {{
case CNN_FAULT_NORMAL:
return "normal";
case CNN_FAULT_HIGH_FREQUENCY:
return "high_frequency_fault";
case CNN_FAULT_IMPULSE:
return "impulse_fault";
default:
return "unknown";
}}
}}
"""

    (SRC / "input_data.c").write_text(text, newline="\n")


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
    np.random.seed(29)
    tf.random.set_seed(29)

    x_train, y_train = make_dataset()

    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(WINDOW_SIZE, 1, 1)),
            tf.keras.layers.Conv2D(
                4,
                kernel_size=(5, 1),
                padding="same",
                activation="relu",
            ),
            tf.keras.layers.Conv2D(
                NUM_CLASSES,
                kernel_size=(WINDOW_SIZE, 1),
                padding="valid",
            ),
        ]
    )

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.01),
        loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True),
        metrics=["accuracy"],
    )

    model.fit(x_train, y_train, epochs=30, batch_size=32, verbose=0)
    _, acc = model.evaluate(x_train, y_train, verbose=0)
    print(f"train_acc={acc:.4f}")

    for name, label, samples in eval_windows():
        x_eval = normalize_window(samples).reshape(1, WINDOW_SIZE, 1, 1)
        output = model.predict(x_eval, verbose=0)[0, 0, 0, :]
        prediction = int(np.argmax(output))
        print(f"eval_window={name}, expected={LABELS[label]}, predicted={LABELS[prediction]}")

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

    write_input_data()
    write_model_cpp(tflite_model)


if __name__ == "__main__":
    main()
