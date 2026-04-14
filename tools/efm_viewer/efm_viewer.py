#!/usr/bin/env python3

import json
import queue
import threading
import time
import tkinter as tk
from collections import deque
from tkinter import messagebox, ttk

import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

try:
	import serial
	from serial.tools import list_ports
except ImportError as exc:
	raise SystemExit(
		"pyserial is required. Install it with: pip install pyserial"
	) from exc


ADC_IDS = (1, 2)
CHANNEL_IDS = (1, 2, 3, 4)
DEFAULT_BAUD = 115200
DEFAULT_WINDOW_SECONDS = 10.0
DEFAULT_MAX_POINTS = 20000


class SerialReader(threading.Thread):
	def __init__(self, port: str, baud: int, output_queue: queue.Queue):
		super().__init__(daemon=True)
		self.port = port
		self.baud = baud
		self.output_queue = output_queue
		self._stop_event = threading.Event()
		self.serial_handle = None

	def stop(self) -> None:
		self._stop_event.set()

	def run(self) -> None:
		try:
			self.serial_handle = serial.Serial(self.port, self.baud, timeout=0.2)
			start = time.time()
			self.output_queue.put(("status", f"Connected to {self.port} @ {self.baud}"))

			while not self._stop_event.is_set():
				raw = self.serial_handle.readline()
				if not raw:
					continue

				try:
					line = raw.decode("utf-8", errors="replace").strip()
					payload = json.loads(line)
				except (json.JSONDecodeError, UnicodeDecodeError):
					continue

				adc = payload.get("adc")
				channel = payload.get("channel")
				voltage = payload.get("voltage")
				if not isinstance(adc, int) or not isinstance(channel, int):
					continue
				if adc not in ADC_IDS or channel not in CHANNEL_IDS:
					continue

				try:
					value = float(voltage)
				except (TypeError, ValueError):
					continue

				timestamp = time.time() - start
				self.output_queue.put(("sample", adc, channel, timestamp, value))
		except Exception as exc:  # noqa: BLE001
			self.output_queue.put(("error", f"Serial error: {exc}"))
		finally:
			if self.serial_handle and self.serial_handle.is_open:
				self.serial_handle.close()
			self.output_queue.put(("status", "Disconnected"))


class PortSelectorDialog(tk.Toplevel):
	def __init__(self, parent: tk.Tk):
		super().__init__(parent)
		self.title("Select Serial Port")
		self.resizable(False, False)
		self.transient(parent)
		self.grab_set()

		self.selected_port = tk.StringVar(value="")
		self.baud = tk.StringVar(value=str(DEFAULT_BAUD))
		self.result = None

		frame = ttk.Frame(self, padding=12)
		frame.grid(row=0, column=0, sticky="nsew")

		ttk.Label(frame, text="Serial port").grid(row=0, column=0, sticky="w")
		self.port_combo = ttk.Combobox(frame, textvariable=self.selected_port, width=34)
		self.port_combo.grid(row=1, column=0, columnspan=3, sticky="ew", pady=(2, 10))

		ttk.Button(frame, text="Refresh", command=self.refresh_ports).grid(
			row=2, column=0, sticky="ew", padx=(0, 6)
		)
		ttk.Button(frame, text="Auto-detect", command=self.autodetect_port).grid(
			row=2, column=1, sticky="ew", padx=(0, 6)
		)

		ttk.Label(frame, text="Baud").grid(row=3, column=0, sticky="w", pady=(10, 0))
		ttk.Entry(frame, textvariable=self.baud, width=12).grid(
			row=4, column=0, sticky="w"
		)

		ttk.Button(frame, text="Connect", command=self.accept).grid(
			row=4, column=1, sticky="ew", pady=(8, 0)
		)
		ttk.Button(frame, text="Cancel", command=self.cancel).grid(
			row=4, column=2, sticky="ew", pady=(8, 0)
		)

		frame.columnconfigure(0, weight=1)
		frame.columnconfigure(1, weight=1)
		frame.columnconfigure(2, weight=1)

		self.refresh_ports()
		self.autodetect_port()
		self.protocol("WM_DELETE_WINDOW", self.cancel)

	def refresh_ports(self) -> None:
		ports = [p.device for p in list_ports.comports()]
		self.port_combo["values"] = ports
		if self.selected_port.get() not in ports:
			self.selected_port.set(ports[0] if ports else "")

	def autodetect_port(self) -> None:
		ports = [p.device for p in list_ports.comports()]
		if not ports:
			self.selected_port.set("")
			return

		preferred = None
		for port_name in ports:
			lowered = port_name.lower()
			if "usb" in lowered or "acm" in lowered or "com" in lowered or "tty" in lowered:
				preferred = port_name
				break
		self.selected_port.set(preferred or ports[0])

	def accept(self) -> None:
		port = self.selected_port.get().strip()
		if not port:
			messagebox.showerror("Missing port", "Select a serial port first.")
			return

		try:
			baud = int(self.baud.get())
		except ValueError:
			messagebox.showerror("Invalid baud", "Baud must be an integer.")
			return

		self.result = (port, baud)
		self.destroy()

	def cancel(self) -> None:
		self.result = None
		self.destroy()


