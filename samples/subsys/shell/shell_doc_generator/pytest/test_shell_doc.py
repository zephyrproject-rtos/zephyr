import os


def test_shell_doc_generator(dut):
    # Wait for the beginning of the documentation output
    dut.readlines_until(regex=".*Zephyr Shell Command Reference", timeout=5.0)

    lines = ["# Zephyr Shell Command Reference"]

    try:
        # We read lines until we get the shell prompt
        output = dut.readlines_until(regex=r"-- EOF --", timeout=10.0)
        lines.extend(output)
    except Exception:
        # In case we timeout, log the issue but proceed - yaml file might not be valid though
        pass

    filepath = os.path.join(dut.device_config.build_dir, "shell_doc.yaml")

    print(f"Writing shell doc to {filepath}")

    with open(filepath, "w") as f:
        for line in lines:
            if "-- EOF --" not in line:
                f.write(line + "\n")
