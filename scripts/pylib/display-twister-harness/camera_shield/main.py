# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import io
import os
import sys
import time
from string import Template

import cv2
import yaml

from camera_shield.uvc_core.camera_controller import UVCCamera
from camera_shield.uvc_core.plugin_base import PluginManager

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')


class Application:
    def __init__(self, config_path="config.yaml"):
        def resolve_env_vars(yaml_dict):
            """Process yaml with Template strings for safer environment variable resolution."""
            if isinstance(yaml_dict, dict):
                return {k: resolve_env_vars(v) for k, v in yaml_dict.items()}
            elif isinstance(yaml_dict, list):
                return [resolve_env_vars(i) for i in yaml_dict]
            elif isinstance(yaml_dict, str):
                # Create a template and substitute environment variables
                template = Template(yaml_dict)
                return template.safe_substitute(os.environ)
            else:
                return yaml_dict

        self.active_plugins = {}  # Initialize empty plugin dictionary
        with open(config_path, encoding="utf-8-sig") as f:
            config = yaml.safe_load(f)
            self.config = resolve_env_vars(config)

        os.environ["DISPLAY"] = ":0"

        self.case_config = {
            "device_id": 0,
            "res_x": 1280,
            "res_y": 720,
            "fps": 30,
            "run_time": 20,
        }

        if "case_config" in self.config:
            self.case_config["device_id"] = self.config["case_config"].get("device_id", 0)
            self.case_config["res_x"] = self.config["case_config"].get("res_x", 1280)
            self.case_config["res_y"] = self.config["case_config"].get("res_y", 720)
            self.case_config["fps"] = self.config["case_config"].get("fps", 30)
            self.case_config["run_time"] = self.config["case_config"].get("run_time", 20)

        self.camera = UVCCamera(self.case_config)
        self.plugin_manager = PluginManager()
        self.load_plugins()
        self.results = []

    def load_plugins(self):
        for plugin_cfg in self.config["plugins"]:
            if plugin_cfg.get("status", "disable") == "disable":
                continue
            module = importlib.import_module(plugin_cfg["module"], package=__package__)
            plugin_class = getattr(module, plugin_cfg["class"])
            self.active_plugins[plugin_cfg["name"]] = plugin_class(
                plugin_cfg["name"], plugin_cfg.get("config", {})
            )
            self.plugin_manager.register_plugin(plugin_cfg["name"], plugin_class)

    def handle_results(self, results, frame):
        for name, plugin in self.active_plugins.items():
            if name in results:
                plugin.handle_results(results[name], frame)

    def shutdown(self):
        self.camera.release()
        for plugin in self.active_plugins.values():
            self.results += plugin.shutdown()

    def run(self):
        try:
            start_time = time.time()
            self.camera.initialize()
            for name, plugin in self.active_plugins.items():  # noqa: B007
                plugin.initialize()
            while True:
                ret, frame = self.camera.get_frame()
                if not ret:
                    continue

                # Maintain OpenCV event loop
                if cv2.waitKey(1) == 27:  # ESC key
                    break

                results = {}
                for name, plugin in self.active_plugins.items():
                    results[name] = plugin.process_frame(frame)

                self.handle_results(results, frame)
                self.camera.show_frame(frame)
                frame_delay = 1 / self.case_config["fps"]
                if time.time() - start_time > self.case_config["run_time"]:
                    break
                time.sleep(frame_delay)

        except KeyboardInterrupt:
            print("quit by key input\n")
        finally:
            self.shutdown()

        return self.results


if __name__ == "__main__":
    app = Application()
    app.run()
