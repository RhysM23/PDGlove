import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial
import threading
import time
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial.tools.list_ports
from scipy import signal


class SensorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Sensor Measurement App")
        self.root.geometry("1000x800")

        # Data storage
        self.data = []
        self.serial_port = None
        self.collecting = False
        self.collection_thread = None

        # Create UI components
        self.create_ui()

    def create_ui(self):
        # Main notebook with tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill='both', expand=True)

        # Create the tabs
        self.setup_tab = ttk.Frame(self.notebook)
        self.raw_data_tab = ttk.Frame(self.notebook)
        self.analysis_tab = ttk.Frame(self.notebook)

        self.notebook.add(self.setup_tab, text="Setup & Control")
        self.notebook.add(self.raw_data_tab, text="Raw Data")
        self.notebook.add(self.analysis_tab, text="Analysis")

        # Setup the control tab
        self.create_control_tab()

        # Setup the raw data tab
        self.create_raw_data_tab()

        # Setup the analysis tab
        self.create_analysis_tab()

        # Automatically populate port list
        self.refresh_ports()

    def create_control_tab(self):
        """Create the control panel tab"""
        # Main frame
        main_frame = ttk.Frame(self.setup_tab, padding=10)
        main_frame.pack(fill='both', expand=True)

        # Control panel frame
        control_frame = ttk.LabelFrame(main_frame, text="Control Panel", padding=10)
        control_frame.pack(fill='x', pady=10)

        # Serial port selection
        ttk.Label(control_frame, text="Serial Port:").grid(row=0, column=0, sticky='w', padx=5, pady=5)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(control_frame, textvariable=self.port_var, width=30)
        self.port_combo.grid(row=0, column=1, sticky='w', padx=5, pady=5)
        ttk.Button(control_frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=5, pady=5)

        # Measurement type selection
        ttk.Label(control_frame, text="Measurement Type:").grid(row=1, column=0, sticky='w', padx=5, pady=5)
        self.measure_type = ttk.Combobox(control_frame, values=["Tremor", "Bradykinesia", "Stiffness"], width=30)
        self.measure_type.grid(row=1, column=1, sticky='w', padx=5, pady=5)
        self.measure_type.current(0)  # Default to Tremor

        # Timeout setting
        ttk.Label(control_frame, text="Timeout (sec):").grid(row=2, column=0, sticky='w', padx=5, pady=5)
        self.timeout_var = tk.IntVar(value=45)  # Default to 45 seconds
        timeout_spinner = ttk.Spinbox(control_frame, from_=5, to=120, textvariable=self.timeout_var, width=5)
        timeout_spinner.grid(row=2, column=1, sticky='w', padx=5, pady=5)

        # Button frame
        button_frame = ttk.Frame(control_frame)
        button_frame.grid(row=3, column=0, columnspan=3, pady=10)

        # Measure button
        self.measure_btn = ttk.Button(button_frame, text="Start Measurement", command=self.start_measurement)
        self.measure_btn.pack(side='left', padx=5)

        # Abort button (initially disabled)
        self.abort_btn = ttk.Button(button_frame, text="Abort", command=self.abort_measurement, state='disabled')
        self.abort_btn.pack(side='left', padx=5)

        # Display data button (for partially complete measurements)
        self.display_btn = ttk.Button(button_frame, text="Display Data", command=self.display_data, state='disabled')
        self.display_btn.pack(side='left', padx=5)

        # Analyze button
        self.analyze_btn = ttk.Button(button_frame, text="Analyze Data", command=self.analyze_data, state='disabled')
        self.analyze_btn.pack(side='left', padx=5)

        # Status indicator
        status_frame = ttk.Frame(main_frame)
        status_frame.pack(fill='x', pady=5)

        ttk.Label(status_frame, text="Status:").pack(side='left', padx=5)
        self.status_var = tk.StringVar(value="Ready")
        status_label = ttk.Label(status_frame, textvariable=self.status_var, font=('Arial', 10, 'bold'))
        status_label.pack(side='left', padx=5)

        # Progress bar
        self.progress_var = tk.DoubleVar(value=0)
        self.progress_bar = ttk.Progressbar(main_frame, variable=self.progress_var, maximum=100)
        self.progress_bar.pack(fill='x', pady=5)

        # Log area
        log_frame = ttk.LabelFrame(main_frame, text="Activity Log", padding=10)
        log_frame.pack(fill='both', expand=True, pady=10)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=10)
        self.log_text.pack(fill='both', expand=True)
        self.log_text.config(state='disabled')

    def create_raw_data_tab(self):
        """Create the raw data visualization tab"""
        # Main frame for raw data visualization
        raw_frame = ttk.Frame(self.raw_data_tab, padding=10)
        raw_frame.pack(fill='both', expand=True)

        # Create matplotlib figure and subplots
        self.raw_fig = plt.Figure(figsize=(9, 8))

        # Create subplots
        self.sensor1_plot = self.raw_fig.add_subplot(311)
        self.sensor1_plot.set_title("Sensor 1")
        self.sensor1_plot.set_ylabel("Value")
        self.sensor1_plot.grid(True)

        self.sensor2_plot = self.raw_fig.add_subplot(312)
        self.sensor2_plot.set_title("Sensor 2")
        self.sensor2_plot.set_ylabel("Value")
        self.sensor2_plot.grid(True)

        self.sensor3_plot = self.raw_fig.add_subplot(313)
        self.sensor3_plot.set_title("Sensor 3")
        self.sensor3_plot.set_xlabel("Time (s)")
        self.sensor3_plot.set_ylabel("Value")
        self.sensor3_plot.grid(True)

        self.raw_fig.tight_layout()

        # Add figure to canvas
        self.raw_canvas = FigureCanvasTkAgg(self.raw_fig, master=raw_frame)
        self.raw_canvas.draw()
        self.raw_canvas.get_tk_widget().pack(fill='both', expand=True)

    def create_analysis_tab(self):
        """Create the analysis tab"""
        # Main frame for analysis
        analysis_frame = ttk.Frame(self.analysis_tab, padding=10)
        analysis_frame.pack(fill='both', expand=True)

        # Control frame for analysis options
        control_frame = ttk.Frame(analysis_frame)
        control_frame.pack(fill='x', pady=5)

        # Sensor selection for analysis
        ttk.Label(control_frame, text="Analyze Sensor:").pack(side='left', padx=5)
        self.analysis_sensor = ttk.Combobox(control_frame, values=["Sensor 1", "Sensor 2", "Sensor 3"], width=10)
        self.analysis_sensor.pack(side='left', padx=5)
        self.analysis_sensor.current(0)  # Default to Sensor 1

        # Filter checkbox
        self.filter_var = tk.BooleanVar(value=True)
        filter_check = ttk.Checkbutton(control_frame, text="Apply Bandpass Filter (1-20 Hz)", variable=self.filter_var)
        filter_check.pack(side='left', padx=20)

        # Update button
        update_btn = ttk.Button(control_frame, text="Update Analysis", command=self.analyze_data)
        update_btn.pack(side='left', padx=5)

        # Results frame
        results_frame = ttk.LabelFrame(analysis_frame, text="Analysis Results", padding=5)
        results_frame.pack(fill='x', pady=5)

        # Create results variables
        self.dominant_freq_var = tk.StringVar(value="-")
        self.tremor_type_var = tk.StringVar(value="-")

        # Results display
        ttk.Label(results_frame, text="Dominant Frequency:").grid(row=0, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(results_frame, textvariable=self.dominant_freq_var, font=('Arial', 10, 'bold')).grid(row=0, column=1,
                                                                                                       sticky='w',
                                                                                                       padx=5, pady=2)
        ttk.Label(results_frame, text="Hz").grid(row=0, column=2, sticky='w', padx=5, pady=2)

        ttk.Label(results_frame, text="Interpretation:").grid(row=1, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(results_frame, textvariable=self.tremor_type_var, font=('Arial', 10, 'bold')).grid(row=1, column=1,
                                                                                                     columnspan=2,
                                                                                                     sticky='w', padx=5,
                                                                                                     pady=2)

        # Create figure for analysis plots
        self.analysis_fig = plt.Figure(figsize=(9, 8))

        # Time domain plot
        self.time_plot = self.analysis_fig.add_subplot(311)
        self.time_plot.set_title("Time Domain Signal")
        self.time_plot.set_ylabel("Amplitude")
        self.time_plot.grid(True)

        # Frequency domain plot
        self.freq_plot = self.analysis_fig.add_subplot(312)
        self.freq_plot.set_title("Frequency Spectrum")
        self.freq_plot.set_xlabel("Frequency (Hz)")
        self.freq_plot.set_ylabel("Amplitude")
        self.freq_plot.grid(True)

        # Spectrogram plot
        self.spec_plot = self.analysis_fig.add_subplot(313)
        self.spec_plot.set_title("Spectrogram")
        self.spec_plot.set_xlabel("Time (s)")
        self.spec_plot.set_ylabel("Frequency (Hz)")

        self.analysis_fig.tight_layout()

        # Add figure to canvas
        self.analysis_canvas = FigureCanvasTkAgg(self.analysis_fig, master=analysis_frame)
        self.analysis_canvas.draw()
        self.analysis_canvas.get_tk_widget().pack(fill='both', expand=True, pady=10)

    def refresh_ports(self):
        """Update the available serial ports in the dropdown"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo['values'] = ports
        if ports:
            self.port_combo.current(0)

    def start_measurement(self):
        """Start a measurement based on selected type"""
        # Validate inputs
        if not self.port_var.get():
            messagebox.showerror("Error", "Please select a serial port")
            return

        if not self.measure_type.get():
            messagebox.showerror("Error", "Please select a measurement type")
            return

        # Clear previous data
        self.data = []

        # Update UI states
        self.measure_btn.configure(state='disabled')
        self.abort_btn.configure(state='normal')
        self.display_btn.configure(state='disabled')
        self.analyze_btn.configure(state='disabled')

        # Reset progress
        self.progress_var.set(0)

        # Log the action
        self.log(f"Starting {self.measure_type.get()} measurement on port {self.port_var.get()}")

        # Start data collection in a separate thread
        self.collecting = True
        self.collection_thread = threading.Thread(target=self.collect_data, daemon=True)
        self.collection_thread.start()

    def abort_measurement(self):
        """Abort the current measurement"""
        if self.collecting:
            self.collecting = False
            self.status_var.set("Measurement aborted")
            self.log("Measurement aborted by user")

            # Give the thread a moment to clean up
            self.root.after(500, self.reset_ui_after_abort)

    def reset_ui_after_abort(self):
        """Reset the UI after an aborted measurement"""
        self.measure_btn.configure(state='normal')
        self.abort_btn.configure(state='disabled')

        # Only enable display/analyze if we got some data
        if len(self.data) > 0:
            self.display_btn.configure(state='normal')
            self.analyze_btn.configure(state='normal')
            self.status_var.set(f"Measurement aborted - {len(self.data)} points collected")
        else:
            self.status_var.set("Measurement aborted - no data collected")

    def collect_data(self):
        """Collect data from the ESP32 via serial port"""
        try:
            # Connect to serial port
            self.status_var.set("Connecting to device...")
            self.log(f"Connecting to serial port {self.port_var.get()}")
            self.serial_port = serial.Serial(self.port_var.get(), 115200, timeout=2)
            time.sleep(1)  # Allow time for connection to establish

            # Send command based on measurement type
            self.status_var.set("Sending command...")
            command = ""
            if self.measure_type.get() == "Tremor":
                command = "TREM\n"
            elif self.measure_type.get() == "Bradykinesia":
                command = "BRAD\n"
            elif self.measure_type.get() == "Stiffness":
                command = "STIF\n"

            self.log(f"Sending command: {command.strip()}")
            self.serial_port.write(command.encode())

            # Wait for and collect data
            self.status_var.set("Waiting for data...")
            max_index = None
            last_data_time = time.time()
            data_timeout = self.timeout_var.get()  # Get timeout from UI

            while self.collecting:
                if time.time() - last_data_time > data_timeout:
                    # Timeout occurred - show a message but don't lose data
                    self.root.after(0, lambda: self.status_var.set(f"Timeout - no data for {data_timeout} seconds"))
                    self.log(f"Data collection timeout after {data_timeout} seconds")
                    break

                try:
                    line = self.serial_port.readline().decode().strip()

                    if line.startswith("DATA"):
                        # Parse data line: "DATAindex×max_index×time_x_ms×sensor1×sensor2×sensor3"
                        parts = line[4:].split('x')
                        if len(parts) == 6:
                            index = int(parts[0])
                            max_index = int(parts[1])
                            time_ms = int(parts[2])
                            sensor1 = int(parts[3])
                            sensor2 = int(parts[4])
                            sensor3 = int(parts[5])

                            # Store data
                            self.data.append([index, time_ms, sensor1, sensor2, sensor3])

                            # Reset timeout timer
                            last_data_time = time.time()

                            # Update progress
                            if max_index > 0:
                                progress = (index / max_index) * 100
                                self.root.after(0, lambda p=progress: self.progress_var.set(p))
                                self.root.after(0, lambda i=index, m=max_index:
                                self.status_var.set(f"Receiving data: {i + 1}/{m + 1} points"))

                            # Check if we're done - allow for off-by-one errors
                            if index >= max_index - 1:  # Consider "close enough" to be done
                                self.log(
                                    f"Data collection complete: {len(self.data)} of {max_index + 1} points received")
                                self.root.after(0, lambda: self.status_var.set(
                                    f"Measurement complete ({len(self.data)}/{max_index + 1} points)"))
                                break
                except UnicodeDecodeError:
                    # Handle potential garbled data
                    continue
                except Exception as e:
                    # Log other errors but keep trying
                    self.log(f"Error reading line: {e}")
                    continue

            # Close serial port
            try:
                if self.serial_port and self.serial_port.is_open:
                    self.serial_port.close()
                    self.log("Serial port closed")
            except Exception as e:
                self.log(f"Error closing serial port: {e}")
            self.serial_port = None

            # Update UI in main thread
            self.root.after(0, self.measurement_complete)

        except Exception as e:
            # Show error in main thread
            self.log(f"Collection error: {e}")
            self.root.after(0, lambda: self.show_error(f"Error: {str(e)}"))

    def measurement_complete(self):
        """Process data when measurement is complete"""
        self.measure_btn.configure(state='normal')
        self.abort_btn.configure(state='disabled')
        self.display_btn.configure(state='normal')
        self.analyze_btn.configure(state='normal')
        self.progress_var.set(100)

        # Display the data
        self.display_data()

        # Switch to the Raw Data tab
        self.notebook.select(1)  # Index 1 is the Raw Data tab

    def display_data(self):
        """Display the raw data on the Raw Data tab"""
        if not self.data:
            messagebox.showinfo("Info", "No data to display")
            return

        # Extract data columns
        time_data = [item[1] / 1000.0 for item in self.data]  # Convert to seconds
        sensor1_data = [item[2] for item in self.data]
        sensor2_data = [item[3] for item in self.data]
        sensor3_data = [item[4] for item in self.data]

        # Clear previous plots
        self.sensor1_plot.clear()
        self.sensor2_plot.clear()
        self.sensor3_plot.clear()

        # Plot new data
        self.sensor1_plot.plot(time_data, sensor1_data, 'g-')
        self.sensor1_plot.set_title(f"Sensor 1 - {self.measure_type.get()}")
        self.sensor1_plot.set_ylabel("Value")
        self.sensor1_plot.set_xlim(0, max(time_data) if time_data else 10)
        self.sensor1_plot.set_ylim(0, 4095)  # Full range of analog values
        self.sensor1_plot.grid(True)

        self.sensor2_plot.plot(time_data, sensor2_data, 'b-')
        self.sensor2_plot.set_title("Sensor 2")
        self.sensor2_plot.set_ylabel("Value")
        self.sensor2_plot.set_xlim(0, max(time_data) if time_data else 10)
        self.sensor2_plot.set_ylim(0, 4095)
        self.sensor2_plot.grid(True)

        self.sensor3_plot.plot(time_data, sensor3_data, 'm-')
        self.sensor3_plot.set_title("Sensor 3")
        self.sensor3_plot.set_xlabel("Time (s)")
        self.sensor3_plot.set_ylabel("Value")
        self.sensor3_plot.set_xlim(0, max(time_data) if time_data else 10)
        self.sensor3_plot.set_ylim(0, 4095)
        self.sensor3_plot.grid(True)

        # Update the layout and redraw
        self.raw_fig.tight_layout()
        self.raw_canvas.draw()

        # Update status with data info
        self.status_var.set(f"Displaying {len(self.data)} data points")
        self.log(f"Data displayed on Raw Data tab")

    def analyze_data(self):
        """Perform frequency analysis on the data"""
        if not self.data:
            messagebox.showinfo("Info", "No data to analyze")
            return

        # Extract data columns
        time_data = np.array([item[1] / 1000.0 for item in self.data])  # Convert to seconds

        # Select sensor data based on dropdown
        sensor_index = self.analysis_sensor.current() + 2  # +2 because index and time are columns 0,1
        if self.analysis_sensor.get() == "Sensor 1":
            sensor_data = np.array([item[2] for item in self.data])
            sensor_name = "Sensor 1"
        elif self.analysis_sensor.get() == "Sensor 2":
            sensor_data = np.array([item[3] for item in self.data])
            sensor_name = "Sensor 2"
        else:
            sensor_data = np.array([item[4] for item in self.data])
            sensor_name = "Sensor 3"

        # Remove DC component (mean)
        sensor_data = sensor_data - np.mean(sensor_data)

        # Calculate sampling rate
        if len(time_data) > 1:
            # Calculate average time step
            dt = np.mean(np.diff(time_data))
            fs = 1.0 / dt  # Sampling frequency in Hz
        else:
            fs = 100  # Default sampling rate

        # Apply bandpass filter if requested
        filtered_data = sensor_data.copy()
        if self.filter_var.get():
            # Bandpass filter (1-20 Hz)
            nyquist = 0.5 * fs
            low = 1.0 / nyquist
            high = 20.0 / nyquist
            b, a = signal.butter(4, [low, high], btype='band')
            filtered_data = signal.filtfilt(b, a, sensor_data)

        # Time domain plot
        self.time_plot.clear()
        self.time_plot.plot(time_data, filtered_data)
        self.time_plot.set_title(f"{sensor_name} - Time Domain")
        self.time_plot.set_xlabel("Time (s)")
        self.time_plot.set_ylabel("Amplitude")
        self.time_plot.grid(True)

        # FFT analysis
        n = len(filtered_data)
        fft_result = np.fft.rfft(filtered_data)
        fft_freq = np.fft.rfftfreq(n, d=1.0 / fs)
        fft_magnitude = np.abs(fft_result) * 2.0 / n

        # FFT plot
        self.freq_plot.clear()
        self.freq_plot.plot(fft_freq, fft_magnitude)
        self.freq_plot.set_title("Frequency Spectrum")
        self.freq_plot.set_xlabel("Frequency (Hz)")
        self.freq_plot.set_ylabel("Amplitude")
        self.freq_plot.set_xlim(0, min(20, fs / 2))  # Limit to relevant frequencies
        self.freq_plot.grid(True)

        # Find dominant frequency
        # Only consider 1-20 Hz range for tremor analysis
        mask = (fft_freq >= 1.0) & (fft_freq <= 20.0)
        if np.any(mask):
            tremor_freqs = fft_freq[mask]
            tremor_mags = fft_magnitude[mask]

            if len(tremor_mags) > 0:
                # Find peak frequency
                peak_idx = np.argmax(tremor_mags)
                dominant_freq = tremor_freqs[peak_idx]

                # Mark the dominant frequency
                self.freq_plot.plot(dominant_freq, tremor_mags[peak_idx], 'ro')
                self.freq_plot.annotate(f"{dominant_freq:.2f} Hz",
                                        xy=(dominant_freq, tremor_mags[peak_idx]),
                                        xytext=(5, 5), textcoords='offset points')

                # Update results display
                self.dominant_freq_var.set(f"{dominant_freq:.2f}")

                # Interpret tremor type
                if 3.0 <= dominant_freq <= 7.0:
                    interpretation = "Consistent with Parkinsonian tremor (3-7 Hz)"
                elif 7.0 < dominant_freq <= 12.0:
                    interpretation = "Consistent with essential/physiological tremor (7-12 Hz)"
                else:
                    interpretation = "Outside typical tremor ranges"

                self.tremor_type_var.set(interpretation)
                self.log(f"Dominant frequency: {dominant_freq:.2f} Hz - {interpretation}")
            else:
                self.dominant_freq_var.set("N/A")
                self.tremor_type_var.set("No significant frequency components detected")
        else:
            self.dominant_freq_var.set("N/A")
            self.tremor_type_var.set("No data in relevant frequency range")

        # Compute spectrogram
        self.spec_plot.clear()
        f, t, Sxx = signal.spectrogram(filtered_data, fs, nperseg=min(256, len(filtered_data)))

        # Plot spectrogram
        spec = self.spec_plot.pcolormesh(t, f, 10 * np.log10(Sxx + 1e-10), shading='gouraud')
        self.spec_plot.set_title("Spectrogram")
        self.spec_plot.set_xlabel("Time (s)")
        self.spec_plot.set_ylabel("Frequency (Hz)")
        self.spec_plot.set_ylim(0, 20)  # Limit to relevant frequencies
        self.analysis_fig.colorbar(spec, ax=self.spec_plot, label='Power/Frequency (dB/Hz)')

        # Update the layout and redraw
        self.analysis_fig.tight_layout()
        self.analysis_canvas.draw()

        # Switch to analysis tab
        self.notebook.select(2)  # Index 2 is the Analysis tab

    def log(self, message):
        """Add a message to the log with timestamp"""
        timestamp = time.strftime("%H:%M:%S")
        log_msg = f"[{timestamp}] {message}\n"

        # Update the log text widget
        self.log_text.config(state='normal')
        self.log_text.insert(tk.END, log_msg)
        self.log_text.see(tk.END)  # Scroll to the end
        self.log_text.config(state='disabled')

    def show_error(self, message):
        """Display an error message and reset UI"""
        messagebox.showerror("Error", message)
        self.status_var.set("Ready")
        self.measure_btn.configure(state='normal')
        self.abort_btn.configure(state='disabled')
        self.collecting = False
        self.log(f"ERROR: {message}")


if __name__ == "__main__":
    root = tk.Tk()
    app = SensorApp(root)
    root.mainloop()