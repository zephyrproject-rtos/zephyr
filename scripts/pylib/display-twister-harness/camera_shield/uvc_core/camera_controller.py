# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import cv2
import numpy as np


class UVCCamera:
    def __init__(self, config):
        self.device_id = config.get("device_id", 0)
        self.res_x = config.get("res_x", 1280)
        self.res_y = config.get("res_y", 720)
        self.cap = cv2.VideoCapture(self.device_id)
        self.prev_frame = None
        self.current_alarms = 0
        self.alarm_duration = 5  # Alarm duration in frames
        self.fingerprint_cache = {}  # Store video fingerprints
        self._original_wb = 0

    def initialize(self):
        if not self.cap.isOpened():
            raise Exception(f"Failed to open camera {self.device_id}")
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.res_x)  # Set resolution
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.res_y)
        self.cap.set(cv2.CAP_PROP_AUTOFOCUS, 1)
        # Read initial 10 frames to stabilize camera
        for _ in range(10):
            self.get_frame()

        # Use first 120 frames to optimize focus
        best_focus_score = 0
        for i in range(120):
            ret, frame = self.get_frame()
            if ret:
                current_score = self.smart_focus(frame)
                if current_score > best_focus_score:
                    best_focus_score = current_score
                if i % 20 == 0:  # Periodically adjust focus
                    self.auto_focus(current_score < 0.5)
        self.auto_white_balance(True)
        self.print_settings()

    def get_frame(self):
        ret, frame = self.cap.read()
        if ret:
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        return ret, frame

    def show_frame(self, frame):
        cv2.imshow(f"Camera Preview (Device {self.device_id})", frame)
        cv2.waitKey(1)

    def auto_focus(self, enable: bool):
        """Auto focus control

        Args:
            enable: True to enable auto focus, False to disable
        """
        self.cap.set(cv2.CAP_PROP_AUTOFOCUS, 1 if enable else 0)

    def smart_focus(self, frame):
        """Smart focus algorithm based on image sharpness

        Args:
            frame: Current video frame

        Returns:
            Focus score value (0-1)
        """
        if frame is None:
            return 0

        # Convert to grayscale
        gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)

        # Calculate Laplacian variance as sharpness metric
        laplacian = cv2.Laplacian(gray, cv2.CV_64F)
        focus_score = laplacian.var() / 1000  # Normalized to 0-1 range

        # Adjust focus based on sharpness
        if focus_score < 0.3:  # Image is blurry
            self.auto_focus(True)
        elif focus_score > 0.7:  # Image is sharp
            self.auto_focus(False)

        return min(max(focus_score, 0), 1)  # Ensure value is within 0-1 range

    def auto_white_balance(self, enable):
        """Enable/disable auto white balance with algorithm optimization"""
        if enable:
            self._original_wb = self.cap.get(cv2.CAP_PROP_WB_TEMPERATURE)

            self.cap.set(cv2.CAP_PROP_AUTO_WB, 1)

            # Sample frames and analyze white balance
            wb_scores = []
            for _ in range(10):  # Sample 10 frames for better accuracy
                ret, frame = self.cap.read()
                if not ret:
                    break

                # Convert to LAB color space and analyze A/B channels
                lab = cv2.cvtColor(frame, cv2.COLOR_BGR2LAB)
                a_channel, b_channel = lab[:, :, 1], lab[:, :, 2]

                # Calculate white balance score (lower is better)
                wb_score = np.std(a_channel) + np.std(b_channel)
                wb_scores.append(wb_score)

                # If score improves significantly, keep current WB
                if len(wb_scores) > 1 and wb_score < min(wb_scores[:-1]) * 0.9:
                    break
        else:
            if hasattr(self, "_original_wb"):
                self.cap.set(cv2.CAP_PROP_AUTO_WB, 0)
                self.cap.set(cv2.CAP_PROP_WB_TEMPERATURE, self._original_wb)
                ret, frame = self.cap.read()
                if ret:
                    cv2.waitKey(1)

    def capture_image(self, save_path=None):
        """Capture current frame at highest resolution and optionally save to file"""
        # Set to maximum resolution
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        ret, frame = self.cap.read()
        if not ret:
            return None

        if save_path:
            cv2.imwrite(save_path, frame)
        return frame

    def analyze_image(self, image):
        """Perform basic image analysis on captured frame"""
        if image is None:
            return None

        analysis = {}
        # Brightness analysis
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        analysis["brightness"] = np.mean(gray)

        # Color analysis
        lab = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
        analysis["color_balance"] = {
            "a_channel": np.mean(lab[:, :, 1]),
            "b_channel": np.mean(lab[:, :, 2]),
        }

        # Sharpness analysis
        analysis["sharpness"] = cv2.Laplacian(gray, cv2.CV_64F).var()

        return analysis

    def show_analysis(self, analysis, frame=None):
        """Display image analysis results on frame if provided"""
        if analysis is None:
            print("No analysis results to display")
            return

        # Print analysis results
        print("Image Analysis Results:")
        print(f"  Brightness: {analysis['brightness']:.2f}")
        print(f"  Color Balance - A Channel: {analysis['color_balance']['a_channel']:.2f}")
        print(f"  Color Balance - B Channel: {analysis['color_balance']['b_channel']:.2f}")
        print(f"  Sharpness: {analysis['sharpness']:.2f}")

        # show report
        if frame is not None:
            text_y = 30
            cv2.putText(
                frame,
                f"Brightness: {analysis['brightness']:.2f}",
                (10, text_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )
            text_y += 30
            cv2.putText(
                frame,
                f"A Channel: {analysis['color_balance']['a_channel']:.2f}",
                (10, text_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )
            text_y += 30
            cv2.putText(
                frame,
                f"B Channel: {analysis['color_balance']['b_channel']:.2f}",
                (10, text_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )
            text_y += 30
            cv2.putText(
                frame,
                f"Sharpness: {analysis['sharpness']:.2f}",
                (10, text_y),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )

    def load_image(self, file_path):
        """Load a saved image from file

        Args:
            file_path: Path to the image file

        Returns:
            The loaded image in BGR format, or None if loading fails
        """
        try:
            image = cv2.imread(file_path)
            if image is None:
                print(f"Failed to load image from {file_path}")
            return image
        except Exception as e:
            print(f"Error loading image: {str(e)}")
            return None

    def analyze_multiple_frames(self, num_frames=10):
        """Analyze multiple frames and return average results"""
        results = []

        for _ in range(num_frames):
            frame = self.capture_image()
            if frame is None:
                continue

            analysis = self.analyze_image(frame)
            results.append(analysis)

        if not results:
            return None

        # Calculate averages
        avg_analysis = {
            "brightness": np.mean([r["brightness"] for r in results]),
            "color_balance": {
                "a_channel": np.mean([r["color_balance"]["a_channel"] for r in results]),
                "b_channel": np.mean([r["color_balance"]["b_channel"] for r in results]),
            },
            "sharpness": np.mean([r["sharpness"] for r in results]),
        }

        return avg_analysis

    def print_settings(self):
        """Print current camera settings"""
        print(f"Camera {self.device_id} Settings:")
        print(
            f"Resolution: {int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))}\
                x{int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))}"
        )
        print(f"  FPS: {self.cap.get(cv2.CAP_PROP_FPS):.2f}")
        print(f"  Auto Focus: {'ON' if self.cap.get(cv2.CAP_PROP_AUTOFOCUS) else 'OFF'}")
        print(f"  Auto White Balance: {'ON' if self.cap.get(cv2.CAP_PROP_AUTO_WB) else 'OFF'}")

    def release(self):
        self.cap.release()
        try:
            # compatible with openCV-headless mode
            cv2.destroyAllWindows()
        except Exception as e:
            # Handle cv2.error and other potential exceptions
            if "not implemented" in str(e) or "Rebuild the library" in str(e):
                # This is expected in headless/no-GUI environments
                pass
            else:
                print(f"Warning: Could not destroy windows: {e}")
