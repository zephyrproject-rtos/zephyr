#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Run an autonomous Chrome answerer opposite the physical RT1170 board."""

import argparse
import hashlib
import json
import sys
import threading
import time
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

PAGE = Path(__file__).with_name("physical_browser_answerer.html")
MAX_SDP_BYTES = 8095
MAX_ENCODED_FRAME_BYTES = 2 * 1024 * 1024
MEDIA_STARTUP_TIMEOUT_SECONDS = 10.0
MEDIA_STALL_TIMEOUT_SECONDS = 8.0
MEDIA_STARTUP_MINIMUMS = {
    "inboundAudioPackets": 100,
    "outboundAudioPackets": 100,
    "inboundVideoPackets": 10,
    "outboundVideoPackets": 10,
    "inboundVideoFramesDecoded": 10,
    "outboundVideoFramesEncoded": 10,
}
MEDIA_FINAL_MINIMUMS = {
    "inboundAudioPackets": 300,
    "outboundAudioPackets": 300,
    "inboundVideoPackets": 30,
    "outboundVideoPackets": 30,
    "inboundVideoFramesDecoded": 30,
    "outboundVideoFramesEncoded": 30,
}


class Exchange:
    def __init__(self):
        self.lock = threading.Lock()
        self.answer_ready = threading.Event()
        self.offer = None
        self.answer = None
        self.offer_received_at = None
        self.answer_received_at = None


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    server_version = "RT1170BrowserGate/1"

    def log_message(self, format_string, *args):
        self.server.log.append({"time": time.time(), "message": format_string % args})

    def send_bytes(self, status, body=b"", content_type="text/plain"):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Connection", "close")
        self.end_headers()
        if body:
            self.wfile.write(body)

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/":
            self.send_bytes(
                HTTPStatus.OK,
                PAGE.read_bytes(),
                "text/html; charset=utf-8",
            )
            return
        if path == "/healthz":
            self.send_bytes(HTTPStatus.OK, b"ok\n")
            return
        if path == "/pending-offer":
            with self.server.exchange.lock:
                offer = self.server.exchange.offer
            if offer is None:
                self.send_bytes(HTTPStatus.NO_CONTENT)
            else:
                self.send_bytes(
                    HTTPStatus.OK,
                    offer,
                    "application/sdp",
                )
            return
        self.send_bytes(HTTPStatus.NOT_FOUND, b"not found\n")

    def read_sdp(self):
        if self.headers.get_content_type() != "application/sdp":
            self.send_bytes(
                HTTPStatus.UNSUPPORTED_MEDIA_TYPE,
                b"application/sdp required\n",
            )
            return None
        try:
            length = int(self.headers.get("Content-Length", ""))
        except ValueError:
            length = -1
        if length < 1 or length > MAX_SDP_BYTES:
            self.send_bytes(HTTPStatus.REQUEST_ENTITY_TOO_LARGE, b"bad size\n")
            return None
        body = self.rfile.read(length)
        if len(body) != length or b"\x00" in body or not body.startswith(b"v=0\r\n"):
            self.send_bytes(HTTPStatus.BAD_REQUEST, b"invalid SDP\n")
            return None
        return body

    def read_encoded_frame(self):
        try:
            length = int(self.headers.get("Content-Length", ""))
            sequence = int(self.headers.get("X-Frame-Sequence", ""))
        except ValueError:
            self.send_bytes(HTTPStatus.BAD_REQUEST, b"invalid frame metadata\n")
            return None
        if length < 1 or length > MAX_ENCODED_FRAME_BYTES or sequence < 0:
            self.send_bytes(HTTPStatus.REQUEST_ENTITY_TOO_LARGE, b"bad size\n")
            return None
        body = self.rfile.read(length)
        if len(body) != length:
            self.send_bytes(HTTPStatus.BAD_REQUEST, b"short frame\n")
            return None
        return sequence, body

    def do_POST(self):
        if self.path == "/encoded-frame":
            encoded = self.read_encoded_frame()
            if encoded is None:
                return
            sequence, body = encoded
            metadata = {
                "sequence": sequence,
                "timestamp": self.headers.get("X-Frame-Timestamp", ""),
                "frame_type": self.headers.get("X-Frame-Type", ""),
                "rtp_timestamp": self.headers.get("X-Rtp-Timestamp", ""),
                "payload_type": self.headers.get("X-Payload-Type", ""),
                "synchronization_source": self.headers.get("X-Synchronization-Source", ""),
                "frame_id": self.headers.get("X-Frame-Id", ""),
                "size": len(body),
                "sha256": hashlib.sha256(body).hexdigest(),
                "received_at": time.time(),
            }
            with self.server.capture_lock:
                if sequence in self.server.captured_sequences:
                    self.send_bytes(HTTPStatus.CONFLICT, b"duplicate frame\n")
                    return
                self.server.captured_sequences.add(sequence)
                path = self.server.capture_dir / f"frame-{sequence:08d}.bin"
                path.write_bytes(body)
                self.server.encoded_frames.append(metadata)
            self.send_bytes(HTTPStatus.NO_CONTENT)
            return

        body = self.read_sdp()
        if body is None:
            return
        exchange = self.server.exchange
        if self.path == "/offer":
            with exchange.lock:
                if exchange.offer is not None:
                    self.send_bytes(HTTPStatus.CONFLICT, b"call already exists\n")
                    return
                exchange.offer = body
                exchange.offer_received_at = time.time()
            if not exchange.answer_ready.wait(timeout=22.0):
                self.send_bytes(HTTPStatus.GATEWAY_TIMEOUT, b"answer timeout\n")
                return
            with exchange.lock:
                answer = exchange.answer
            self.send_bytes(HTTPStatus.OK, answer, "application/sdp")
            return
        if self.path == "/answer":
            with exchange.lock:
                if exchange.offer is None:
                    self.send_bytes(HTTPStatus.CONFLICT, b"no offer\n")
                    return
                if exchange.answer is not None:
                    self.send_bytes(HTTPStatus.CONFLICT, b"answer already exists\n")
                    return
                exchange.answer = body
                exchange.answer_received_at = time.time()
                exchange.answer_ready.set()
            self.send_bytes(HTTPStatus.NO_CONTENT)
            return
        self.send_bytes(HTTPStatus.NOT_FOUND, b"not found\n")


