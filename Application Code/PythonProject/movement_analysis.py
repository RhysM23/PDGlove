import tkinter as tk
from tkinter import ttk, messagebox
import numpy as np
from scipy.signal import find_peaks, savgol_filter
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


class MovementAnalyzer:
    def __init__(self, app):
        self.app = app

    def analyze(self, parent_frame, data, measure_type):
        """Analyze movement metrics (count and range)"""
        if not data or len(data) < 10:
            messagebox.showinfo("Info", "Not enough data to analyze movements")
            return

        # Log data details to help debug
        self.app.log(f"Movement analysis - specifically using Sensor 2 data")

        try:
            # Extract time data
            time_data = np.array([item[1] / 1000.0 for item in data])  # Convert to seconds

            # Specifically extract sensor 2 data (as confirmed by user)
            # New format: value2 is at index 4
            # Old format: sensor2 is at index 3
            angle_data = []

            # Check if we're using new or old format
            if len(data[0]) >= 6:  # New format with 6+ elements
                self.app.log("Using new data format (value2 at index 4)")
                sensor_index = 4  # value2
            else:
                self.app.log("Using old data format (sensor2 at index 3)")
                sensor_index = 3  # sensor2

            # Extract sensor 2 data, handling NaN values
            for item in data:
                try:
                    value = float(item[sensor_index])
                    if not np.isnan(value):
                        angle_data.append(value)
                    else:
                        # Skip NaN values instead of replacing with 0
                        # This preserves the data's true range
                        angle_data.append(None)
                except (IndexError, ValueError, TypeError):
                    angle_data.append(None)

            # Convert to numpy array, but keep None values as NaN
            angle_data = np.array([x if x is not None else np.nan for x in angle_data])

            # Now get a clean version of the data with NaN values removed
            clean_indices = ~np.isnan(angle_data)
            clean_time = time_data[clean_indices]
            clean_angles = angle_data[clean_indices]

            # Check if we have enough valid data points
            if len(clean_angles) < 10:
                messagebox.showinfo("Error",
                                    f"Not enough valid angle data points for analysis: only {len(clean_angles)} valid points")
                return

            # Log information about the data
            self.app.log(f"Total data points: {len(angle_data)}")
            self.app.log(f"Valid data points: {len(clean_angles)}")
            self.app.log(f"Angle range: {np.min(clean_angles):.2f} to {np.max(clean_angles):.2f}")

            # Clear parent frame
            for widget in parent_frame.winfo_children():
                widget.destroy()

            # Set proper parameter ranges based on data values
            data_range = np.max(clean_angles) - np.min(clean_angles)

            # Set default parameters based on data range
            # For degrees, use 10% of range for height and 5% for prominence
            if np.max(abs(clean_angles)) < 500:  # Likely degrees or normalized values
                default_height = max(5, data_range * 0.1)  # 10% of range
                default_prominence = max(3, data_range * 0.05)  # 5% of range
                unit_label = "degrees"
            else:  # Old 0-4095 range
                default_height = max(100, data_range * 0.1)
                default_prominence = max(50, data_range * 0.05)
                unit_label = "units"

            # Use default parameters for analysis
            smooth_window = 21
            peak_height = default_height
            peak_distance = 30
            peak_prominence = default_prominence

            # Run analysis with default parameters
            self.update_movement_analysis(
                parent_frame, clean_time, clean_angles, measure_type,
                smooth_window, peak_height, peak_distance, peak_prominence,
                unit_label)

        except Exception as e:
            self.app.log(f"Movement analysis error: {str(e)}")
            import traceback
            self.app.log(f"Traceback: {traceback.format_exc()}")
            messagebox.showerror("Error", f"Movement analysis failed: {str(e)}")

    def update_movement_analysis(self, parent_frame, time_data, angle_data, measure_type,
                                 smooth_window, peak_height, peak_distance, peak_prominence,
                                 unit_label="degrees"):
        """Update the movement analysis with new parameters"""
        try:
            # Log parameters
            self.app.log(f"Analysis parameters: smooth={smooth_window}, height={peak_height}, "
                         f"distance={peak_distance}, prominence={peak_prominence}")

            # Remove DC offset
            angle_data_centered = angle_data - np.mean(angle_data)

            # Apply Savitzky-Golay filter to smooth the data
            # Ensure window_length is odd
            if smooth_window % 2 == 0:
                smooth_window += 1

            # Ensure window is valid for the data length
            smooth_window = min(smooth_window, len(angle_data) - 3)
            if smooth_window < 5:
                smooth_window = 5
            if smooth_window % 2 == 0:
                smooth_window -= 1

            # Apply the filter
            angle_smooth = savgol_filter(angle_data_centered, window_length=smooth_window, polyorder=3)

            # Calculate the peak detection parameters
            data_range = np.max(angle_smooth) - np.min(angle_smooth)
            height_threshold = peak_height

            # Find peaks (movements in one direction)
            peaks, peak_props = find_peaks(
                angle_smooth,
                height=height_threshold,
                distance=int(peak_distance),  # Use int for distance
                prominence=peak_prominence
            )

            # Find troughs (movements in opposite direction)
            troughs, trough_props = find_peaks(
                -angle_smooth,
                height=height_threshold,
                distance=int(peak_distance),  # Use int for distance
                prominence=peak_prominence
            )

            # Log detection results
            self.app.log(f"Peaks found: {len(peaks)}, Troughs found: {len(troughs)}")

            # Combine and sort all extrema points
            if len(peaks) > 0 or len(troughs) > 0:
                # Convert to lists before concatenating
                peaks_list = peaks.tolist() if len(peaks) > 0 else []
                troughs_list = troughs.tolist() if len(troughs) > 0 else []
                all_extrema_list = sorted(peaks_list + troughs_list)
                all_extrema = np.array(all_extrema_list)
            else:
                all_extrema = np.array([])

            # Calculate movement count (a movement is considered a transition between extrema)
            if len(all_extrema) >= 2:
                movement_count = len(all_extrema) - 1
            else:
                movement_count = 0

            # Calculate ranges between consecutive extrema
            ranges = []
            for i in range(len(all_extrema) - 1):
                idx1 = all_extrema[i]
                idx2 = all_extrema[i + 1]
                range_value = abs(angle_smooth[idx1] - angle_smooth[idx2])
                ranges.append(range_value)

            # Calculate average range
            if ranges:
                avg_range = np.mean(ranges)
            else:
                avg_range = 0

            # Calculate frequency (movements per second)
            if movement_count > 0 and len(time_data) > 1:
                duration = time_data[-1] - time_data[0]
                movement_frequency = movement_count / duration if duration > 0 else 0
            else:
                movement_frequency = 0

            # Results frame
            results_frame = ttk.LabelFrame(parent_frame, text="Movement Metrics", padding=10)
            results_frame.pack(fill='x', padx=10, pady=10)

            # Display results
            ttk.Label(results_frame, text="Movement Count:").grid(row=0, column=0, sticky='w', padx=5, pady=5)
            ttk.Label(results_frame, text=f"{movement_count}", font=('Arial', 10, 'bold')).grid(
                row=0, column=1, sticky='w', padx=5, pady=5)

            ttk.Label(results_frame, text="Average Range of Motion:").grid(row=1, column=0, sticky='w', padx=5, pady=5)
            ttk.Label(results_frame, text=f"{avg_range:.2f} {unit_label}", font=('Arial', 10, 'bold')).grid(
                row=1, column=1, sticky='w', padx=5, pady=5)

            ttk.Label(results_frame, text="Movement Frequency:").grid(row=2, column=0, sticky='w', padx=5, pady=5)
            ttk.Label(results_frame, text=f"{movement_frequency:.2f} Hz", font=('Arial', 10, 'bold')).grid(
                row=2, column=1, sticky='w', padx=5, pady=5)

            # Raw Data Stats
            ttk.Label(results_frame, text=f"Data Range ({unit_label}):").grid(row=0, column=2, sticky='w', padx=5,
                                                                              pady=5)
            ttk.Label(results_frame, text=f"{data_range:.2f}", font=('Arial', 10, 'bold')).grid(
                row=0, column=3, sticky='w', padx=5, pady=5)

            ttk.Label(results_frame, text="Peaks Found:").grid(row=1, column=2, sticky='w', padx=5, pady=5)
            ttk.Label(results_frame, text=f"{len(peaks)}", font=('Arial', 10, 'bold')).grid(
                row=1, column=3, sticky='w', padx=5, pady=5)

            ttk.Label(results_frame, text="Troughs Found:").grid(row=2, column=2, sticky='w', padx=5, pady=5)
            ttk.Label(results_frame, text=f"{len(troughs)}", font=('Arial', 10, 'bold')).grid(
                row=2, column=3, sticky='w', padx=5, pady=5)

            # Calculate period changes between peaks
            peak_periods = []
            peak_times_for_period = []
            if len(peaks) > 1:
                for i in range(len(peaks) - 1):
                    period = time_data[peaks[i + 1]] - time_data[peaks[i]]
                    peak_periods.append(period)
                    # Use the middle time point between the two peaks
                    peak_times_for_period.append((time_data[peaks[i]] + time_data[peaks[i + 1]]) / 2)

            # Calculate amplitude changes over time
            amplitude_values = []
            amplitude_times = []
            if len(all_extrema) > 1:
                for i in range(len(all_extrema) - 1):
                    idx1 = int(all_extrema[i])
                    idx2 = int(all_extrema[i + 1])
                    amplitude = abs(angle_smooth[idx1] - angle_smooth[idx2])
                    amplitude_values.append(amplitude)
                    # Use the middle time point between the two extrema
                    amplitude_times.append((time_data[idx1] + time_data[idx2]) / 2)

            # Create visualization with 4 plots (2x2 grid)
            fig = plt.Figure(figsize=(12, 10))

            # Top left: Raw and filtered data
            ax1 = fig.add_subplot(221)
            ax1.plot(time_data, angle_data, 'k-', alpha=0.3, label='Raw Data')
            ax1.plot(time_data, angle_smooth, 'b-', label='Filtered Data')
            ax1.set_title(f"Angle Data ({unit_label})")
            ax1.set_xlabel("Time (s)")
            ax1.set_ylabel(f"Angle ({unit_label})")
            ax1.legend()
            ax1.grid(True)

            # Top right: Movement detection
            ax2 = fig.add_subplot(222)
            ax2.plot(time_data, angle_smooth, 'b-', label='Filtered Data')
            if len(peaks) > 0:
                ax2.plot(time_data[peaks], angle_smooth[peaks], 'ro', label='Peaks')
            if len(troughs) > 0:
                ax2.plot(time_data[troughs], angle_smooth[troughs], 'go', label='Troughs')

            # Add lines connecting the extrema
            for i in range(len(all_extrema) - 1):
                idx1 = int(all_extrema[i])
                idx2 = int(all_extrema[i + 1])
                ax2.plot([time_data[idx1], time_data[idx2]],
                         [angle_smooth[idx1], angle_smooth[idx2]], 'r--', alpha=0.5)

            ax2.set_title("Movement Detection")
            ax2.set_xlabel("Time (s)")
            ax2.set_ylabel(f"Angle ({unit_label})")
            ax2.legend()
            ax2.grid(True)

            # Bottom left: Period between peaks over time
            ax3 = fig.add_subplot(223)
            if peak_periods:
                ax3.plot(peak_times_for_period, peak_periods, 'ro-', linewidth=2, markersize=6)
                ax3.set_title("Period Between Peaks Over Time")
                ax3.set_xlabel("Time (s)")
                ax3.set_ylabel("Period (s)")
                ax3.grid(True)

                # Add average period line
                avg_period = np.mean(peak_periods)
                ax3.axhline(y=avg_period, color='g', linestyle='--', alpha=0.7,
                            label=f'Avg: {avg_period:.3f}s')
                ax3.legend()
            else:
                ax3.text(0.5, 0.5, 'Not enough peaks for period analysis',
                         transform=ax3.transAxes, ha='center', va='center', fontsize=12)
                ax3.set_title("Period Between Peaks Over Time")

            # Bottom right: Amplitude changes over time
            ax4 = fig.add_subplot(224)
            if amplitude_values:
                ax4.plot(amplitude_times, amplitude_values, 'mo-', linewidth=2, markersize=6)
                ax4.set_title("Amplitude Changes Over Time")
                ax4.set_xlabel("Time (s)")
                ax4.set_ylabel(f"Amplitude ({unit_label})")
                ax4.grid(True)

                # Add average amplitude line
                avg_amplitude = np.mean(amplitude_values)
                ax4.axhline(y=avg_amplitude, color='g', linestyle='--', alpha=0.7,
                            label=f'Avg: {avg_amplitude:.2f}{unit_label}')
                ax4.legend()
            else:
                ax4.text(0.5, 0.5, 'Not enough data for amplitude analysis',
                         transform=ax4.transAxes, ha='center', va='center', fontsize=12)
                ax4.set_title("Amplitude Changes Over Time")

            fig.tight_layout()

            # Add to parent frame
            figure_frame = ttk.Frame(parent_frame)
            figure_frame.pack(fill='both', expand=True, padx=10, pady=10)

            canvas = FigureCanvasTkAgg(fig, master=figure_frame)
            canvas.draw()
            canvas.get_tk_widget().pack(fill='both', expand=True)

            # Log the analysis
            self.app.log(f"Movement analysis updated: {movement_count} movements, "
                         f"{avg_range:.2f} {unit_label} average range, {movement_frequency:.2f} Hz")

            # Log period and amplitude statistics
            if peak_periods:
                avg_period = np.mean(peak_periods)
                std_period = np.std(peak_periods)
                self.app.log(f"Period analysis: Average={avg_period:.3f}s, Std={std_period:.3f}s")

            if amplitude_values:
                avg_amplitude = np.mean(amplitude_values)
                std_amplitude = np.std(amplitude_values)
                self.app.log(
                    f"Amplitude analysis: Average={avg_amplitude:.2f}{unit_label}, Std={std_amplitude:.2f}{unit_label}")

        except Exception as e:
            self.app.log(f"Movement analysis update error: {str(e)}")
            import traceback
            self.app.log(f"Traceback: {traceback.format_exc()}")
            messagebox.showerror("Error", f"Movement analysis update failed: {str(e)}")