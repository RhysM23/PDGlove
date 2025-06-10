import numpy as np
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from scipy.signal import butter, filtfilt, find_peaks
from scipy.stats import pearsonr

try:
    from scipy.integrate import cumulative_trapezoid
except ImportError:
    from scipy.integrate import cumtrapz as cumulative_trapezoid


class TremorComparison:
    def __init__(self, app):
        self.app = app
        self.canvas = None
        self.fig = None

    def analyze(self, data, measure_type):
        """Compare value1 (analog sensor) and value2 (accelerometer Y) for tremor measurements using frequency analysis"""
        if not data or len(data) < 50:
            self.app.show_error("Not enough data for tremor frequency comparison")
            return

        # Only proceed if this is a tremor measurement (mode 1)
        mode = data[0][2]  # Get mode from first data point
        if mode != 1:
            messagebox.showinfo("Info", "Tremor frequency comparison is only applicable for Tremor tests")
            return

        # Create the comparison analysis window
        comparison_window = tk.Toplevel(self.app.root)
        comparison_window.title("Tremor Frequency Comparison")
        comparison_window.geometry("1400x900")

        # Extract relevant data
        time_data = np.array([item[1] / 1000.0 for item in data])  # Convert to seconds
        value1_data = np.array([item[3] for item in data])  # Analog sensor (pin 4)
        value2_data = np.array([item[4] for item in data])  # Accelerometer Y-axis

        # Remove any NaN or invalid values
        valid_indices = ~(np.isnan(value1_data) | np.isnan(value2_data))
        time_clean = time_data[valid_indices]
        value1_clean = value1_data[valid_indices]
        value2_clean = value2_data[valid_indices]

        if len(value1_clean) < 50:
            messagebox.showerror("Error", "Not enough valid data points for frequency analysis")
            return

        # Calculate sampling rate
        if len(time_clean) > 1:
            dt = np.mean(np.diff(time_clean))
            fs = 1.0 / dt  # Sampling frequency in Hz
        else:
            fs = 100  # Default sampling rate

        # Apply bandpass filter (1-20 Hz) to both signals
        value1_filtered = self.apply_bandpass_filter(value1_clean, fs)
        value2_filtered = self.apply_bandpass_filter(value2_clean, fs)

        # Perform frequency analysis on both signals
        freq_results_analog = self.analyze_tremor_frequency(value1_clean, fs)
        freq_results_accel = self.analyze_tremor_frequency(value2_clean, fs)

        # Calculate displacements
        analog_displacement = self.calculate_displacement_from_analog(value1_filtered)
        accel_displacement_data = self.calculate_displacement_from_acceleration(value2_filtered, time_clean)
        accel_displacement = accel_displacement_data['displacement'] * 1000  # Convert to mm

        # Calculate amplitudes for display
        analog_amplitude = self.calculate_amplitude_from_analog(value1_filtered)
        accel_amplitude = self.calculate_amplitude_from_double_integration(value2_filtered, time_clean)

        # Create visualization with 6 plots (2 rows, 3 columns)
        self.fig = plt.Figure(figsize=(18, 10))

        # Top left: Raw analog sensor data
        ax1 = self.fig.add_subplot(231)
        ax1.plot(time_clean, value1_clean, 'b-', linewidth=1)
        ax1.set_title("Raw Data - Analog Sensor (Pin 4)")
        ax1.set_xlabel("Time (s)")
        ax1.set_ylabel("Analog Reading")
        ax1.grid(True, alpha=0.3)

        # Top middle: Raw vs filtered accelerometer data
        ax2 = self.fig.add_subplot(232)
        ax2.plot(time_clean, value2_clean, 'r-', linewidth=1, alpha=0.5, label='Raw')
        ax2.plot(time_clean, value2_filtered, 'k-', linewidth=1.5, label='Bandpass Filtered (1-20 Hz)')
        ax2.set_title("Accelerometer Y-axis - Raw vs Filtered")
        ax2.set_xlabel("Time (s)")
        ax2.set_ylabel("Acceleration (g)")
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        # Top right: Combined frequency spectrums
        ax3 = self.fig.add_subplot(233)
        freqs_analog = freq_results_analog['freqs']
        magnitude_analog = freq_results_analog['magnitude']
        freqs_accel = freq_results_accel['freqs']
        magnitude_accel = freq_results_accel['magnitude']

        ax3.plot(freqs_analog, magnitude_analog, 'b-', linewidth=1.5, label='Analog Sensor')
        ax3.plot(freqs_accel, magnitude_accel, 'r-', linewidth=1.5, label='Accelerometer')
        ax3.axvline(x=freq_results_analog['dominant_freq'], color='b', linestyle='--', alpha=0.7)
        ax3.axvline(x=freq_results_accel['dominant_freq'], color='r', linestyle='--', alpha=0.7)
        ax3.set_title("Combined Frequency Spectrums")
        ax3.set_xlabel("Frequency (Hz)")
        ax3.set_ylabel("Magnitude")
        ax3.set_xlim(0, 20)
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        # Bottom left: Frequency spectrum - Analog sensor
        ax4 = self.fig.add_subplot(234)
        ax4.plot(freqs_analog, magnitude_analog, 'b-', linewidth=1.5)
        ax4.axvline(x=freq_results_analog['dominant_freq'], color='b', linestyle='--', alpha=0.7)
        ax4.text(freq_results_analog['dominant_freq'], max(magnitude_analog) * 0.9,
                 f'   Peak: {freq_results_analog["dominant_freq"]:.2f} Hz',
                 color='b', fontsize=10, ha='left')
        ax4.set_title("Frequency Spectrum - Analog Sensor")
        ax4.set_xlabel("Frequency (Hz)")
        ax4.set_ylabel("Magnitude")
        ax4.set_xlim(0, 20)
        ax4.grid(True, alpha=0.3)

        # Bottom middle: Frequency spectrum - Accelerometer
        ax5 = self.fig.add_subplot(235)
        ax5.plot(freqs_accel, magnitude_accel, 'r-', linewidth=1.5)
        ax5.axvline(x=freq_results_accel['dominant_freq'], color='r', linestyle='--', alpha=0.7)
        ax5.text(freq_results_accel['dominant_freq'], max(magnitude_accel) * 0.9,
                 f'   Peak: {freq_results_accel["dominant_freq"]:.2f} Hz',
                 color='r', fontsize=10, ha='left')
        ax5.set_title("Frequency Spectrum - Accelerometer Y-axis")
        ax5.set_xlabel("Frequency (Hz)")
        ax5.set_ylabel("Magnitude")
        ax5.set_xlim(0, 20)
        ax5.grid(True, alpha=0.3)

        # Bottom right: Displacement comparison from both sensors
        ax6 = self.fig.add_subplot(236)

        # Ensure both signals have the same time base
        min_length = min(len(time_clean), len(analog_displacement))
        analog_displacement_trimmed = analog_displacement[:min_length]
        time_trimmed = time_clean[:min_length]

        # Initialize comparison statistics
        displacement_correlation = None
        displacement_avg_diff = None
        displacement_std_diff = None
        displacement_rms_diff = None
        analog_avg_peak_to_trough = None
        accel_avg_peak_to_trough = None

        # Get accelerometer displacement data
        if len(accel_displacement_data['time']) > 0:
            accel_displacement_plot = accel_displacement_data['displacement'] * 1000  # Convert to mm
            accel_time_plot = accel_displacement_data['time']

            # Trim accelerometer data to match the minimum length
            if len(accel_displacement_plot) > min_length:
                accel_displacement_plot = accel_displacement_plot[:min_length]
                accel_time_plot = accel_time_plot[:min_length]

            # ALIGNMENT: Make both signals start at the same point
            if len(analog_displacement_trimmed) > 0:
                # Align both to start at zero
                analog_displacement_aligned = analog_displacement_trimmed - analog_displacement_trimmed[0]
                accel_displacement_aligned = accel_displacement_plot - accel_displacement_plot[0]

                # Ensure both arrays are exactly the same length for comparison
                final_length = min(len(analog_displacement_aligned), len(accel_displacement_aligned))
                analog_displacement_final = analog_displacement_aligned[:final_length]
                accel_displacement_final = accel_displacement_aligned[:final_length]
                time_final = time_trimmed[:final_length]

                # Calculate correlation coefficient and difference statistics
                try:
                    displacement_correlation, p_value = pearsonr(analog_displacement_final, accel_displacement_final)

                    # Calculate difference statistics
                    displacement_diff = analog_displacement_final - accel_displacement_final
                    displacement_avg_diff = np.mean(displacement_diff)
                    displacement_std_diff = np.std(displacement_diff)
                    displacement_rms_diff = np.sqrt(np.mean(displacement_diff ** 2))

                    # Calculate peak-to-trough distances for both signals
                    analog_avg_peak_to_trough = self.calculate_peak_to_trough_distance(analog_displacement_final)
                    accel_avg_peak_to_trough = self.calculate_peak_to_trough_distance(accel_displacement_final)

                    # Log detailed statistics
                    self.app.log(f"Displacement comparison statistics:")
                    self.app.log(f"  Correlation coefficient: {displacement_correlation:.4f} (p={p_value:.6f})")
                    self.app.log(f"  Average difference: {displacement_avg_diff:.3f} mm")
                    self.app.log(f"  Std dev of difference: {displacement_std_diff:.3f} mm")
                    self.app.log(f"  RMS difference: {displacement_rms_diff:.3f} mm")
                    if analog_avg_peak_to_trough is not None:
                        self.app.log(f"  Analog avg peak-to-trough: {analog_avg_peak_to_trough:.3f} mm")
                    else:
                        self.app.log(f"  Analog avg peak-to-trough: Could not calculate")
                    if accel_avg_peak_to_trough is not None:
                        self.app.log(f"  Accel avg peak-to-trough: {accel_avg_peak_to_trough:.3f} mm")
                    else:
                        self.app.log(f"  Accel avg peak-to-trough: Could not calculate")

                except Exception as e:
                    self.app.log(f"Error calculating displacement statistics: {str(e)}")

                # Plot both aligned signals
                ax6.plot(accel_time_plot[:final_length], accel_displacement_final, 'r-', linewidth=1.5,
                         label='Accelerometer (Double Integration)')
                ax6.plot(time_final, analog_displacement_final, 'b-', linewidth=1.5,
                         label='Analog Sensor')

                # Add peak and trough markers if analysis was successful
                if analog_avg_peak_to_trough is not None:
                    self.mark_peaks_and_troughs(ax6, time_final, analog_displacement_final, 'blue', alpha=0.6)
                if accel_avg_peak_to_trough is not None:
                    self.mark_peaks_and_troughs(ax6, accel_time_plot[:final_length], accel_displacement_final, 'red',
                                                alpha=0.6)

                # Add statistics text to plot (expanded)
                if displacement_correlation is not None:
                    stats_text = f'r = {displacement_correlation:.3f}\nΔ = {displacement_avg_diff:.2f}±{displacement_std_diff:.2f} mm'
                    if analog_avg_peak_to_trough is not None:
                        stats_text += f'\nAnalog P-T: {analog_avg_peak_to_trough:.2f} mm'
                    if accel_avg_peak_to_trough is not None:
                        stats_text += f'\nAccel P-T: {accel_avg_peak_to_trough:.2f} mm'

                    ax6.text(0.02, 0.98, stats_text, transform=ax6.transAxes,
                             verticalalignment='top', fontsize=9,
                             bbox=dict(boxstyle='round', facecolor='white', alpha=0.9))

        # If only analog data available
        elif len(analog_displacement_trimmed) > 0:
            analog_displacement_aligned = analog_displacement_trimmed - analog_displacement_trimmed[0]
            ax6.plot(time_trimmed, analog_displacement_aligned, 'b-', linewidth=1.5, label='Analog Sensor')

            # Calculate peak-to-trough for analog only
            analog_avg_peak_to_trough = self.calculate_peak_to_trough_distance(analog_displacement_aligned)
            self.mark_peaks_and_troughs(ax6, time_trimmed, analog_displacement_aligned, 'blue', alpha=0.6)

            if analog_avg_peak_to_trough is not None:
                stats_text = f'Analog P-T: {analog_avg_peak_to_trough:.2f} mm'
                ax6.text(0.02, 0.98, stats_text, transform=ax6.transAxes,
                         verticalalignment='top', fontsize=9,
                         bbox=dict(boxstyle='round', facecolor='white', alpha=0.9))

        ax6.set_title("Displacement Comparison")
        ax6.set_xlabel("Time (s)")
        ax6.set_ylabel("Displacement (mm)")
        ax6.legend(loc='lower right')
        ax6.grid(True, alpha=0.3)

        self.fig.tight_layout()

        # Add figure to canvas
        self.canvas = FigureCanvasTkAgg(self.fig, master=comparison_window)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill='both', expand=True, padx=10, pady=10)

        # Create results summary frame
        results_frame = ttk.LabelFrame(comparison_window, text="Analysis Results", padding=10)
        results_frame.pack(fill='x', padx=10, pady=10)

        # Display results in a grid format
        ttk.Label(results_frame, text="Analog Sensor Dominant Frequency:").grid(row=0, column=0, sticky='w', padx=10,
                                                                                pady=5)
        ttk.Label(results_frame, text=f"{freq_results_analog['dominant_freq']:.2f} Hz",
                  font=('Arial', 10, 'bold')).grid(row=0, column=1, sticky='w', padx=10, pady=5)

        ttk.Label(results_frame, text="Accelerometer Dominant Frequency:").grid(row=1, column=0, sticky='w', padx=10,
                                                                                pady=5)
        ttk.Label(results_frame, text=f"{freq_results_accel['dominant_freq']:.2f} Hz",
                  font=('Arial', 10, 'bold')).grid(row=1, column=1, sticky='w', padx=10, pady=5)

        ttk.Label(results_frame, text="Analog Sensor Tremor Amplitude:").grid(row=0, column=2, sticky='w', padx=10,
                                                                              pady=5)
        ttk.Label(results_frame, text=f"{analog_amplitude:.2f} mm",
                  font=('Arial', 10, 'bold')).grid(row=0, column=3, sticky='w', padx=10, pady=5)

        ttk.Label(results_frame, text="Accelerometer Tremor Amplitude:").grid(row=1, column=2, sticky='w', padx=10,
                                                                              pady=5)
        ttk.Label(results_frame, text=f"{accel_amplitude:.2f} mm",
                  font=('Arial', 10, 'bold')).grid(row=1, column=3, sticky='w', padx=10, pady=5)

        # Tremor classification
        tremor_classification = self.classify_tremor(freq_results_accel['dominant_freq'])
        ttk.Label(results_frame, text="Tremor Classification:").grid(row=2, column=0, sticky='w', padx=10, pady=5)
        ttk.Label(results_frame, text=tremor_classification,
                  font=('Arial', 10, 'bold')).grid(row=2, column=1, columnspan=3, sticky='w', padx=10, pady=5)

        # ADD DISPLACEMENT COMPARISON SECTION
        if displacement_correlation is not None:
            # Add separator
            ttk.Separator(results_frame, orient='horizontal').grid(row=3, column=0, columnspan=4, sticky='ew', pady=10)

            # Displacement comparison results
            ttk.Label(results_frame, text="Displacement Correlation:").grid(row=4, column=0, sticky='w', padx=10,
                                                                            pady=5)

            # Color-coded correlation
            corr_color = 'green' if displacement_correlation > 0.7 else 'orange' if displacement_correlation > 0.4 else 'red'
            ttk.Label(results_frame, text=f"{displacement_correlation:.3f}",
                      font=('Arial', 10, 'bold'), foreground=corr_color).grid(row=4, column=1, sticky='w', padx=10,
                                                                              pady=5)

            ttk.Label(results_frame, text="Average Difference:").grid(row=5, column=0, sticky='w', padx=10, pady=5)
            ttk.Label(results_frame, text=f"{displacement_avg_diff:.3f} mm",
                      font=('Arial', 10, 'bold')).grid(row=5, column=1, sticky='w', padx=10, pady=5)

            ttk.Label(results_frame, text="RMS Difference:").grid(row=4, column=2, sticky='w', padx=10, pady=5)
            ttk.Label(results_frame, text=f"{displacement_rms_diff:.3f} mm",
                      font=('Arial', 10, 'bold')).grid(row=4, column=3, sticky='w', padx=10, pady=5)

            # Interpretation
            if displacement_correlation > 0.8:
                interpretation = "Excellent displacement agreement"
            elif displacement_correlation > 0.6:
                interpretation = "Good displacement agreement"
            elif displacement_correlation > 0.4:
                interpretation = "Moderate displacement agreement"
            else:
                interpretation = "Poor displacement agreement"

            ttk.Label(results_frame, text="Agreement:").grid(row=5, column=2, sticky='w', padx=10, pady=5)
            ttk.Label(results_frame, text=interpretation,
                      font=('Arial', 10, 'bold')).grid(row=5, column=3, sticky='w', padx=10, pady=5)

            # ADD PEAK-TO-TROUGH MEASUREMENTS
            if analog_avg_peak_to_trough is not None or accel_avg_peak_to_trough is not None:
                # Peak-to-trough section header
                ttk.Label(results_frame, text="Peak-to-Trough Analysis:",
                          font=('Arial', 10, 'bold')).grid(row=6, column=0, columnspan=4, sticky='w', padx=10,
                                                           pady=(15, 5))

                if analog_avg_peak_to_trough is not None:
                    ttk.Label(results_frame, text="Analog Sensor Avg P-T Distance:").grid(row=7, column=0, sticky='w',
                                                                                          padx=10, pady=5)
                    ttk.Label(results_frame, text=f"{analog_avg_peak_to_trough:.3f} mm",
                              font=('Arial', 10, 'bold')).grid(row=7, column=1, sticky='w', padx=10, pady=5)
                else:
                    ttk.Label(results_frame, text="Analog Sensor Avg P-T Distance:").grid(row=7, column=0, sticky='w',
                                                                                          padx=10, pady=5)
                    ttk.Label(results_frame, text="N/A",
                              font=('Arial', 10, 'bold')).grid(row=7, column=1, sticky='w', padx=10, pady=5)

                if accel_avg_peak_to_trough is not None:
                    ttk.Label(results_frame, text="Accelerometer Avg P-T Distance:").grid(row=7, column=2, sticky='w',
                                                                                          padx=10, pady=5)
                    ttk.Label(results_frame, text=f"{accel_avg_peak_to_trough:.3f} mm",
                              font=('Arial', 10, 'bold')).grid(row=7, column=3, sticky='w', padx=10, pady=5)
                else:
                    ttk.Label(results_frame, text="Accelerometer Avg P-T Distance:").grid(row=7, column=2, sticky='w',
                                                                                          padx=10, pady=5)
                    ttk.Label(results_frame, text="N/A",
                              font=('Arial', 10, 'bold')).grid(row=7, column=3, sticky='w', padx=10, pady=5)

                # Peak-to-trough comparison if both are available
                if analog_avg_peak_to_trough is not None and accel_avg_peak_to_trough is not None:
                    pt_difference = abs(analog_avg_peak_to_trough - accel_avg_peak_to_trough)
                    pt_ratio = max(analog_avg_peak_to_trough, accel_avg_peak_to_trough) / min(analog_avg_peak_to_trough,
                                                                                              accel_avg_peak_to_trough)

                    ttk.Label(results_frame, text="P-T Difference:").grid(row=8, column=0, sticky='w', padx=10, pady=5)
                    ttk.Label(results_frame, text=f"{pt_difference:.3f} mm",
                              font=('Arial', 10, 'bold')).grid(row=8, column=1, sticky='w', padx=10, pady=5)

                    ttk.Label(results_frame, text="P-T Ratio:").grid(row=8, column=2, sticky='w', padx=10, pady=5)
                    ttk.Label(results_frame, text=f"{pt_ratio:.2f}:1",
                              font=('Arial', 10, 'bold')).grid(row=8, column=3, sticky='w', padx=10, pady=5)
                else:
                    # Show N/A if either measurement failed
                    ttk.Label(results_frame, text="P-T Difference:").grid(row=8, column=0, sticky='w', padx=10, pady=5)
                    ttk.Label(results_frame, text="N/A",
                              font=('Arial', 10, 'bold')).grid(row=8, column=1, sticky='w', padx=10, pady=5)

                    ttk.Label(results_frame, text="P-T Ratio:").grid(row=8, column=2, sticky='w', padx=10, pady=5)
                    ttk.Label(results_frame, text="N/A",
                              font=('Arial', 10, 'bold')).grid(row=8, column=3, sticky='w', padx=10, pady=5)

        # Log the analysis
        self.app.log(f"Tremor comparison completed: Analog={freq_results_analog['dominant_freq']:.2f}Hz "
                     f"({analog_amplitude:.2f}mm), Accel={freq_results_accel['dominant_freq']:.2f}Hz "
                     f"({accel_amplitude:.2f}mm), Classification={tremor_classification}")

    def apply_bandpass_filter(self, signal, fs, low_freq=1.0, high_freq=20.0):
        """Apply bandpass filter to remove noise and focus on tremor frequencies"""
        try:
            # Remove DC component first
            signal_centered = signal - np.mean(signal)

            # Design bandpass filter
            nyquist = 0.5 * fs
            low = low_freq / nyquist
            high = min(high_freq / nyquist, 0.95)  # Ensure we don't exceed Nyquist

            if low < high:
                b, a = butter(4, [low, high], btype='band')
                filtered_signal = filtfilt(b, a, signal_centered)
                return filtered_signal
            else:
                return signal_centered

        except Exception as e:
            self.app.log(f"Bandpass filter error: {str(e)}")
            return signal - np.mean(signal)

    def analyze_tremor_frequency(self, signal, fs):
        """Perform frequency analysis on a tremor signal"""
        try:
            # Remove DC component
            signal_centered = signal - np.mean(signal)

            # Apply window to reduce spectral leakage
            windowed_signal = signal_centered * np.hanning(len(signal_centered))

            # Compute FFT
            fft_result = np.fft.rfft(windowed_signal)
            freqs = np.fft.rfftfreq(len(signal_centered), d=1.0 / fs)
            magnitude = np.abs(fft_result) * 2.0 / len(signal_centered)

            # Focus on tremor frequency range (1-20 Hz)
            tremor_mask = (freqs >= 1.0) & (freqs <= 20.0)
            tremor_freqs = freqs[tremor_mask]
            tremor_magnitude = magnitude[tremor_mask]

            # Find dominant frequency
            if len(tremor_magnitude) > 0:
                peak_idx = np.argmax(tremor_magnitude)
                dominant_freq = tremor_freqs[peak_idx]
            else:
                dominant_freq = 0

            return {
                'freqs': freqs,
                'magnitude': magnitude,
                'dominant_freq': dominant_freq
            }
        except Exception as e:
            self.app.log(f"Frequency analysis error: {str(e)}")
            return {'freqs': [], 'magnitude': [], 'dominant_freq': 0}

    def calculate_displacement_from_analog(self, analog_signal):
        """
        Calculate displacement from analog sensor (convert angular position to linear displacement)
        Returns displacement in mm over time
        """
        try:
            # Remove DC component to center around zero
            signal_centered = analog_signal - np.mean(analog_signal)

            # Convert analog reading to degrees
            # Assuming 0-4095 analog range corresponds to 0-360 degrees
            angle_degrees = (signal_centered / 4095.0) * 360.0

            # Convert to radians
            angle_radians = np.radians(angle_degrees)

            # Convert angular displacement to linear displacement
            # Assuming 8cm radius (typical finger length)
            radius_cm = 7.5
            linear_displacement_cm = radius_cm * angle_radians

            # Convert to mm
            displacement_mm = linear_displacement_cm * 10

            return displacement_mm

        except Exception as e:
            self.app.log(f"Analog displacement calculation error: {str(e)}")
            return np.zeros_like(analog_signal)

    def calculate_amplitude_from_analog(self, analog_signal):
        """
        Calculate tremor amplitude from analog sensor (angular position)
        Convert angular variation to linear displacement assuming 8cm radius
        """
        try:
            # Remove DC component
            signal_centered = analog_signal - np.mean(analog_signal)

            # Calculate peak-to-peak amplitude of the filtered signal
            peak_to_peak_analog = np.ptp(signal_centered)

            # Convert analog reading to degrees
            # Assuming 0-4095 analog range corresponds to 0-360 degrees
            peak_to_peak_degrees = (peak_to_peak_analog / 4095.0) * 360.0

            # Convert to radians
            peak_to_peak_radians = np.radians(peak_to_peak_degrees)

            # Convert angular displacement to linear displacement
            # Assuming 8cm radius (typical finger length)
            radius_cm = 7.5
            linear_displacement_cm = radius_cm * peak_to_peak_radians

            # Convert to mm and take half for amplitude (peak-to-peak to amplitude)
            amplitude_mm = (linear_displacement_cm * 10) / 2

            return amplitude_mm

        except Exception as e:
            self.app.log(f"Analog amplitude calculation error: {str(e)}")
            return 0

    def calculate_amplitude_from_double_integration(self, acceleration_signal, time_data):
        """
        Calculate tremor amplitude from acceleration using double integration
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

            # Calculate peak-to-peak amplitude and convert to mm
            amplitude_m = np.ptp(displacement) / 2  # Peak-to-peak to amplitude
            amplitude_mm = amplitude_m * 1000  # Convert to mm

            return amplitude_mm

        except Exception as e:
            self.app.log(f"Double integration amplitude calculation error: {str(e)}")
            return 0

    def calculate_displacement_from_acceleration(self, acceleration_signal, time_data):
        """
        Calculate displacement from acceleration using double integration
        Returns time and displacement arrays for plotting
        """
        try:
            # Remove DC component
            accel_centered = -acceleration_signal

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

    def calculate_amplitude_over_time(self, signal, time_data, window_seconds=2.0):
        """
        Calculate amplitude over time using a sliding window for analog sensor
        """
        try:
            # Calculate window size in samples
            dt = np.mean(np.diff(time_data))
            window_samples = int(window_seconds / dt)

            if window_samples < 10:
                window_samples = 10

            amplitude_time = []
            amplitude_values = []

            # Sliding window calculation
            for i in range(window_samples, len(signal), window_samples // 4):
                window_data = signal[i - window_samples:i]

                # Calculate amplitude from this window
                peak_to_peak = np.ptp(window_data)
                peak_to_peak_degrees = (peak_to_peak / 4095.0) * 360.0
                peak_to_peak_radians = np.radians(peak_to_peak_degrees)
                radius_cm = 8.0
                linear_displacement_cm = radius_cm * peak_to_peak_radians
                amplitude_mm = (linear_displacement_cm * 10) / 2

                amplitude_time.append(time_data[i])
                amplitude_values.append(amplitude_mm)

            return {
                'time': np.array(amplitude_time),
                'amplitude': np.array(amplitude_values)
            }

        except Exception as e:
            self.app.log(f"Amplitude over time calculation error: {str(e)}")
            return {'time': [], 'amplitude': []}

    def calculate_amplitude_over_time_from_integration(self, acceleration_signal, time_data, window_seconds=2.0):
        """
        Calculate amplitude over time using double integration in sliding windows
        """
        try:
            # Calculate window size in samples
            dt = np.mean(np.diff(time_data))
            window_samples = int(window_seconds / dt)

            if window_samples < 20:
                window_samples = 20

            amplitude_time = []
            amplitude_values = []

            # Sliding window calculation
            for i in range(window_samples, len(acceleration_signal), window_samples // 4):
                window_accel = acceleration_signal[i - window_samples:i]
                window_time = time_data[i - window_samples:i] - time_data[i - window_samples]

                # Calculate amplitude from this window using double integration
                accel_centered = window_accel - np.mean(window_accel)
                accel_ms2 = accel_centered * 9.81

                velocity = cumulative_trapezoid(accel_ms2, window_time, initial=0)
                velocity = velocity - np.mean(velocity)

                displacement = cumulative_trapezoid(velocity, window_time, initial=0)
                displacement = displacement - np.mean(displacement)

                amplitude_m = np.ptp(displacement) / 2
                amplitude_mm = amplitude_m * 1000

                amplitude_time.append(time_data[i])
                amplitude_values.append(amplitude_mm)

            return {
                'time': np.array(amplitude_time),
                'amplitude': np.array(amplitude_values)
            }

        except Exception as e:
            self.app.log(f"Integration amplitude over time calculation error: {str(e)}")
            return {'time': [], 'amplitude': []}

    def calculate_peak_to_trough_distance(self, signal, min_prominence=None):
        """
        Calculate the average peak-to-trough distance for a displacement signal

        Parameters:
        signal: displacement signal array
        min_prominence: minimum prominence for peak detection (auto-calculated if None)

        Returns:
        average peak-to-trough distance in mm, or None if insufficient peaks/troughs
        """
        try:
            # Auto-calculate prominence if not provided
            if min_prominence is None:
                signal_range = np.ptp(signal)
                min_prominence = signal_range * 0.1  # 10% of signal range

            # Minimum distance between peaks (prevent detecting noise)
            min_distance = max(10, len(signal) // 20)  # At least 10 samples or 5% of signal length

            # Find peaks
            peaks, peak_props = find_peaks(signal, prominence=min_prominence, distance=min_distance)

            # Find troughs (peaks of inverted signal)
            troughs, trough_props = find_peaks(-signal, prominence=min_prominence, distance=min_distance)

            self.app.log(f"Peak-to-trough analysis: Found {len(peaks)} peaks and {len(troughs)} troughs")

            if len(peaks) == 0 and len(troughs) == 0:
                self.app.log("No significant peaks or troughs detected")
                return None

            # Combine peaks and troughs, sort by time
            all_extrema = []
            for peak_idx in peaks:
                all_extrema.append((peak_idx, signal[peak_idx], 'peak'))
            for trough_idx in troughs:
                all_extrema.append((trough_idx, signal[trough_idx], 'trough'))

            # Sort by index (time)
            all_extrema.sort(key=lambda x: x[0])

            if len(all_extrema) < 2:
                self.app.log("Insufficient extrema for peak-to-trough calculation")
                return None

            # Calculate distances between consecutive extrema
            peak_to_trough_distances = []
            for i in range(len(all_extrema) - 1):
                current_value = all_extrema[i][1]
                next_value = all_extrema[i + 1][1]
                distance = abs(current_value - next_value)
                peak_to_trough_distances.append(distance)

            if len(peak_to_trough_distances) == 0:
                return None

            # Calculate average
            avg_distance = np.mean(peak_to_trough_distances)

            self.app.log(f"Peak-to-trough distances: {[f'{d:.2f}' for d in peak_to_trough_distances]} mm")
            self.app.log(f"Average peak-to-trough distance: {avg_distance:.3f} mm")

            return avg_distance

        except Exception as e:
            self.app.log(f"Error calculating peak-to-trough distance: {str(e)}")
            return None

    def mark_peaks_and_troughs(self, ax, time_data, signal, color, alpha=0.7):
        """
        Mark peaks and troughs on a displacement plot

        Parameters:
        ax: matplotlib axis object
        time_data: time array
        signal: displacement signal array
        color: color for the markers
        alpha: transparency of markers
        """
        try:
            # Calculate prominence for peak detection
            signal_range = np.ptp(signal)
            min_prominence = signal_range * 0.1  # 10% of signal range
            min_distance = max(10, len(signal) // 20)

            # Find peaks
            peaks, _ = find_peaks(signal, prominence=min_prominence, distance=min_distance)

            # Find troughs
            troughs, _ = find_peaks(-signal, prominence=min_prominence, distance=min_distance)

            # Mark peaks with upward triangles
            if len(peaks) > 0:
                ax.scatter(time_data[peaks], signal[peaks],
                           marker='^', s=40, color=color, alpha=alpha,
                           edgecolors='white', linewidth=0.5, zorder=5)

            # Mark troughs with downward triangles
            if len(troughs) > 0:
                ax.scatter(time_data[troughs], signal[troughs],
                           marker='v', s=40, color=color, alpha=alpha,
                           edgecolors='white', linewidth=0.5, zorder=5)

        except Exception as e:
            self.app.log(f"Error marking peaks and troughs: {str(e)}")

    def classify_tremor(self, dominant_freq):
        """Classify tremor type based on dominant frequency"""
        if dominant_freq == 0:
            return "No tremor detected"
        elif 3.0 <= dominant_freq <= 7.0:
            return "Parkinsonian tremor"
        elif 4.0 <= dominant_freq <= 12.0:
            return "Essential tremor"
        elif dominant_freq > 12.0:
            return "Physiological tremor"
        else:
            return "Atypical frequency"