class Server(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, address, exchange, output):
        super().__init__(address, Handler)
        self.exchange = exchange
        self.log = []
        self.capture_lock = threading.Lock()
        self.capture_dir = output / "chrome-encoded-frames"
        self.capture_dir.mkdir(parents=True, exist_ok=True)
        self.captured_sequences = set()
        self.encoded_frames = []


def collect(driver):
    result = driver.execute_async_script(
        "const done = arguments[arguments.length - 1];"
        "window.collectTestDiagnostics().then(done).catch("
        "error => done({errors: [String(error)]}));"
    )
    if not isinstance(result, dict):
        raise RuntimeError(f"invalid browser diagnostics: {result!r}")
    return result


def save_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def max_counters(history):
    result = {}
    for snapshot in history:
        for key, value in snapshot.get("counters", {}).items():
            result[key] = max(result.get(key, 0), value or 0)
    return result


def max_contiguous_connected_seconds(history):
    longest = 0.0
    segment_started = None
    for snapshot in history:
        elapsed = snapshot.get("elapsed", 0.0)
        if snapshot.get("connectionState") == "connected":
            if segment_started is None:
                segment_started = elapsed
            longest = max(longest, elapsed - segment_started)
        else:
            segment_started = None
    return longest


def validate_offer(offer):
    text = offer.decode("utf-8", errors="strict")
    required = (
        "m=video 9 UDP/TLS/RTP/SAVPF 96\r\n",
        "a=rtpmap:96 H264/90000\r\n",
        "packetization-mode=1",
        "profile-level-id=42c01f",
        "m=audio 9 UDP/TLS/RTP/SAVPF 0\r\n",
        "a=rtpmap:0 PCMU/8000\r\n",
        "a=group:BUNDLE 1 2\r\n",
    )
    missing = [line for line in required if line not in text]
    if missing:
        raise AssertionError(f"board offer missing required profile: {missing}")


