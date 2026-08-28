#!/usr/bin/env python3
"""Capture RF_Monitor UART output and save RSSI windows as CSV and PNG.

Install dependencies:
    python3 -m pip install pyserial matplotlib

Example:
    python3 tools/capture_rssi.py --label background --windows 20
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


WINDOW_START = "WINDOW_START"
WINDOW_END = "WINDOW_END"
DEFAULT_OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "test_data"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Capture STM32 UART output, save the complete terminal log, and "
            "export valid RSSI windows as CSV and PNG files."
        )
    )
    parser.add_argument(
        "--port",
        help="Serial port, for example /dev/cu.usbmodem1103. Auto-detected if omitted.",
    )
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--label",
        default="unlabeled",
        help="Class name used for the output folder and filenames.",
    )
    parser.add_argument(
        "--windows",
        type=int,
        default=20,
        help="Number of valid windows to capture (default: 20).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT_DIRECTORY,
        help="Base output directory (default: tools/test_data).",
    )
    parser.add_argument(
        "--no-plots",
        action="store_true",
        help="Save terminal and CSV files without generating PNG plots.",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List available serial ports and exit.",
    )
    args = parser.parse_args()

    if args.windows < 1:
        parser.error("--windows must be at least 1")
    if args.baud < 1:
        parser.error("--baud must be positive")

    return args


def load_serial_module() -> Any:
    try:
        import serial  # type: ignore[import-not-found]
    except ImportError:
        print(
            "Missing pyserial. Install it with:\n"
            "  python3 -m pip install pyserial matplotlib",
            file=sys.stderr,
        )
        raise SystemExit(2)
    return serial


def available_ports() -> list[Any]:
    load_serial_module()
    from serial.tools import list_ports  # type: ignore[import-not-found]

    return list(list_ports.comports())


def print_ports(ports: list[Any]) -> None:
    if not ports:
        print("No serial ports found.")
        return

    for port in ports:
        description = port.description or "unknown device"
        print(f"{port.device}: {description}")


def choose_port(requested_port: str | None, ports: list[Any]) -> str:
    if requested_port:
        return requested_port

    likely_ports = [
        port
        for port in ports
        if "usbmodem" in port.device.lower()
        or "ttyacm" in port.device.lower()
        or "stlink" in (port.description or "").lower()
        or "stmicro" in (port.manufacturer or "").lower()
    ]

    if len(likely_ports) == 1:
        return likely_ports[0].device

    if not likely_ports:
        print("No likely STM32 serial port was found.", file=sys.stderr)
    else:
        print("More than one likely STM32 serial port was found.", file=sys.stderr)

    print_ports(ports)
    print("Pass the correct device using --port.", file=sys.stderr)
    raise SystemExit(2)


def safe_label(label: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_-]+", "_", label.strip()).strip("_")
    return cleaned or "unlabeled"


def parse_metadata(line: str) -> dict[str, str]:
    metadata: dict[str, str] = {}
    for token in line.split()[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            metadata[key] = value
    return metadata


def parse_samples(lines: list[str]) -> list[int]:
    sample_text = "".join(lines)
    return [int(value.strip()) for value in sample_text.split(",") if value.strip()]


def save_csv(
    path: Path,
    samples: list[int],
    sample_rate_hz: int,
) -> None:
    with path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(
            ["sample_index", "time_ms", "rssi_half_dbm", "rssi_dbm"]
        )
        for index, sample in enumerate(samples):
            time_ms = index * 1000.0 / sample_rate_hz
            writer.writerow([index, f"{time_ms:.3f}", sample, f"{sample / 2.0:.1f}"])


def save_plot(
    path: Path,
    samples: list[int],
    sample_rate_hz: int,
    label: str,
    sequence: str,
) -> bool:
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return False

    times_ms = [index * 1000.0 / sample_rate_hz for index in range(len(samples))]
    rssi_dbm = [sample / 2.0 for sample in samples]

    figure, axis = plt.subplots(figsize=(10, 4.5))
    axis.plot(times_ms, rssi_dbm, linewidth=1.2)
    axis.set_title(f"{label} - RSSI window {sequence}")
    axis.set_xlabel("Time (ms)")
    axis.set_ylabel("RSSI (dBm)")
    axis.grid(True, alpha=0.3)
    figure.tight_layout()
    figure.savefig(path, dpi=160)
    plt.close(figure)
    return True


def capture(args: argparse.Namespace, port_name: str) -> int:
    serial = load_serial_module()
    label = safe_label(args.label)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    session_directory = args.output / label / timestamp
    session_directory.mkdir(parents=True, exist_ok=False)
    terminal_path = session_directory / "terminal.log"

    print(f"Opening {port_name} at {args.baud} baud")
    print(f"Saving capture to {session_directory.resolve()}")
    print("Press Ctrl-C to stop early.\n")

    valid_windows = 0
    current_metadata: dict[str, str] | None = None
    current_sample_lines: list[str] = []
    plot_dependency_warning_shown = False

    try:
        with serial.Serial(port_name, args.baud, timeout=1) as uart, terminal_path.open(
            "w", encoding="utf-8", buffering=1
        ) as terminal_log:
            while valid_windows < args.windows:
                raw_line = uart.readline()
                if not raw_line:
                    continue

                line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")
                print(line, flush=True)
                terminal_log.write(line + "\n")

                if line.startswith(WINDOW_START):
                    current_metadata = parse_metadata(line)
                    current_sample_lines = []
                    continue

                if line == WINDOW_END:
                    if current_metadata is None:
                        print("Warning: WINDOW_END received without WINDOW_START.")
                        continue

                    try:
                        samples = parse_samples(current_sample_lines)
                        expected_count = int(current_metadata.get("count", "512"))
                        sample_rate_hz = int(
                            current_metadata.get("rate_hz", "1000")
                        )
                    except ValueError as error:
                        print(f"Warning: malformed RSSI window skipped: {error}")
                        current_metadata = None
                        current_sample_lines = []
                        continue

                    if len(samples) != expected_count:
                        print(
                            "Warning: incomplete window skipped: "
                            f"expected {expected_count}, received {len(samples)}"
                        )
                        current_metadata = None
                        current_sample_lines = []
                        continue

                    valid_windows += 1
                    sequence = current_metadata.get("sequence", str(valid_windows))
                    file_stem = f"{label}_window_{valid_windows:04d}_seq_{sequence}"
                    csv_path = session_directory / f"{file_stem}.csv"
                    save_csv(csv_path, samples, sample_rate_hz)

                    saved_message = f"Saved {csv_path.name}"
                    if not args.no_plots:
                        plot_path = session_directory / f"{file_stem}.png"
                        if save_plot(
                            plot_path,
                            samples,
                            sample_rate_hz,
                            label,
                            sequence,
                        ):
                            saved_message += f" and {plot_path.name}"
                        elif not plot_dependency_warning_shown:
                            print(
                                "Matplotlib is not installed; CSV capture will continue "
                                "without plots. Install it with:\n"
                                "  python3 -m pip install matplotlib"
                            )
                            plot_dependency_warning_shown = True

                    print(f"{saved_message} ({valid_windows}/{args.windows})")
                    current_metadata = None
                    current_sample_lines = []
                    continue

                if current_metadata is not None:
                    current_sample_lines.append(line)

    except KeyboardInterrupt:
        print("\nCapture stopped by user.")
    except serial.SerialException as error:
        print(f"Serial error: {error}", file=sys.stderr)
        return 1

    print(f"Captured {valid_windows} valid window(s).")
    print(f"Terminal log: {terminal_path.resolve()}")
    return 0


def main() -> int:
    args = parse_arguments()
    ports = available_ports()

    if args.list_ports:
        print_ports(ports)
        return 0

    port_name = choose_port(args.port, ports)
    return capture(args, port_name)


if __name__ == "__main__":
    raise SystemExit(main())
