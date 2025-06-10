import tkinter as tk
from tkinter import ttk, messagebox
import threading
import time

# Import modules
from data_acquisition import SerialDataCollector
from data_visualization import DataVisualizer
from frequency_analysis import FrequencyAnalyzer
from movement_analysis import MovementAnalyzer
from force_analysis import ForceAnalyzer
from bradykinesia_comparison import BradykinesiaComparison
from tremor_comparison import TremorComparison  # New import
from ui_components import create_tab, create_logger


class SensorApp:
    def __init__(self, root):
        self.measurement_mode = None
        self.root = root
        self.root.title("Sensor Measurement App")

        # Data storage
        self.data = []
        self.collecting = False
        self.collection_thread = None

        # Create the main notebook with tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill='both', expand=True)

        # Create tabs
        self.setup_tab = ttk.Frame(self.notebook)
        self.raw_data_tab = ttk.Frame(self.notebook)
        self.analysis_tab = ttk.Frame(self.notebook)

        self.notebook.add(self.setup_tab, text="Setup & Control")
        self.notebook.add(self.raw_data_tab, text="Raw Data")
        self.notebook.add(self.analysis_tab, text="Analysis")

        # Initialize UI components
        self.initialize_ui()

        # Initialize modules
        self.data_collector = SerialDataCollector(self)
        self.data_visualizer = DataVisualizer(self)
        self.frequency_analyzer = FrequencyAnalyzer(self)
        self.movement_analyzer = MovementAnalyzer(self)
        self.force_analyzer = ForceAnalyzer(self)
        self.bradykinesia_comparison = BradykinesiaComparison(self)
        self.tremor_comparison = TremorComparison(self)  # New analyzer

        # Refresh serial ports
        self.refresh_ports()

    def initialize_ui(self):
        """Initialize all UI components"""
        # Main control panel in setup tab
        control_frame = ttk.LabelFrame(self.setup_tab, text="Control Panel", padding=10)
        control_frame.pack(fill='x', padx=10, pady=10)

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

        # Control buttons - first row
        button_row1 = ttk.Frame(button_frame)
        button_row1.pack(pady=2)

        self.measure_btn = ttk.Button(button_row1, text="Start Measurement", command=self.start_measurement)
        self.measure_btn.pack(side='left', padx=5)

        self.abort_btn = ttk.Button(button_row1, text="Abort", command=self.abort_measurement, state='disabled')
        self.abort_btn.pack(side='left', padx=5)

        self.display_btn = ttk.Button(button_row1, text="Display Data", command=self.display_data, state='disabled')
        self.display_btn.pack(side='left', padx=5)

        # Analysis buttons - second row
        button_row2 = ttk.Frame(button_frame)
        button_row2.pack(pady=2)

        self.analyze_freq_btn = ttk.Button(button_row2, text="Analyse Frequency",
                                           command=self.analyze_frequency, state='disabled')
        self.analyze_freq_btn.pack(side='left', padx=5)

        self.analyze_move_btn = ttk.Button(button_row2, text="Analyse Movement",
                                           command=self.analyze_movement, state='disabled')
        self.analyze_move_btn.pack(side='left', padx=5)

        self.analyze_force_btn = ttk.Button(button_row2, text="Analyse Force",
                                            command=self.analyze_force, state='disabled')
        self.analyze_force_btn.pack(side='left', padx=5)

        # Third row for sensor comparisons
        button_row3 = ttk.Frame(button_frame)
        button_row3.pack(pady=2)

        self.compare_brady_btn = ttk.Button(button_row3, text="Compare Bradykinesia Sensors",
                                            command=self.compare_bradykinesia, state='disabled')
        self.compare_brady_btn.pack(side='left', padx=5)

        self.compare_tremor_btn = ttk.Button(button_row3, text="Compare Tremor Sensors",
                                             command=self.compare_tremor, state='disabled')
        self.compare_tremor_btn.pack(side='left', padx=5)

        # Status indicator
        status_frame = ttk.Frame(self.setup_tab)
        status_frame.pack(fill='x', padx=10, pady=5)

        ttk.Label(status_frame, text="Status:").pack(side='left', padx=5)
        self.status_var = tk.StringVar(value="Ready")
        status_label = ttk.Label(status_frame, textvariable=self.status_var, font=('Arial', 10, 'bold'))
        status_label.pack(side='left', padx=5)

        # Progress bar
        self.progress_var = tk.DoubleVar(value=0)
        self.progress_bar = ttk.Progressbar(self.setup_tab, variable=self.progress_var, maximum=100)
        self.progress_bar.pack(fill='x', padx=10, pady=5)

        # Create log area
        log_frame = ttk.LabelFrame(self.setup_tab, text="Activity Log", padding=10)
        log_frame.pack(fill='both', expand=True, padx=10, pady=10)

        self.log_text = create_logger(log_frame)

    def refresh_ports(self):
        """Update the available serial ports in the dropdown"""
        import serial.tools.list_ports
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
        self.analyze_freq_btn.configure(state='disabled')
        self.analyze_move_btn.configure(state='disabled')
        self.analyze_force_btn.configure(state='disabled')
        self.compare_brady_btn.configure(state='disabled')
        self.compare_tremor_btn.configure(state='disabled')

        # Reset progress
        self.progress_var.set(0)

        # Log the action
        self.log(f"Starting {self.measure_type.get()} measurement on port {self.port_var.get()}")

        # Start data collection in a separate thread
        self.collecting = True
        self.collection_thread = threading.Thread(
            target=self.data_collector.collect_data,
            args=(self.port_var.get(), self.measure_type.get(), self.timeout_var.get()),
            daemon=True
        )
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
            self.analyze_freq_btn.configure(state='normal')
            self.analyze_move_btn.configure(state='normal')
            self.analyze_force_btn.configure(state='normal')
            self.compare_brady_btn.configure(state='normal')
            self.compare_tremor_btn.configure(state='normal')
            self.status_var.set(f"Measurement aborted - {len(self.data)} points collected")
        else:
            self.status_var.set("Measurement aborted - no data collected")

    def measurement_complete(self):
        """Process data when measurement is complete"""
        self.measure_btn.configure(state='normal')
        self.abort_btn.configure(state='disabled')
        self.display_btn.configure(state='normal')
        self.analyze_freq_btn.configure(state='normal')
        self.analyze_move_btn.configure(state='normal')
        self.analyze_force_btn.configure(state='normal')
        self.compare_brady_btn.configure(state='normal')
        self.compare_tremor_btn.configure(state='normal')
        self.progress_var.set(100)

        # Store the mode in app.measurement_mode for future reference
        if self.data:
            self.measurement_mode = self.data[0][2]

        # Display the data
        self.display_data()

        # Switch to the Raw Data tab
        self.notebook.select(1)  # Index 1 is the Raw Data tab

    def display_data(self):
        """Display the raw data on the Raw Data tab"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        self.data_visualizer.plot_raw_data(self.raw_data_tab, filtered_data, self.measure_type.get())
        self.status_var.set(f"Displaying {len(filtered_data)} data points (filtered)")
        self.log(f"Data displayed on Raw Data tab - filtered first 0.5s")

    def analyze_frequency(self):
        """Analyze frequency content of the data"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        self.frequency_analyzer.analyse(self.analysis_tab, filtered_data, self.measure_type.get())
        self.notebook.select(2)  # Switch to Analysis tab

    def analyze_movement(self):
        """Analyze movement metrics"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        # Pass the analysis tab as the parent frame (even though MovementAnalyzer creates its own window)
        self.movement_analyzer.analyze(self.analysis_tab, filtered_data, self.measure_type.get())

    def analyze_force(self):
        """Analyze force data and convert to newtons"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        self.force_analyzer.analyze(filtered_data, self.measure_type.get())

    def compare_bradykinesia(self):
        """Compare analog sensor and IMU angle for bradykinesia measurements"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        self.bradykinesia_comparison.analyze(filtered_data, self.measure_type.get())

    def compare_tremor(self):
        """Compare analog sensor and accelerometer for tremor measurements using frequency analysis"""
        # Filter out first 0.5 seconds
        filtered_data = self.filter_initial_data(self.data)
        self.tremor_comparison.analyze(filtered_data, self.measure_type.get())

    def filter_initial_data(self, data):
        """Filter out the first 0.5 seconds of data and normalize time to 0-10 seconds"""
        if not data:
            return data

        filtered_data = []
        first_valid_time = None

        for item in data:
            time_ms = item[1]  # Time is at index 1
            if time_ms >= 500:  # Keep data after 500ms (0.5 seconds)
                if first_valid_time is None:
                    first_valid_time = time_ms

                # Create new item with normalized time (starting from 0)
                normalized_time = time_ms - first_valid_time
                new_item = [item[0], normalized_time] + item[2:]  # Keep index, normalize time, keep rest
                filtered_data.append(new_item)

        return filtered_data

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