def validate_answer(answer):
    text = answer.decode("utf-8", errors="strict")
    required = (
        "a=rtpmap:96 H264/90000\r\n",
        "m=audio 9 UDP/TLS/RTP/SAVPF 0\r\n",
        "a=rtpmap:0 PCMU/8000\r\n",
        "a=setup:active\r\n",
        "a=candidate:",
    )
    missing = [line for line in required if line not in text]
    if not any(
        line.startswith("m=video ")
        and line.endswith(" UDP/TLS/RTP/SAVPF 96")
        and line.split()[1] != "0"
        for line in text.splitlines()
    ):
        missing.append("active m=video with H.264 PT96")
    if missing:
        raise AssertionError(f"browser answer missing required profile: {missing}")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--timeout", type=float, default=50.0)
    parser.add_argument("--connected-seconds", type=float, default=13.0)
    parser.add_argument("--headed", action="store_true")
    parser.add_argument("--real-media", action="store_true")
    parser.add_argument("--capture-encoded", action="store_true")
    parser.add_argument("--video-fps", type=float, default=5.0)
    parser.add_argument("--chrome-binary", type=Path)
    args = parser.parse_args()
    if not 0.0 < args.video_fps <= 30.0:
        parser.error("--video-fps must be greater than 0 and at most 30")
    return args


