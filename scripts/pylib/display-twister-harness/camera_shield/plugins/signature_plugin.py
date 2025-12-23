# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import hashlib
import os
import pickle
from datetime import datetime

import cv2
import numpy as np

from camera_shield.uvc_core.plugin_base import DetectionPlugin


class VideoSignaturePlugin(DetectionPlugin):
    """Plugin for generating and comparing video frame signatures and fingerprints"""

    def __init__(self, name, config):
        super().__init__(name, config)
        self.fingerprint_cache = []  # Store video fingerprints
        self.fingerprint = {
            "method": config.get("method", "combined"),
            "frame_signatures": [],
            "metadata": config.get("metadata", {}),
        }
        self.frame_count = 0
        self.operations = config.get("operations", "compare")
        self.saved = False
        self.result = []

    def initialize(self):
        """Initialize detection resources"""
        print("initialize")
        if self.operations == "compare":
            print("compare")
            directory = self.config.get("directory", "./fingerprints")
            if os.path.isdir(directory):
                self.load_all_fingerprints(directory)
            else:
                print(f"{directory} not exist")

    def generate_frame_signature(self, frame, method="combined") -> dict:
        """Generate a robust signature for a given frame using multiple techniques

        Args:
            frame: The input frame
            method: Signature method ('phash', 'dhash', 'histogram', 'combined')

        Returns:
            A dictionary containing signature data
        """
        if frame is None:
            return None

        # Convert to grayscale for consistent processing
        if len(frame.shape) == 3:
            gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
        else:
            gray = frame.copy()

        # Resize to standard size for consistent signatures
        resized = cv2.resize(gray, (64, 64))

        signature = {}

        # Perceptual Hash (pHash)
        if method in ["phash", "combined"]:
            # DCT transform
            dct = cv2.dct(np.float32(resized))
            # Keep only the top-left 8x8 coefficients
            dct_low = dct[:8, :8]
            # Compute median value
            med = np.median(dct_low)
            # Convert to binary hash
            phash = (dct_low > med).flatten().astype(int)
            signature["phash"] = phash.tolist()
            # Generate a compact hex representation
            phash_hex = "".join(
                [
                    hex(int("".join(map(str, phash[i : i + 4])), 2))[2:]
                    for i in range(0, len(phash), 4)
                ]
            )
            signature["phash_hex"] = phash_hex

        # Difference Hash (dHash)
        if method in ["dhash", "combined"]:
            # Resize to 9x8 (one pixel larger horizontally)
            resized_dhash = cv2.resize(gray, (9, 8))
            # Compute differences horizontally
            diff = resized_dhash[:, 1:] > resized_dhash[:, :-1]
            # Flatten to 1D array
            dhash = diff.flatten().astype(int)
            signature["dhash"] = dhash.tolist()
            # Generate a compact hex representation
            dhash_hex = "".join(
                [
                    hex(int("".join(map(str, dhash[i : i + 4])), 2))[2:]
                    for i in range(0, len(dhash), 4)
                ]
            )
            signature["dhash_hex"] = dhash_hex

        # Color histogram features
        if method in ["histogram", "combined"]:
            if len(frame.shape) == 3:  # Color image
                hist_features = []
                for i in range(3):  # For each color channel
                    hist = cv2.calcHist([frame], [i], None, [16], [0, 256])
                    hist = cv2.normalize(hist, hist).flatten()
                    hist_features.extend(hist)
                signature["color_hist"] = [float(x) for x in hist_features]
            else:  # Grayscale
                hist = cv2.calcHist([gray], [0], None, [32], [0, 256])
                hist = cv2.normalize(hist, hist).flatten()
                signature["gray_hist"] = [float(x) for x in hist]

        # Edge features
        if method in ["combined"]:
            # Canny edge detection
            edges = cv2.Canny(resized, 100, 200)
            # Count percentage of edge pixels
            edge_ratio = np.count_nonzero(edges) / edges.size
            signature["edge_ratio"] = float(edge_ratio)

            # Compute gradient magnitude histogram
            sobelx = cv2.Sobel(resized, cv2.CV_64F, 1, 0, ksize=3)
            sobely = cv2.Sobel(resized, cv2.CV_64F, 0, 1, ksize=3)
            magnitude = np.sqrt(sobelx**2 + sobely**2)
            mag_hist = np.histogram(magnitude, bins=8, range=(0, 255))[0]
            mag_hist = mag_hist / np.sum(mag_hist)  # Normalize
            signature["gradient_hist"] = [float(x) for x in mag_hist]

        # Generate a unique hash for the entire signature
        signature_str = str(signature)
        signature["hash"] = hashlib.sha256(signature_str.encode()).hexdigest()

        return signature

    def compare_signatures(self, sig1, sig2, method="combined") -> float:
        """Compare two frame signatures and return similarity score (0-1)

        Args:
            sig1: First signature dictionary
            sig2: Second signature dictionary
            method: Comparison method matching the signature generation method

        Returns:
            Similarity score between 0 (different) and 1 (identical)
        """
        if sig1 is None or sig2 is None:
            return 0.0

        scores = []

        # Compare perceptual hashes (Hamming distance)
        if method in ["phash", "combined"] and "phash" in sig1 and "phash" in sig2:
            phash1 = np.array(sig1["phash"])
            phash2 = np.array(sig2["phash"])
            hamming_dist = np.sum(phash1 != phash2)
            max_dist = len(phash1)
            phash_score = 1.0 - (hamming_dist / max_dist)
            scores.append(phash_score)

        # Compare difference hashes
        if method in ["dhash", "combined"] and "dhash" in sig1 and "dhash" in sig2:
            dhash1 = np.array(sig1["dhash"])
            dhash2 = np.array(sig2["dhash"])
            hamming_dist = np.sum(dhash1 != dhash2)
            max_dist = len(dhash1)
            dhash_score = 1.0 - (hamming_dist / max_dist)
            scores.append(dhash_score)

        # Compare color histograms
        if method in ["histogram", "combined"]:
            if "color_hist" in sig1 and "color_hist" in sig2:
                hist1 = np.array(sig1["color_hist"])
                hist2 = np.array(sig2["color_hist"])
                # Bhattacharyya distance for histograms
                hist_score = cv2.compareHist(
                    hist1.astype(np.float32),
                    hist2.astype(np.float32),
                    cv2.HISTCMP_BHATTACHARYYA,
                )
                # Convert to similarity score (0-1)
                hist_score = 1.0 - min(hist_score, 1.0)
                scores.append(hist_score)
            elif "gray_hist" in sig1 and "gray_hist" in sig2:
                hist1 = np.array(sig1["gray_hist"])
                hist2 = np.array(sig2["gray_hist"])
                hist_score = cv2.compareHist(
                    hist1.astype(np.float32),
                    hist2.astype(np.float32),
                    cv2.HISTCMP_BHATTACHARYYA,
                )
                hist_score = 1.0 - min(hist_score, 1.0)
                scores.append(hist_score)

        # Compare edge features
        if method in ["combined"] and "edge_ratio" in sig1 and "edge_ratio" in sig2:
            edge_diff = abs(sig1["edge_ratio"] - sig2["edge_ratio"])
            edge_score = 1.0 - min(edge_diff, 1.0)
            scores.append(edge_score)

        # Compare gradient histograms
        if method in ["combined"] and "gradient_hist" in sig1 and "gradient_hist" in sig2:
            grad1 = np.array(sig1["gradient_hist"])
            grad2 = np.array(sig2["gradient_hist"])
            grad_score = cv2.compareHist(
                grad1.astype(np.float32),
                grad2.astype(np.float32),
                cv2.HISTCMP_BHATTACHARYYA,
            )
            grad_score = 1.0 - min(grad_score, 1.0)
            scores.append(grad_score)

        # If no scores were calculated, return 0
        if not scores:
            return 0.0

        # Weight the scores based on reliability (can be adjusted)
        weights = {
            "phash": self.config.get("phash_weight", 0.35),
            "dhash": self.config.get("dhash_weight", 0.25),
            "histogram": self.config.get("histogram_weight", 0.2),
            "edge_ratio": self.config.get("edge_ratio_weight", 0.1),
            "gradient_hist": self.config.get("gradient_hist_weight", 0.1),
        }

        # For combined method, use weighted average
        if method == "combined":
            final_score = 0.0
            total_weight = 0.0

            if "phash" in sig1 and "phash" in sig2:
                final_score += scores[0] * weights["phash"]
                total_weight += weights["phash"]

            if "dhash" in sig1 and "dhash" in sig2:
                final_score += scores[1] * weights["dhash"]
                total_weight += weights["dhash"]

            if ("color_hist" in sig1 and "color_hist" in sig2) or (
                "gray_hist" in sig1 and "gray_hist" in sig2
            ):
                final_score += scores[2] * weights["histogram"]
                total_weight += weights["histogram"]

            if "edge_ratio" in sig1 and "edge_ratio" in sig2:
                final_score += scores[3] * weights["edge_ratio"]
                total_weight += weights["edge_ratio"]

            if "gradient_hist" in sig1 and "gradient_hist" in sig2:
                final_score += scores[4] * weights["gradient_hist"]
                total_weight += weights["gradient_hist"]

            if total_weight > 0:
                return final_score / total_weight
            else:
                return 0.0
        else:
            # For single methods, return the calculated score
            return scores[0]

    def generate_video_fingerprint(self, frame, method="combined") -> dict:
        """Generate a fingerprint from a list of video frames

        Args:
            frames: List of video frames
            device_id: Camera device ID
            method: Signature method to use

        Returns:
            Dictionary containing video fingerprint data
        """

        if self.frame_count < self.config.get("duration", 100):
            # Generate signature for this frame
            signature = self.generate_frame_signature(frame, method)
            if signature:
                self.fingerprint["frame_signatures"].append(signature)

            self.frame_count += 1

        return self.frame_count

    def save_fingerprint(self, directory="fingerprints"):
        """Save a video fingerprint to disk

        Args:
            fingerprint: The fingerprint dictionary to save
            directory: Directory to save fingerprints in

        Returns:
            Path to the saved fingerprint file
        """

        if not self.fingerprint["frame_signatures"]:
            return ""

        if self.frame_count < self.config.get("duration", 100):
            return ""

        if self.saved:
            return ""

        # Create directory if it doesn't exist
        os.makedirs(directory, exist_ok=True)

        # Create filename based on fingerprint ID and timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"fingerprint_{timestamp}.pkl"
        filepath = os.path.join(directory, filename)

        # Save fingerprint using pickle
        with open(filepath, "wb") as f:
            pickle.dump(self.fingerprint, f)

        self.saved = True

        return filepath

    def load_fingerprint(self, filepath):
        """Load a fingerprint from disk

        Args:
            filepath: Path to the fingerprint file

        Returns:
            The loaded fingerprint dictionary or None if loading fails
        """
        try:
            with open(filepath, "rb") as f:
                fingerprint = pickle.load(f)

            return fingerprint
        except Exception as e:
            print(f"Error loading fingerprint from {filepath}: {str(e)}")
            return None

    def compare_video_fingerprints(self, fp1, fp2) -> dict:
        """Compare two video fingerprints and return similarity metrics

        Args:
            fp1: First fingerprint dictionary
            fp2: Second fingerprint dictionary

        Returns:
            Dictionary with similarity metrics
        """
        if fp1 is None or fp2 is None:
            return {"overall_similarity": 0.0, "error": "Invalid fingerprints"}

        # Check if fingerprints use the same method
        if fp1.get("method") != fp2.get("method"):
            print(
                f"Warning: Comparing fingerprints with different methods:\
                    {fp1.get('method')} vs {fp2.get('method')}"
            )

        method = fp1.get("method", "combined")

        # Get frame signatures
        sigs1 = fp1.get("frame_signatures", [])
        sigs2 = fp2.get("frame_signatures", [])

        if not sigs1 or not sigs2:
            return {"overall_similarity": 0.0, "error": "Empty frame signatures"}

        # Calculate frame-by-frame similarities
        frame_similarities = []

        # Use dynamic time warping approach for different length fingerprints
        if len(sigs1) != len(sigs2):
            # Create similarity matrix
            sim_matrix = np.zeros((len(sigs1), len(sigs2)))

            for i, sig1 in enumerate(sigs1):
                for j, sig2 in enumerate(sigs2):
                    sim_matrix[i, j] = self.compare_signatures(sig1, sig2, method)

            # Find optimal path through similarity matrix (simplified DTW)
            path_similarities = []

            # For each frame in the shorter fingerprint, find best match in longer one
            if len(sigs1) <= len(sigs2):
                for i in range(len(sigs1)):
                    best_match = np.max(sim_matrix[i, :])
                    path_similarities.append(best_match)
            else:
                for j in range(len(sigs2)):
                    best_match = np.max(sim_matrix[:, j])
                    path_similarities.append(best_match)

            frame_similarities = path_similarities
        else:
            # Direct frame-by-frame comparison for same length fingerprints
            for sig1, sig2 in zip(sigs1, sigs2, strict=False):
                similarity = self.compare_signatures(sig1, sig2, method)
                frame_similarities.append(similarity)

        # Calculate overall metrics
        overall_similarity = np.mean(frame_similarities) if frame_similarities else 0.0
        min_similarity = np.min(frame_similarities) if frame_similarities else 0.0
        max_similarity = np.max(frame_similarities) if frame_similarities else 0.0

        # Calculate temporal consistency (how consistent the similarity is across frames)
        temporal_consistency = 1.0 - np.std(frame_similarities) if frame_similarities else 0.0

        return {
            "overall_similarity": float(overall_similarity),
            "min_similarity": float(min_similarity),
            "max_similarity": float(max_similarity),
            "temporal_consistency": float(temporal_consistency),
            "frame_similarities": [float(s) for s in frame_similarities],
            "name": fp2["metadata"]["name"],
            "metadata": fp2["metadata"],
        }

    def identify_video(self, threshold=0.85) -> list[dict]:
        """Identify a video sample against stored fingerprints

        Args:
            sample_fingerprint: Fingerprint of the video to identify
            threshold: Minimum similarity threshold for a match

        Returns:
            List of matching fingerprints with similarity scores
        """
        matches = []

        # Compare against all cached fingerprints
        for fingerprint in self.fingerprint_cache:
            comparison = self.compare_video_fingerprints(self.fingerprint, fingerprint)

            if comparison["overall_similarity"] >= threshold:
                matches.append(
                    {
                        "similarity": comparison["overall_similarity"],
                        "details": comparison,
                    }
                )

        # Sort matches by similarity (highest first)
        matches.sort(key=lambda x: x["similarity"], reverse=True)

        return matches

    def load_all_fingerprints(self, directory="fingerprints"):
        """Load all fingerprints from a directory into cache

        Args:
            directory: Directory containing fingerprint files

        Returns:
            Number of fingerprints loaded
        """
        if not os.path.exists(directory):
            return 0

        count = 0
        for filename in os.listdir(directory):
            if filename.endswith(".pkl"):
                filepath = os.path.join(directory, filename)
                fingerprint = self.load_fingerprint(filepath)
                if fingerprint:
                    self.fingerprint_cache.append(fingerprint)
                    count += 1

        return count

    def process_frame(self, frame):
        method = self.config.get("method", "combined")
        count = self.generate_video_fingerprint(frame, method)
        return {
            "frame_count": count,
            "frame": frame,
        }

    def handle_results(self, result, frame):
        matched = []
        operations = self.config.get("operations", "compare")
        if result["frame_count"] == self.config.get("duration", 100):
            if operations == "compare":
                matched = self.identify_video(self.config.get("threshold", 0.85))
            elif operations == "generate":
                self.save_fingerprint(self.config.get("directory", "./fingerprint"))
            else:
                print("not supported operation")

            if matched:
                if matched[0]["details"]["name"] not in self.result:
                    self.result.append(matched[0]["details"]["name"])
                cv2.putText(
                    frame,
                    f"match with {matched[0]['details']['name']} :{matched[0]['similarity']:.2f}",
                    (150, frame.shape[0] - 90),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 0, 255),
                    2,
                )
            else:
                cv2.putText(
                    frame,
                    "signature done",
                    (150, frame.shape[0] - 90),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 0, 255),
                    2,
                )
        else:
            cv2.putText(
                frame,
                "signature in progress",
                (150, frame.shape[0] - 90),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 0, 255),
                2,
            )

    def shutdown(self) -> list:
        """Release plugin resources"""
        if self.config.get("operations", "compare") == "compare":
            if self.result:
                print(f"{self.__class__.__name__} result: {self.result}\n")
            else:
                print(f"{self.__class__.__name__} result: no match\n")

        return self.result
