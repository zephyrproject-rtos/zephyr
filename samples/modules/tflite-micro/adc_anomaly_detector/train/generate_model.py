# SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=duplicate-code

from pathlib import Path

import numpy as np
import tensorflow as tf

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

WINDOW_SIZE = 64
HOP_SIZE = 32
SAMPLES_PER_CLASS = WINDOW_SIZE + HOP_SIZE
NUM_CLASSES = 3
NUM_FEATURES = 6

LABELS = ["normal", "drift", "transient"]


def normal_stream(amp: float = 1000.0, phase: float = 0.0, noise: float = 0.0):
    x = np.arange(SAMPLES_PER_CLASS, dtype=np.float32)
    y = amp * np.sin((2.0 * np.pi * 2.0 * x / WINDOW_SIZE) + phase)

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=SAMPLES_PER_CLASS)

    return y.astype(np.float32)


def drift_stream(amp: float = 650.0, drift: float = 1200.0, noise: float = 0.0):
    x = np.arange(SAMPLES_PER_CLASS, dtype=np.float32)
    base = amp * np.sin(2.0 * np.pi * x / WINDOW_SIZE)
    ramp = np.linspace(0.0, drift, SAMPLES_PER_CLASS, dtype=np.float32)
    y = base + ramp

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=SAMPLES_PER_CLASS)

    return y.astype(np.float32)


def transient_stream(amp: float = 800.0, noise: float = 0.0):
    x = np.arange(SAMPLES_PER_CLASS, dtype=np.float32)
    y = amp * np.sin(2.0 * np.pi * 2.0 * x / WINDOW_SIZE)
    y[20] += 2500.0
    y[72] -= 2300.0

    if noise > 0.0:
        y += np.random.normal(0.0, noise, size=SAMPLES_PER_CLASS)

    return y.astype(np.float32)


def extract_features(samples: np.ndarray):
    samples = samples.astype(np.float32)

    mean = np.mean(samples)
    rms = np.sqrt(np.mean(samples * samples))
    peak = np.max(np.abs(samples))
    zcr = np.mean(np.signbit(samples[:-1]) != np.signbit(samples[1:]))
    slope = (samples[-1] - samples[0]) / len(samples)
    energy = np.sum(samples * samples)

    return np.array(
        [
            mean / 3000.0,
            rms / 2000.0,
            peak / 4000.0,
            zcr / 0.20,
            slope / 100.0,
            energy / 200000000.0,
        ],
        dtype=np.float32,
    )


def stream_windows(stream):
    return [
        stream[:WINDOW_SIZE],
        stream[HOP_SIZE : HOP_SIZE + WINDOW_SIZE],
    ]


def make_dataset(samples_per_class: int = 700):
    xs = []
    ys = []

    for _ in range(samples_per_class):
        streams = [
            normal_stream(
                amp=np.random.uniform(800.0, 1100.0),
                phase=np.random.uniform(0.0, 2.0 * np.pi),
                noise=20.0,
            ),
            drift_stream(
                amp=np.random.uniform(500.0, 750.0),
                drift=np.random.uniform(900.0, 1500.0),
                noise=20.0,
            ),
            transient_stream(
                amp=np.random.uniform(650.0, 900.0),
                noise=20.0,
            ),
        ]

        for label, stream in enumerate(streams):
            for window in stream_windows(stream):
                xs.append(extract_features(window))
                ys.append(label)

    return np.stack(xs), np.array(ys, dtype=np.int64)


def eval_streams():
    return [
        ("normal", 0, normal_stream(amp=1000.0)),
        ("drift", 1, drift_stream(amp=650.0, drift=1200.0)),
        ("transient", 2, transient_stream(amp=800.0)),
    ]


def write_synthetic_source():
    arrays = []
    entries = []

    for name, label, samples in eval_streams():
        values = np.round(samples).astype(np.int16)
        parts = [str(int(value)) for value in values]
        lines = []

        for index in range(0, len(parts), 8):
            lines.append("\t" + ", ".join(parts[index : index + 8]))

        arrays.append(
            f"static const int16_t {name}_stream[ADC_ANOMALY_SAMPLES_PER_CLASS] = {{\n"
            + ",\n".join(lines)
            + "\n};\n"
        )
        entries.append(f"\t[{label}] = {name}_stream")

    stream_arrays = "\n".join(arrays)
    stream_entries = ",\n".join(entries)

    text = f"""/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sample_source.h"

#include "ring_buffer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define ADC_ANOMALY_SAMPLES_PER_CLASS (ADC_ANOMALY_WINDOW_SIZE + ADC_ANOMALY_HOP_SIZE)

{stream_arrays}
static const int16_t *const streams[ADC_ANOMALY_NUM_CLASSES] = {{
{stream_entries}
}};

static struct adc_anomaly_ring_buffer sample_ring;

int sample_source_init(void)
{{
adc_anomaly_ring_buffer_reset(&sample_ring);

return 0;
}}

int sample_source_load_window(size_t sample_window, int16_t *window, size_t len)
{{
enum adc_anomaly_label label;
const int16_t *samples;
size_t position;

if (len != ADC_ANOMALY_WINDOW_SIZE) {{
return -EINVAL;
}}

label = sample_source_expected_label(sample_window);
samples = streams[(size_t)label];
position = sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS;

if (position == 0U) {{
adc_anomaly_ring_buffer_reset(&sample_ring);
adc_anomaly_ring_buffer_push_many(&sample_ring, samples,
  ADC_ANOMALY_WINDOW_SIZE);
}} else {{
adc_anomaly_ring_buffer_push_many(&sample_ring,
  &samples[ADC_ANOMALY_WINDOW_SIZE],
  ADC_ANOMALY_HOP_SIZE);
}}

adc_anomaly_ring_buffer_copy_window(&sample_ring, window,
    ADC_ANOMALY_WINDOW_SIZE);

return 0;
}}

enum adc_anomaly_label sample_source_expected_label(size_t sample_window)
{{
return (enum adc_anomaly_label)(sample_window / ADC_ANOMALY_WINDOWS_PER_CLASS);
}}

size_t sample_source_window_start(size_t sample_window)
{{
return (sample_window % ADC_ANOMALY_WINDOWS_PER_CLASS) * ADC_ANOMALY_HOP_SIZE;
}}

const char *sample_source_label_name(enum adc_anomaly_label label)
{{
switch (label) {{
case ADC_ANOMALY_NORMAL:
return "normal";
case ADC_ANOMALY_DRIFT:
return "drift";
case ADC_ANOMALY_TRANSIENT:
return "transient";
default:
return "unknown";
}}
}}

const char *sample_source_name(void)
{{
return "synthetic";
}}

bool sample_source_has_expected_labels(void)
{{
return true;
}}
"""

    (SRC / "synthetic_source.c").write_text(text, newline="\n")


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
    np.random.seed(23)
    tf.random.set_seed(23)

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

    for name, label, stream in eval_streams():
        for window_index, window in enumerate(stream_windows(stream)):
            output = model.predict(extract_features(window)[None, :], verbose=0)[0]
            prediction = int(np.argmax(output))
            print(
                f"eval_stream={name}, window={window_index}, "
                f"expected={LABELS[label]}, predicted={LABELS[prediction]}"
            )

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

    write_synthetic_source()
    write_model_cpp(tflite_model)


if __name__ == "__main__":
    main()