class EfmViewerApp:
	def __init__(self, root: tk.Tk):
		self.root = root
		self.root.title("EFM Live ADC Viewer")
		self.root.geometry("1200x720")

		self.data_queue = queue.Queue()
		self.reader = None

		self.time_window_seconds = tk.DoubleVar(value=DEFAULT_WINDOW_SECONDS)
		self.auto_scale = tk.BooleanVar(value=True)
		self.y_min = tk.DoubleVar(value=0.0)
		self.y_max = tk.DoubleVar(value=3.3)
		self.status_text = tk.StringVar(value="Not connected")

		self.channel_enabled = {
			(adc, channel): tk.BooleanVar(value=True)
			for adc in ADC_IDS
			for channel in CHANNEL_IDS
		}

		self.series = {
			(adc, channel): {
				"t": deque(maxlen=DEFAULT_MAX_POINTS),
				"v": deque(maxlen=DEFAULT_MAX_POINTS),
			}
			for adc in ADC_IDS
			for channel in CHANNEL_IDS
		}

		self._build_ui()
		self._open_port_selector_and_connect()
		self._schedule_update()

		self.root.protocol("WM_DELETE_WINDOW", self.on_close)

	def _build_ui(self) -> None:
		container = ttk.Frame(self.root)
		container.pack(fill="both", expand=True)

		control_frame = ttk.Frame(container, padding=10)
		control_frame.pack(side="left", fill="y")

		plot_frame = ttk.Frame(container, padding=(0, 10, 10, 10))
		plot_frame.pack(side="left", fill="both", expand=True)

		ttk.Button(control_frame, text="Connect Port", command=self._open_port_selector_and_connect).pack(
			fill="x", pady=(0, 8)
		)
		ttk.Label(control_frame, textvariable=self.status_text, foreground="#2a5").pack(
			fill="x", pady=(0, 12)
		)

		ttk.Label(control_frame, text="Time window (s)").pack(anchor="w")
		ttk.Entry(control_frame, textvariable=self.time_window_seconds, width=14).pack(
			anchor="w", pady=(0, 10)
		)

		auto_scale_checkbox = ttk.Checkbutton(
			control_frame,
			text="Auto Y-scale",
			variable=self.auto_scale,
			command=self._update_scale_inputs_state,
		)
		auto_scale_checkbox.pack(anchor="w", pady=(0, 8))

		scale_frame = ttk.Frame(control_frame)
		scale_frame.pack(fill="x", pady=(0, 12))

		ttk.Label(scale_frame, text="Y min").grid(row=0, column=0, sticky="w")
		self.y_min_entry = ttk.Entry(scale_frame, textvariable=self.y_min, width=10)
		self.y_min_entry.grid(row=0, column=1, sticky="w", padx=(6, 0))
		ttk.Label(scale_frame, text="Y max").grid(row=1, column=0, sticky="w", pady=(6, 0))
		self.y_max_entry = ttk.Entry(scale_frame, textvariable=self.y_max, width=10)
		self.y_max_entry.grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))

		ttk.Label(control_frame, text="Channels").pack(anchor="w")
		channels_frame = ttk.Frame(control_frame)
		channels_frame.pack(fill="x", pady=(6, 0))

		for adc in ADC_IDS:
			adc_label = ttk.Label(channels_frame, text=f"ADC {adc}")
			adc_label.pack(anchor="w", pady=(8 if adc > 1 else 0, 2))
			for channel in CHANNEL_IDS:
				var = self.channel_enabled[(adc, channel)]
				ttk.Checkbutton(channels_frame, text=f"Ch {channel}", variable=var).pack(anchor="w")

		self.figure, self.ax = plt.subplots(figsize=(10, 6), dpi=100)
		self.canvas = FigureCanvasTkAgg(self.figure, master=plot_frame)
		self.canvas.get_tk_widget().pack(fill="both", expand=True)

		colors = {
			(1, 1): "#1f77b4",
			(1, 2): "#ff7f0e",
			(1, 3): "#2ca02c",
			(1, 4): "#d62728",
			(2, 1): "#9467bd",
			(2, 2): "#8c564b",
			(2, 3): "#e377c2",
			(2, 4): "#7f7f7f",
		}
		self.lines = {}
		for adc in ADC_IDS:
			for channel in CHANNEL_IDS:
				label = f"ADC {adc} Ch {channel}"
				line, = self.ax.plot([], [], label=label, color=colors[(adc, channel)], linewidth=1.6)
				self.lines[(adc, channel)] = line

		self.ax.set_xlabel("Time (s)")
		self.ax.set_ylabel("Voltage (V)")
		self.ax.grid(True, alpha=0.3)
		self.ax.legend(loc="upper right", ncol=2)
		self._update_scale_inputs_state()

	def _update_scale_inputs_state(self) -> None:
		state = "disabled" if self.auto_scale.get() else "normal"
		self.y_min_entry.config(state=state)
		self.y_max_entry.config(state=state)

	def _open_port_selector_and_connect(self) -> None:
		dialog = PortSelectorDialog(self.root)
		self.root.wait_window(dialog)
		if not dialog.result:
			return

		port, baud = dialog.result
		self._disconnect_reader()
		self.reader = SerialReader(port, baud, self.data_queue)
		self.reader.start()

	def _disconnect_reader(self) -> None:
		if self.reader is not None:
			self.reader.stop()
			self.reader = None

	def _trim_old_samples(self, newest_time: float) -> None:
		try:
			window = float(self.time_window_seconds.get())
			if window <= 0:
				return
		except (ValueError, tk.TclError):
			return

		lower_bound = newest_time - window
		for key in self.series:
			t_deque = self.series[key]["t"]
			v_deque = self.series[key]["v"]
			while t_deque and t_deque[0] < lower_bound:
				t_deque.popleft()
				v_deque.popleft()

	def _process_queue(self) -> None:
		newest_time = None
		while True:
			try:
				item = self.data_queue.get_nowait()
			except queue.Empty:
				break

			kind = item[0]
			if kind == "sample":
				_, adc, channel, timestamp, value = item
				self.series[(adc, channel)]["t"].append(timestamp)
				self.series[(adc, channel)]["v"].append(value)
				newest_time = timestamp
			elif kind == "status":
				self.status_text.set(item[1])
			elif kind == "error":
				self.status_text.set(item[1])

		if newest_time is not None:
			self._trim_old_samples(newest_time)

	def _update_plot(self) -> None:
		self._process_queue()

		visible_times = []
		visible_values = []

		for key, line in self.lines.items():
			enabled = self.channel_enabled[key].get()
			t_data = list(self.series[key]["t"])
			v_data = list(self.series[key]["v"])

			if enabled and t_data:
				line.set_data(t_data, v_data)
				line.set_visible(True)
				visible_times.extend((t_data[0], t_data[-1]))
				visible_values.extend(v_data)
			else:
				line.set_data([], [])
				line.set_visible(False)

		if visible_times:
			left = min(visible_times)
			right = max(visible_times)
			if right - left < 1e-6:
				right = left + max(0.1, self.time_window_seconds.get())
			self.ax.set_xlim(left, right)
		else:
			window = max(0.1, self.time_window_seconds.get())
			self.ax.set_xlim(0.0, window)

		if self.auto_scale.get():
			if visible_values:
				vmin = min(visible_values)
				vmax = max(visible_values)
				if vmax - vmin < 1e-9:
					vmax = vmin + 0.1
				pad = (vmax - vmin) * 0.1
				self.ax.set_ylim(vmin - pad, vmax + pad)
			else:
				self.ax.set_ylim(0.0, 3.3)
		else:
			try:
				ymin = float(self.y_min.get())
				ymax = float(self.y_max.get())
				if ymax <= ymin:
					ymax = ymin + 0.1
				self.ax.set_ylim(ymin, ymax)
			except (ValueError, tk.TclError):
				pass

		self.canvas.draw_idle()

	def _schedule_update(self) -> None:
		self._update_plot()
		self.root.after(50, self._schedule_update)

	def on_close(self) -> None:
		self._disconnect_reader()
		self.root.destroy()


def main() -> None:
	root = tk.Tk()
	app = EfmViewerApp(root)
	_ = app
	root.mainloop()


if __name__ == "__main__":
	main()