def main():
    args = parse_args()
    media_startup_minimums = dict(MEDIA_STARTUP_MINIMUMS)
    media_startup_minimums["outboundVideoFramesEncoded"] = min(
        MEDIA_STARTUP_MINIMUMS["outboundVideoFramesEncoded"],
        max(2, int(args.video_fps * 4.0)),
    )
    output = args.output_dir.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)
    for name in ("remote-video.png", "browser-page.png"):
        (output / name).unlink(missing_ok=True)

    exchange = Exchange()
    server = Server((args.bind, args.port), exchange, output)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()
    driver = None
    history = []
    browser_console = []
    blockers = []
    connected_at = None
    connected_segment_started = None
    disconnected_at = None
    media_ready_at = None
    last_counter_values = {}
    last_counter_progress = {}
    screenshot_saved = False
    started = time.monotonic()
    try:
        from selenium import webdriver
        from selenium.webdriver.chrome.options import Options
        from selenium.webdriver.common.by import By

        options = Options()
        flags = [
            "--no-sandbox",
            "--disable-dev-shm-usage",
            "--use-fake-ui-for-media-stream",
            "--autoplay-policy=no-user-gesture-required",
            "--disable-features=WebRtcHideLocalIpsWithMdns",
            "--window-size=1280,1000",
        ]
        if not args.headed:
            flags.extend(("--headless=new", "--disable-gpu"))
        if not args.real_media:
            flags.append("--use-fake-device-for-media-stream")
        if args.capture_encoded:
            flags.append("--enable-blink-features=WebRTCEncodedTransform")
        for flag in flags:
            options.add_argument(flag)
        if args.chrome_binary:
            options.binary_location = str(args.chrome_binary.resolve())
        options.set_capability("goog:loggingPrefs", {"browser": "ALL"})
        driver = webdriver.Chrome(options=options)
        driver.set_script_timeout(5.0)
        page_url = f"http://127.0.0.1:{args.port}/"
        query = []
        if args.real_media:
            query.append("real=1")
        if args.capture_encoded:
            query.append("capture=1")
        query.append(f"fps={args.video_fps:g}")
        if query:
            page_url += "?" + "&".join(query)
        driver.get(page_url)
        driver.find_element(By.ID, "start").click()

        deadline = started + args.timeout
        ready_reported = False
        while time.monotonic() < deadline:
            now = time.monotonic()
            snapshot = collect(driver)
            snapshot["elapsed"] = round(now - started, 3)
            history.append(snapshot)
            if not ready_reported and snapshot.get("status") == "waiting for RT1170 offer":
                print("READY_FOR_RT1170", flush=True)
                ready_reported = True
            if snapshot.get("errors"):
                blockers.extend(snapshot["errors"])
                break

            connection_state = snapshot.get("connectionState")
            if connected_at is not None and connection_state in ("failed", "closed"):
                blockers.append(f"browser connection became {connection_state} after connecting")
                break
            if connection_state == "connected":
                if connected_at is None:
                    connected_at = now
                    print("BROWSER_CONNECTED", flush=True)
                if connected_segment_started is None:
                    connected_segment_started = now
                disconnected_at = None

                counters = snapshot.get("counters", {})
                for key in MEDIA_FINAL_MINIMUMS:
                    value = counters.get(key, 0) or 0
                    if value > last_counter_values.get(key, 0):
                        last_counter_values[key] = value
                        last_counter_progress[key] = now

                too_low = {
                    key: {"actual": counters.get(key, 0) or 0, "minimum": minimum}
                    for key, minimum in media_startup_minimums.items()
                    if (counters.get(key, 0) or 0) < minimum
                }
                if media_ready_at is None:
                    if not too_low:
                        media_ready_at = now
                        print("MEDIA_PATHS_ACTIVE", flush=True)
                    elif now - connected_at >= MEDIA_STARTUP_TIMEOUT_SECONDS:
                        blockers.append(f"initial media gate failed: {too_low}")
                        break
                else:
                    stalled = {
                        key: round(now - last_counter_progress.get(key, media_ready_at), 3)
                        for key in MEDIA_FINAL_MINIMUMS
                        if now - last_counter_progress.get(key, media_ready_at)
                        >= MEDIA_STALL_TIMEOUT_SECONDS
                    }
                    if stalled:
                        blockers.append(
                            f"media counters stalled for at least "
                            f"{MEDIA_STALL_TIMEOUT_SECONDS:.0f}s: {stalled}"
                        )
                        break

                video = snapshot.get("remoteVideo", {})
                if (
                    not screenshot_saved
                    and video.get("readyState", 0) >= 2
                    and video.get("width", 0) > 0
                ):
                    screenshot_saved = driver.find_element(By.ID, "remote").screenshot(
                        str(output / "remote-video.png")
                    )
                if now - connected_segment_started >= args.connected_seconds:
                    break
            elif connected_at is not None:
                connected_segment_started = None
                if connection_state == "disconnected":
                    if disconnected_at is None:
                        disconnected_at = now
                    elif now - disconnected_at >= MEDIA_STALL_TIMEOUT_SECONDS:
                        blockers.append(
                            f"browser remained disconnected for at least "
                            f"{MEDIA_STALL_TIMEOUT_SECONDS:.0f}s"
                        )
                        break
            time.sleep(0.25)
        else:
            blockers.append("browser gate timed out")

        driver.save_screenshot(str(output / "browser-page.png"))
        browser_console = driver.get_log("browser")
    except Exception as error:  # noqa: BLE001 - every browser failure is a blocker
        blockers.append(f"{type(error).__name__}: {error}")
    finally:
        if driver is not None:
            if args.capture_encoded:
                try:
                    capture_status = driver.execute_async_script(
                        "const done = arguments[arguments.length - 1];"
                        "window.waitForCaptureUploads().then(done).catch("
                        "error => done({error: String(error)}));"
                    )
                    if capture_status.get("pending", 0):
                        blockers.append(f"capture flush incomplete: {capture_status}")
                    if capture_status.get("error"):
                        blockers.append(f"capture flush: {capture_status['error']}")
                except Exception as error:  # noqa: BLE001 - reported as a blocker
                    blockers.append(f"capture flush: {error}")
            try:
                final = collect(driver)
                final["elapsed"] = round(time.monotonic() - started, 3)
                history.append(final)
            except Exception as error:  # noqa: BLE001 - reported as a blocker
                blockers.append(f"final diagnostics: {error}")
            driver.quit()
        server.shutdown()
        server.server_close()
        server_thread.join(timeout=2)

    offer = exchange.offer or b""
    answer = exchange.answer or b""
    (output / "offer.sdp").write_bytes(offer)
    (output / "answer.sdp").write_bytes(answer)
    save_json(output / "history.json", history)
    save_json(output / "browser-console.json", browser_console)
    save_json(output / "http-server.json", server.log)
    encoded_frames = sorted(server.encoded_frames, key=lambda frame: frame["sequence"])
    save_json(output / "chrome-encoded-frames.json", encoded_frames)

    counters = max_counters(history)
    final_snapshot = history[-1] if history else {}
    qualified_connected_seconds = max_contiguous_connected_seconds(history)
    try:
        if not offer:
            raise AssertionError("board did not post an offer")
        validate_offer(offer)
        if not answer:
            raise AssertionError("browser did not post an answer")
        validate_answer(answer)
        if connected_at is None:
            raise AssertionError("Chrome never reached connected")
        too_low = {
            key: {"actual": counters.get(key, 0), "minimum": minimum}
            for key, minimum in MEDIA_FINAL_MINIMUMS.items()
            if counters.get(key, 0) < minimum
        }
        if too_low:
            raise AssertionError(f"insufficient browser media: {too_low}")
        if media_ready_at is None:
            raise AssertionError("all six browser media counters never became active")
        if final_snapshot.get("connectionState") != "connected":
            raise AssertionError(
                f"browser did not remain connected: {final_snapshot.get('connectionState')}"
            )
        if qualified_connected_seconds + 0.5 < args.connected_seconds:
            raise AssertionError(
                "connected interval was too short: "
                f"{qualified_connected_seconds:.3f}s < "
                f"{args.connected_seconds:.3f}s"
            )
        if not screenshot_saved:
            raise AssertionError("RT1170 remote-video screenshot was not saved")
        if args.capture_encoded and not encoded_frames:
            raise AssertionError("no Chrome encoded video frames were captured")
    except (AssertionError, ValueError) as error:
        blockers.append(f"{type(error).__name__}: {error}")

    blockers = list(dict.fromkeys(blockers))
    result = {
        "passed": not blockers,
        "blockers": blockers,
        "duration_seconds": round(time.monotonic() - started, 3),
        "connected": connected_at is not None,
        "screenshot_saved": screenshot_saved,
        "offer_bytes": len(offer),
        "answer_bytes": len(answer),
        "offer_received_at": exchange.offer_received_at,
        "answer_received_at": exchange.answer_received_at,
        "captured_encoded_frames": len(encoded_frames),
        "captured_encoded_bytes": sum(frame["size"] for frame in encoded_frames),
        "max_counters": counters,
        "media_paths_active": media_ready_at is not None,
        "qualified_connected_seconds": round(qualified_connected_seconds, 3),
        "requested_connected_seconds": args.connected_seconds,
        "final_connection_state": final_snapshot.get("connectionState"),
        "final_ice_connection_state": final_snapshot.get("iceConnectionState"),
        "selected_local_tracks": final_snapshot.get("localTracks", []),
        "remote_tracks": final_snapshot.get("remoteTracks", []),
        "remote_video": final_snapshot.get("remoteVideo", {}),
        "real_media": args.real_media,
        "capture_encoded": args.capture_encoded,
        "headed": args.headed,
        "argv": [sys.executable, *sys.argv],
        "output_directory": str(output),
    }
    save_json(
        output / "run-config.json",
        {
            "argv": result["argv"],
            "real_media": args.real_media,
            "capture_encoded": args.capture_encoded,
            "headed": args.headed,
            "timeout_seconds": args.timeout,
            "connected_seconds": args.connected_seconds,
        },
    )
    save_json(output / "result.json", result)
    print(json.dumps(result, indent=2, sort_keys=True), flush=True)
    return 0 if not blockers else 1


if __name__ == "__main__":
    raise SystemExit(main())
