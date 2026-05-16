# Copyright (c) 2026 Mahad Faisal
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import numpy as np
import tensorflow as tf

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

WINDOW_SIZE = 64
NUM_CLASSES = 3
NUM_FEATURES = 8

LABELS = ["normal", "high_frequency", "transient"]


def sine_window(cycles, amp=1000.0, phase=0.0, noise=0.0, transient=False):
    x = np.arange(WINDOW_SIZE, dtype=np.float32)
    y = amp * np.sin((2.0 * np.pi * cycles * x / WINDOW_SIZE) + phase)

    if transient:
        y[16] += 2500.0
        y[48] -= 2300.0

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=WINDOW_SIZE)

    return y.astype(np.float32)


def extract_features(samples):
    samples = samples.astype(np.float32)

    mean = np.mean(samples)
    rms = np.sqrt(np.mean(samples * samples))
    peak = np.max(np.abs(samples))
    zcr = np.mean(np.signbit(samples[:-1]) != np.signbit(samples[1:]))

    centered = samples - mean
    windowed = centered * np.hanning(WINDOW_SIZE).astype(np.float32)
    magnitudes = np.abs(np.fft.rfft(windowed))

    low_energy = np.sum(magnitudes[1:5] ** 2)
    mid_energy = np.sum(magnitudes[5:13] ** 2)
    high_energy = np.sum(magnitudes[13:] ** 2)
    total_energy = low_energy + mid_energy + high_energy + 1.0e-6

    bins = np.arange(len(magnitudes), dtype=np.float32)
    centroid = np.sum(bins[1:] * magnitudes[1:]) / (np.sum(magnitudes[1:]) + 1.0e-6)
    centroid = centroid / (WINDOW_SIZE / 2.0)

    return np.array(
        [
            mean / 4000.0,
            rms / 2000.0,
            peak / 4000.0,
            zcr / 0.50,
            low_energy / total_energy,
            mid_energy / total_energy,
            high_energy / total_energy,
            centroid,
        ],
        dtype=np.float32,
    )


def make_dataset(samples_per_class=800):
    xs = []
    ys = []

    for _ in range(samples_per_class):
        xs.append(
            extract_features(
                sine_window(
                    cycles=np.random.uniform(1.8, 2.4),
                    amp=np.random.uniform(750.0, 1150.0),
                    phase=np.random.uniform(0.0, 2.0 * np.pi),
                    noise=25.0,
                )
            )
        )
        ys.append(0)

        xs.append(
            extract_features(
                sine_window(
                    cycles=np.random.uniform(8.0, 12.0),
                    amp=np.random.uniform(650.0, 1100.0),
                    phase=np.random.uniform(0.0, 2.0 * np.pi),
                    noise=25.0,
                )
            )
        )
        ys.append(1)

        xs.append(
            extract_features(
                sine_window(
                    cycles=np.random.uniform(1.5, 3.0),
                    amp=np.random.uniform(650.0, 1000.0),
                    phase=np.random.uniform(0.0, 2.0 * np.pi),
                    noise=25.0,
                    transient=True,
                )
            )
        )
        ys.append(2)

    return np.stack(xs), np.array(ys, dtype=np.int64)


def eval_windows():
    return [
        ("normal", 0, sine_window(cycles=2.0, amp=1000.0)),
        ("high_frequency", 1, sine_window(cycles=10.0, amp=1000.0)),
        ("transient", 2, sine_window(cycles=2.0, amp=900.0, transient=True)),
    ]


def write_input_data():
    windows = eval_windows()

    arrays = []
    entries = []

    for name, label, samples in windows:
        values = np.round(samples).astype(np.int16)
        body = ", ".join(str(int(v)) for v in values)

        wrapped = []
        parts = body.split(", ")
        for i in range(0, len(parts), 8):
            wrapped.append("\t" + ", ".join(parts[i : i + 8]))

        arrays.append(
            f"static const int16_t {name}_window[FFT_EVENT_WINDOW_SIZE] = {{\n"
            + ",\n".join(wrapped)
            + "\n};\n"
        )

        entries.append(
            "\t{\n"
            f'\t\t.name = "{name}",\n'
            f"\t\t.label = FFT_EVENT_{label_name_c(label)},\n"
            f"\t\t.samples = {name}_window,\n"
            "\t\t.len = FFT_EVENT_WINDOW_SIZE,\n"
            "\t}"
        )

    text = """/*
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dsp_features.h"
#include "input_data.h"

#include <stddef.h>
#include <stdint.h>

{}
static const struct fft_event_window windows[FFT_EVENT_NUM_CLASSES] = {{
{}
}};

const struct fft_event_window *fft_event_get_window(size_t index)
{{
return &windows[index % FFT_EVENT_NUM_CLASSES];
}}

const char *fft_event_label_name(enum fft_event_label label)
{{
switch (label) {{
case FFT_EVENT_NORMAL:
return "normal";
case FFT_EVENT_HIGH_FREQUENCY:
return "high_frequency";
case FFT_EVENT_TRANSIENT:
return "transient";
default:
return "unknown";
}}
}}
""".format("\n".join(arrays), ",\n".join(entries))

    (SRC / "input_data.c").write_text(text, newline="\n")


def label_name_c(label):
    return {
        0: "NORMAL",
        1: "HIGH_FREQUENCY",
        2: "TRANSIENT",
    }[label]


def write_model_cpp(model_bytes):
    items = [f"0x{b:02x}" for b in model_bytes]
    wrapped = []

    for i in range(0, len(items), 12):
        wrapped.append("\t" + ", ".join(items[i : i + 12]))

    model_cpp = """/*
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "model.hpp"

#include <cstdint>

alignas(16) const unsigned char g_model[] = {{
{}
}};

const int g_model_len = sizeof(g_model);
""".format(",\n".join(wrapped))

    model_hpp = """/*
 * Copyright (c) 2026 Mahad Faisal
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

extern const unsigned char g_model[];
extern const int g_model_len;
"""

    (SRC / "model.cpp").write_text(model_cpp, newline="\n")
    (SRC / "model.hpp").write_text(model_hpp, newline="\n")


def main():
    np.random.seed(11)
    tf.random.set_seed(11)

    x_train, y_train = make_dataset()

    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(NUM_FEATURES,)),
            tf.keras.layers.Dense(12, activation="relu"),
            tf.keras.layers.Dense(NUM_CLASSES),
        ]
    )

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.01),
        loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
        metrics=["accuracy"],
    )

    model.fit(x_train, y_train, epochs=35, batch_size=32, verbose=0)

    _, acc = model.evaluate(x_train, y_train, verbose=0)
    print(f"train_acc={acc:.4f}")

    for name, label, samples in eval_windows():
        prediction = np.argmax(model.predict(extract_features(samples)[None, :], verbose=0)[0])
        print(f"eval_window={name}, expected={LABELS[label]}, predicted={LABELS[prediction]}")

    def representative_dataset():
        for i in range(min(300, len(x_train))):
            yield [x_train[i : i + 1].astype(np.float32)]

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
