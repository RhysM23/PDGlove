import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
import tkinter as tk


class DataVisualizer:
    def __init__(self, app):
        self.app = app
        self.canvas = None
        self.fig = None

    def plot_raw_data(self, parent_frame, data, measure_type):
        """Plot the raw sensor data"""
        if not data:
            return

        # Clear parent frame
        for widget in parent_frame.winfo_children():
            widget.destroy()

        # Extract data columns
        time_data = [item[1] / 1000.0 for item in data]  # Convert to seconds
        mode = data[0][2]  # Get mode from first data point

        # Extract all available values
        value1_data = [item[3] for item in data]
        value2_data = [item[4] for item in data]
        value3_data = [item[5] for item in data]

        # Check if we have 5 values (new format) or 3 values (old format)
        has_5_values = len(data[0]) >= 8
        if has_5_values:
            value4_data = [item[6] for item in data]
            value5_data = [item[7] for item in data]

        # Determine number of subplots based on mode and data format
        if mode == 3 and has_5_values:  # Stiffness with 5 sensors
            num_plots = 5
            self.fig = plt.Figure(figsize=(9, 12))  # Taller figure for 5 plots
        else:
            num_plots = 3
            self.fig = plt.Figure(figsize=(9, 8))

        # Set appropriate y-axis labels and limits based on mode
        if mode == 1:  # Tremor
            sensor_titles = ["X Acceleration", "Y Acceleration", "Z Acceleration"]
            y_labels = ["Acceleration (g)"] * 3
            y_limits = [(-2, 2)] * 3
        elif mode == 2:  # Bradykinesia
            sensor_titles = ["Contact State", "Angle 1 (roll1)", "Angle 2 (roll2)"]
            y_labels = ["State (0/1)", "Angle (degrees)", "Angle (degrees)"]
            y_limits = [(0, 1), (-180, 180), (-180, 180)]
        elif mode == 3:  # Stiffness
            if has_5_values:
                sensor_titles = ["Force Sensor 1", "Force Sensor 2", "Angle (roll2)",
                                 "Force Sensor 3", "Force Sensor 4"]
                y_labels = ["Force (normalized)", "Force (normalized)", "Angle (degrees)",
                            "Force (normalized)", "Force (normalized)"]
                y_limits = [(0, 1), (0, 1), (-180, 180), (0, 1), (0, 1)]
            else:
                sensor_titles = ["Force Sensor 1", "Force Sensor 2", "Angle (roll2)"]
                y_labels = ["Force (normalized)", "Force (normalized)", "Angle (degrees)"]
                y_limits = [(0, 1), (0, 1), (-180, 180)]
        else:
            # Default for unknown mode
            all_values = value1_data + value2_data + value3_data
            if has_5_values:
                all_values += value4_data + value5_data
            sensor_titles = [f"Sensor {i + 1}" for i in range(num_plots)]
            y_labels = ["Value"] * num_plots
            y_limits = [(min(all_values), max(all_values))] * num_plots

        # Colors for each sensor
        colors = ['green', 'blue', 'purple', 'red', 'orange']

        # Create subplots
        for i in range(num_plots):
            ax = self.fig.add_subplot(num_plots, 1, i + 1)

            # Select data for this subplot
            if i == 0:
                plot_data = value1_data
            elif i == 1:
                plot_data = value2_data
            elif i == 2:
                plot_data = value3_data
            elif i == 3 and has_5_values:
                plot_data = value4_data
            elif i == 4 and has_5_values:
                plot_data = value5_data
            else:
                continue

            # Plot the data
            ax.plot(time_data, plot_data, color=colors[i], linewidth=1)
            ax.set_title(sensor_titles[i])
            ax.set_ylabel(y_labels[i])
            ax.set_xlim(0, max(time_data) if time_data else 10)
            ax.set_ylim(*y_limits[i])
            ax.grid(True)

            # Only add x-label to bottom plot
            if i == num_plots - 1:
                ax.set_xlabel("Time (s)")

        # Update the layout
        self.fig.tight_layout()

        # Create canvas and add to frame
        self.canvas = FigureCanvasTkAgg(self.fig, master=parent_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill='both', expand=True, padx=10, pady=10)

        # Add sensor labels with explanations based on test type
        info_frame = tk.Frame(parent_frame)
        info_frame.pack(fill='x', padx=10, pady=5)

        # Add sensor explanations based on test type
        if measure_type == "Tremor":
            descriptions = [
                "Sensor 1: Accelerometer X-axis (g)",
                "Sensor 2: Accelerometer Y-axis (g)",
                "Sensor 3: Accelerometer Z-axis (g)"
            ]
        elif measure_type == "Bradykinesia":
            descriptions = [
                "Sensor 1: Contact sensor (0/1)",
                "Sensor 2: IMU angle (roll1, degrees)",
                "Sensor 3: IMU angle (roll2, degrees)"
            ]
        elif measure_type == "Stiffness":
            if has_5_values:
                descriptions = [
                    "Sensor 1: Force sensor 1 - pin 25 (normalized 0-1)",
                    "Sensor 2: Force sensor 2 - pin 26 (normalized 0-1)",
                    "Sensor 3: IMU angle (roll2, degrees)",
                    "Sensor 4: Force sensor 3 - pin 32 (normalized 0-1)",
                    "Sensor 5: Force sensor 4 - pin 33 (normalized 0-1)"
                ]
            else:
                descriptions = [
                    "Sensor 1: Force sensor 1 (normalized 0-1)",
                    "Sensor 2: Force sensor 2 (normalized 0-1)",
                    "Sensor 3: IMU angle (roll2, degrees)"
                ]
        else:
            descriptions = [f"Sensor {i + 1}" for i in range(num_plots)]

        # Display descriptions with appropriate colors
        for i, desc in enumerate(descriptions):
            if i < len(colors):
                tk.Label(info_frame, text=desc, fg=colors[i]).pack(anchor='w')
            else:
                tk.Label(info_frame, text=desc).pack(anchor='w')