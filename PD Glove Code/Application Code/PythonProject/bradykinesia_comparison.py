import numpy as np
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from scipy.signal import savgol_filter
from scipy.stats import pearsonr


class BradykinesiaComparison:
    def __init__(self, app):
        self.app = app
        self.canvas = None
        self.fig = None

    def analyze(self, data, measure_type):
        """Compare analog angle sensor (value1) and IMU angle (value2) for bradykinesia measurements"""
        if not data or len(data) < 50:
            self.app.show_error("Not enough data for bradykinesia angle comparison")
            return

        # Only proceed if this is a bradykinesia measurement (mode 2)
        mode = data[0][2]  # Get mode from first data point
        if mode != 2:
            messagebox.showinfo("Info", "Bradykinesia angle comparison is only applicable for Bradykinesia tests")
            return

        # Create the comparison analysis window
        comparison_window = tk.Toplevel(self.app.root)
        comparison_window.title("Bradykinesia Angle Sensor Comparison")
        comparison_window.geometry("1200x900")

        # Extract relevant data
        time_data = np.array([item[1] / 1000.0 for item in data])  # Convert to seconds
        value1_data = np.array([item[3] for item in data])  # Analog sensor (0-4095)
        value2_data = np.array([item[4] for item in data])  # IMU angle (degrees)

        # Remove any NaN or invalid values
        valid_indices = ~(np.isnan(value1_data) | np.isnan(value2_data))
        time_clean = time_data[valid_indices]
        value1_clean = value1_data[valid_indices]
        value2_clean = value2_data[valid_indices]

        if len(value1_clean) < 50:
            messagebox.showerror("Error", "Not enough valid data points for angle comparison")
            return

        # Convert analog reading to degrees (0-4095 -> 0-360)
        analog_angle_raw = (value1_clean / 4095.0) * 360.0
        imu_angle = value2_clean.copy()

        # Check if sensors move in opposite directions by looking at correlation
        # Remove any DC offsets first for accurate correlation check
        analog_detrended = analog_angle_raw - np.mean(analog_angle_raw)
        imu_detrended = imu_angle - np.mean(imu_angle)

        # Calculate initial correlation to detect inverse relationship
        initial_correlation, _ = pearsonr(analog_detrended, imu_detrended)

        # If correlation is negative, the sensors move in opposite directions
        direction_inverted = False
        if initial_correlation < -0.3:  # Threshold for detecting inverse correlation
            # Invert the analog sensor direction
            analog_angle_raw = 360.0 - analog_angle_raw
            direction_inverted = True
            self.app.log(
                f"Detected inverse correlation ({initial_correlation:.3f}). Inverting analog sensor direction.")

        # Align the starting angles by adjusting the analog sensor offset
        analog_start = analog_angle_raw[0]
        imu_start = imu_angle[0]
        offset = imu_start - analog_start

        # Adjust analog angle to start at the same point as IMU
        analog_angle_aligned = analog_angle_raw + offset

        # Handle angle wrapping for analog sensor (keep it in reasonable range)
        analog_angle_aligned = self.normalize_angle(analog_angle_aligned)

        # Apply smoothing filter to reduce noise
        if len(time_clean) > 10:
            window_length = min(21, len(time_clean))
            if window_length % 2 == 0:
                window_length -= 1
            if window_length >= 5:
                analog_angle_smooth = savgol_filter(analog_angle_aligned, window_length, 3)
                imu_angle_smooth = savgol_filter(imu_angle, window_length, 3)
            else:
                analog_angle_smooth = analog_angle_aligned
                imu_angle_smooth = imu_angle
        else:
            analog_angle_smooth = analog_angle_aligned
            imu_angle_smooth = imu_angle

        # Calculate correlation
        correlation_coeff, p_value = pearsonr(analog_angle_smooth, imu_angle_smooth)

        # Calculate angle differences
        angle_diff = imu_angle_smooth - analog_angle_smooth
        mean_diff = np.mean(angle_diff)
        std_diff = np.std(angle_diff)

        # Calculate range of motion for both sensors
        analog_range = np.ptp(analog_angle_smooth)
        imu_range = np.ptp(imu_angle_smooth)

        # Create visualization with 2 plots (1x2 grid)
        self.fig = plt.Figure(figsize=(15, 6))

        # Left: Time series comparison
        ax1 = self.fig.add_subplot(121)
        ax1.plot(time_clean, analog_angle_smooth, 'b-', linewidth=2, label='Analog Sensor (Aligned)', alpha=0.8)
        ax1.plot(time_clean, imu_angle_smooth, 'r-', linewidth=2, label='IMU Sensor', alpha=0.8)
        ax1.set_title('Angle Comparison Over Time')
        ax1.set_xlabel('Time (s)')
        ax1.set_ylabel('Angle (degrees)')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        # Right: Scatter plot for correlation
        ax2 = self.fig.add_subplot(122)
        ax2.scatter(analog_angle_smooth, imu_angle_smooth, alpha=0.6, s=25, color='blue', label='Data Points')

        # Add correlation line
        z = np.polyfit(analog_angle_smooth, imu_angle_smooth, 1)
        p = np.poly1d(z)
        ax2.plot(analog_angle_smooth, p(analog_angle_smooth), "r-", alpha=0.8, linewidth=2, label='Correlation Line')

        ax2.set_title(f'Sensor Correlation (r = {correlation_coeff:.3f})')
        ax2.set_xlabel('Analog Sensor Angle (degrees)')
        ax2.set_ylabel('IMU Sensor Angle (degrees)')
        ax2.legend()
        ax2.grid(True, alpha=0.3)

        self.fig.tight_layout()

        # Add figure to canvas
        canvas_frame = ttk.Frame(comparison_window)
        canvas_frame.pack(fill='both', expand=True, padx=10, pady=5)

        self.canvas = FigureCanvasTkAgg(self.fig, master=canvas_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill='both', expand=True)

        # Create results summary frame
        results_frame = ttk.LabelFrame(comparison_window, text="Analysis Results", padding=10)
        results_frame.pack(fill='x', padx=10, pady=10)

        # Display results in a 2x4 grid (wide and short format)
        ttk.Label(results_frame, text="Correlation Coefficient:").grid(row=0, column=0, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{correlation_coeff:.3f}",
                  font=('Arial', 10, 'bold')).grid(row=1, column=0, sticky='w', padx=10, pady=2)

        ttk.Label(results_frame, text="P-value:").grid(row=0, column=1, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{p_value:.4f}",
                  font=('Arial', 10, 'bold')).grid(row=1, column=1, sticky='w', padx=10, pady=2)

        ttk.Label(results_frame, text="Mean Angle Difference:").grid(row=0, column=2, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{mean_diff:.2f}°",
                  font=('Arial', 10, 'bold')).grid(row=1, column=2, sticky='w', padx=10, pady=2)

        ttk.Label(results_frame, text="Std Dev of Difference:").grid(row=0, column=3, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{std_diff:.2f}°",
                  font=('Arial', 10, 'bold')).grid(row=1, column=3, sticky='w', padx=10, pady=2)

        ttk.Label(results_frame, text="Analog Range of Motion:").grid(row=0, column=4, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{analog_range:.2f}°",
                  font=('Arial', 10, 'bold')).grid(row=1, column=4, sticky='w', padx=10, pady=2)

        ttk.Label(results_frame, text="IMU Range of Motion:").grid(row=0, column=5, sticky='w', padx=10, pady=2)
        ttk.Label(results_frame, text=f"{imu_range:.2f}°",
                  font=('Arial', 10, 'bold')).grid(row=1, column=5, sticky='w', padx=10, pady=2)



        # Log the analysis
        self.app.log(f"Bradykinesia angle comparison completed: r={correlation_coeff:.3f}, "
                     f"mean_diff={mean_diff:.2f}°, std_diff={std_diff:.2f}°")

    def normalize_angle(self, angles):
        """Normalize angles to a reasonable range, handling wrapping"""
        # Convert to range [-180, 180] then shift to more appropriate range
        normalized = np.mod(angles + 180, 360) - 180

        # If the range is large, keep it as is
        # If the range is small, shift to be around the mean
        if np.ptp(normalized) < 180:
            return normalized
        else:
            # Handle case where angle wrapping occurred
            # Find the median and shift everything to be around that value
            median_angle = np.median(angles)
            normalized = angles - median_angle
            normalized = np.mod(normalized + 180, 360) - 180
            normalized = normalized + median_angle
            return normalized

    def interpret_results(self, correlation_coeff, mean_diff, std_diff, p_value):
        """Provide interpretation of the angle comparison results"""
        interpretation = ""

        # Interpret correlation
        if abs(correlation_coeff) > 0.9:
            interpretation += "Excellent correlation between sensors. "
        elif abs(correlation_coeff) > 0.8:
            interpretation += "Very good correlation between sensors. "
        elif abs(correlation_coeff) > 0.7:
            interpretation += "Good correlation between sensors. "
        elif abs(correlation_coeff) > 0.5:
            interpretation += "Moderate correlation between sensors. "
        else:
            interpretation += "Poor correlation between sensors. "

        # Interpret statistical significance
        if p_value < 0.001:
            interpretation += "The correlation is highly statistically significant (p < 0.001). "
        elif p_value < 0.01:
            interpretation += "The correlation is statistically significant (p < 0.01). "
        elif p_value < 0.05:
            interpretation += "The correlation is statistically significant (p < 0.05). "
        else:
            interpretation += "The correlation is not statistically significant. "

        # Interpret systematic bias
        if abs(mean_diff) < 2:
            interpretation += "Minimal systematic bias between sensors. "
        elif abs(mean_diff) < 5:
            interpretation += "Small systematic bias between sensors. "
        else:
            interpretation += f"Significant systematic bias of {mean_diff:.1f}° between sensors. "

        # Interpret measurement precision
        if std_diff < 2:
            interpretation += "Excellent agreement in measurement precision. "
        elif std_diff < 5:
            interpretation += "Good agreement in measurement precision. "
        elif std_diff < 10:
            interpretation += "Moderate agreement in measurement precision. "
        else:
            interpretation += "Poor agreement in measurement precision. "

        # Clinical recommendations
        interpretation += "\n\nFor bradykinesia assessment: "
        if abs(correlation_coeff) > 0.8 and std_diff < 5:
            interpretation += "Both sensors provide consistent measurements and can be used interchangeably."
        elif abs(correlation_coeff) > 0.7:
            interpretation += "Sensors show good agreement. Consider the systematic offset when comparing measurements."
        else:
            interpretation += "Significant differences between sensors. Calibration or sensor investigation may be needed."

        return interpretation