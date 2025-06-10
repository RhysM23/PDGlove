import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk
from tkinter import ttk

try:
    from scipy.integrate import cumulative_trapezoid
except ImportError:
    from scipy.integrate import cumtrapz as cumulative_trapezoid


class FrequencyAnalyzer:
    def __init__(self, app):
        self.app = app
        self.canvas = None
        self.fig = None
        self.sensor_var = None
        self.filter_var = None

    def analyse(self, parent_frame, data, measure_type):
        """Perform frequency analysis on the data"""
        if not data or len(data) < 10:
            self.app.show_error("Not enough data for frequency analysis")
            return

        # Clear parent frame
        for widget in parent_frame.winfo_children():
            widget.destroy()

        # Create control panel
        control_frame = ttk.Frame(parent_frame)
        control_frame.pack(fill='x', padx=10, pady=5)

        # Sensor selection
        ttk.Label(control_frame, text="Analyse Sensor:").pack(side='left', padx=5)
        self.sensor_var = tk.StringVar(value="Sensor 1")
        sensor_combo = ttk.Combobox(control_frame, textvariable=self.sensor_var,
                                    values=["Sensor 1", "Sensor 2", "Sensor 3"], width=10)
        sensor_combo.pack(side='left', padx=5)

        # Filter option
        self.filter_var = tk.BooleanVar(value=True)
        filter_check = ttk.Checkbutton(control_frame, text="Apply Bandpass Filter (1-20 Hz)",
                                       variable=self.filter_var)
        filter_check.pack(side='left', padx=20)

        # Update button
        update_btn = ttk.Button(control_frame, text="Update Analysis",
                                command=lambda: self.update_analysis(data, measure_type))
        update_btn.pack(side='left', padx=5)

        # Results frame
        results_frame = ttk.LabelFrame(parent_frame, text="Analysis Results", padding=5)
        results_frame.pack(fill='x', padx=10, pady=5)

        # Create results variables
        self.dominant_freq_var = tk.StringVar(value="-")
        self.interpretation_var = tk.StringVar(value="-")
        self.displacement_amplitude_var = tk.StringVar(value="-")

        # Results display
        ttk.Label(results_frame, text="Dominant Frequency:").grid(row=0, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(results_frame, textvariable=self.dominant_freq_var, font=('Arial', 10, 'bold')).grid(
            row=0, column=1, sticky='w', padx=5, pady=2)
        ttk.Label(results_frame, text="Hz").grid(row=0, column=2, sticky='w', padx=5, pady=2)

        ttk.Label(results_frame, text="Displacement Amplitude:").grid(row=0, column=3, sticky='w', padx=15, pady=2)
        ttk.Label(results_frame, textvariable=self.displacement_amplitude_var, font=('Arial', 10, 'bold')).grid(
            row=0, column=4, sticky='w', padx=5, pady=2)

        ttk.Label(results_frame, text="Interpretation:").grid(row=1, column=0, sticky='w', padx=5, pady=2)
        ttk.Label(results_frame, textvariable=self.interpretation_var, font=('Arial', 10, 'bold')).grid(
            row=1, column=1, columnspan=4, sticky='w', padx=5, pady=2)

        # Figure frame
        figure_frame = ttk.Frame(parent_frame)
        figure_frame.pack(fill='both', expand=True, padx=10, pady=10)

        # Create figure
        self.fig = plt.Figure(figsize=(9, 8))

        # Create subplots (will be populated in update_analysis)
        self.time_plot = self.fig.add_subplot(311)
        self.freq_plot = self.fig.add_subplot(312)
        self.displacement_plot = self.fig.add_subplot(313)

        self.fig.tight_layout()

        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, master=figure_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill='both', expand=True)

        # Initial analysis
        self.update_analysis(data, measure_type)

    def update_analysis(self, data, measure_type):
        """Update the frequency analysis based on current settings"""
        # Extract time data
        time_data = np.array([item[1] / 1000.0 for item in data])  # Convert to seconds
        mode = data[0][2]  # Get the mode from the data

        # Select sensor data
        if self.sensor_var.get() == "Sensor 1":
            sensor_data = np.array([item[3] for item in data])  # value1
            sensor_name = "Sensor 1"
        elif self.sensor_var.get() == "Sensor 2":
            sensor_data = np.array([item[4] for item in data])  # value2
            sensor_name = "Sensor 2"
        else:
            sensor_data = np.array([item[5] for item in data])  # value3
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

        # Clear plots
        self.time_plot.clear()
        self.freq_plot.clear()
        self.displacement_plot.clear()

        # Time domain plot
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

                # Interpret based on measurement type
                if measure_type == "Tremor":
                    if 3.0 <= dominant_freq <= 7.0:
                        interpretation = "Consistent with Parkinsonian tremor (3-7 Hz)"
                    elif 7.0 < dominant_freq <= 12.0:
                        interpretation = "Consistent with essential/physiological tremor (7-12 Hz)"
                    else:
                        interpretation = "Outside typical tremor ranges"
                else:
                    interpretation = f"Dominant frequency: {dominant_freq:.2f} Hz"

                self.interpretation_var.set(interpretation)
                self.app.log(f"Dominant frequency: {dominant_freq:.2f} Hz - {interpretation}")
            else:
                self.dominant_freq_var.set("N/A")
                self.interpretation_var.set("No significant frequency components detected")
        else:
            self.dominant_freq_var.set("N/A")
            self.interpretation_var.set("No data in relevant frequency range")

        # Displacement analysis (only for accelerometer data)
        displacement_amplitude = self.calculate_displacement_analysis(filtered_data, time_data, mode)

        if displacement_amplitude is not None:
            self.displacement_amplitude_var.set(f"{displacement_amplitude:.2f} mm")
        else:
            self.displacement_amplitude_var.set("N/A")

        # Update the layout and redraw
        self.fig.tight_layout()
        self.canvas.draw()

    def calculate_displacement_analysis(self, acceleration_data, time_data, mode):
        """Calculate displacement from acceleration and create displacement plot"""
        try:
            # Only calculate displacement for tremor mode (mode 1) and accelerometer sensors
            if mode != 1:
                self.displacement_plot.text(0.5, 0.5, 'Displacement analysis only available for Tremor measurements',
                                            transform=self.displacement_plot.transAxes, ha='center', va='center',
                                            fontsize=12, style='italic')
                self.displacement_plot.set_title("Displacement Analysis (Tremor mode only)")
                return None

            # Check if this is an accelerometer sensor (for tremor mode, sensors 1-3 are accelerometer X, Y, Z)
            sensor_name = self.sensor_var.get()
            if sensor_name not in ["Sensor 1", "Sensor 2", "Sensor 3"]:
                self.displacement_plot.text(0.5, 0.5, 'Displacement analysis only available for accelerometer sensors',
                                            transform=self.displacement_plot.transAxes, ha='center', va='center',
                                            fontsize=12, style='italic')
                self.displacement_plot.set_title("Displacement Analysis (Accelerometer only)")
                return None

            # Calculate displacement from acceleration using double integration
            displacement_data = self.calculate_displacement_from_acceleration(acceleration_data, time_data)

            if len(displacement_data['displacement']) == 0:
                self.displacement_plot.text(0.5, 0.5, 'Unable to calculate displacement',
                                            transform=self.displacement_plot.transAxes, ha='center', va='center',
                                            fontsize=12, style='italic')
                self.displacement_plot.set_title("Displacement Analysis - Error")
                return None

            # Plot displacement
            displacement_mm = displacement_data['displacement'] * 1000  # Convert to mm
            self.displacement_plot.plot(displacement_data['time'], displacement_mm, 'g-', linewidth=1.5)
            self.displacement_plot.set_title(f"Displacement from {sensor_name} (Double Integration)")
            self.displacement_plot.set_xlabel("Time (s)")
            self.displacement_plot.set_ylabel("Displacement (mm)")
            self.displacement_plot.grid(True)

            # Calculate and display amplitude
            amplitude_mm = np.ptp(displacement_mm) / 2  # Peak-to-peak to amplitude

            # Mark peaks and troughs for visual reference
            self.mark_displacement_extrema(displacement_data['time'], displacement_mm)

            # Add amplitude text to plot
            self.displacement_plot.text(0.02, 0.98, f'Amplitude: {amplitude_mm:.2f} mm',
                                        transform=self.displacement_plot.transAxes,
                                        verticalalignment='top', fontsize=10,
                                        bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))

            self.app.log(f"Displacement analysis: Amplitude = {amplitude_mm:.2f} mm from {sensor_name}")

            return amplitude_mm

        except Exception as e:
            self.app.log(f"Displacement analysis error: {str(e)}")
            self.displacement_plot.text(0.5, 0.5, f'Displacement calculation failed:\n{str(e)}',
                                        transform=self.displacement_plot.transAxes, ha='center', va='center',
                                        fontsize=10, style='italic')
            self.displacement_plot.set_title("Displacement Analysis - Error")
            return None

    def calculate_displacement_from_acceleration(self, acceleration_signal, time_data):
        """
        Calculate displacement from acceleration using double integration
        Returns time and displacement arrays for plotting
        """
        try:
            # Remove DC component
            accel_centered = acceleration_signal - np.mean(acceleration_signal)

            # Convert acceleration from g to m/s² (1g = 9.81 m/s²)
            accel_ms2 = accel_centered * 9.81

            # First integration: acceleration -> velocity
            velocity = cumulative_trapezoid(accel_ms2, time_data, initial=0)

            # Remove any DC drift in velocity
            velocity = velocity - np.mean(velocity)

            # Second integration: velocity -> displacement
            displacement = cumulative_trapezoid(velocity, time_data, initial=0)

            # Remove any DC drift in displacement
            displacement = displacement - np.mean(displacement)

            return {
                'time': time_data,
                'displacement': displacement  # in meters
            }

        except Exception as e:
            self.app.log(f"Displacement calculation error: {str(e)}")
            return {'time': [], 'displacement': []}

    def mark_displacement_extrema(self, time_data, displacement_mm):
        """Mark peaks and troughs on the displacement plot for visual reference"""
        try:
            from scipy.signal import find_peaks

            # Calculate minimum prominence for peak detection
            signal_range = np.ptp(displacement_mm)
            min_prominence = signal_range * 0.1  # 10% of signal range
            min_distance = max(10, len(displacement_mm) // 20)  # Minimum distance between peaks

            # Find peaks
            peaks, _ = find_peaks(displacement_mm, prominence=min_prominence, distance=min_distance)

            # Find troughs (peaks of inverted signal)
            troughs, _ = find_peaks(-displacement_mm, prominence=min_prominence, distance=min_distance)

            # Mark peaks with upward triangles
            if len(peaks) > 0:
                self.displacement_plot.scatter(time_data[peaks], displacement_mm[peaks],
                                               marker='^', s=30, color='red', alpha=0.7,
                                               edgecolors='white', linewidth=0.5, zorder=5)

            # Mark troughs with downward triangles
            if len(troughs) > 0:
                self.displacement_plot.scatter(time_data[troughs], displacement_mm[troughs],
                                               marker='v', s=30, color='blue', alpha=0.7,
                                               edgecolors='white', linewidth=0.5, zorder=5)

        except Exception as e:
            # Don't fail the entire analysis if peak marking fails
            self.app.log(f"Peak marking error: {str(e)}")
            pass