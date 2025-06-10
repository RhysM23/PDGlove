import numpy as np
import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


class ForceAnalyzer:
    def __init__(self, app):
        self.app = app
        self.canvas = None
        self.fig = None

        # Sensor conversion parameters
        self.VCC = 3.3  # ESP32 supply voltage
        self.R_DIVIDER = 4700  # 4.7kΩ resistor in voltage divider
        self.ADC_MAX = 4095  # 12-bit ADC maximum value

        # Force-resistance equation parameters: y = 153.18 * x^(-0.699)
        # Where y is force in KILOGRAMS-FORCE (kgf) and x is resistance in OHMS
        self.FORCE_COEFF = 153.18
        self.FORCE_EXPONENT = -0.699

        # Choose output unit: 'kilograms-force', 'grams-force', or 'newtons'
        self.force_unit = 'newtons'  # Convert to Newtons for display

        # Set appropriate label based on unit
        if self.force_unit == 'kilograms-force':
            self.force_label = 'kgf'
        elif self.force_unit == 'grams-force':
            self.force_label = 'gf'
        else:  # newtons
            self.force_label = 'N'

    def adc_to_force(self, adc_reading, output_unit='newtons'):
        """
        Convert 12-bit ADC reading to force using the sensor characteristics

        Conversion steps:
        1. ADC reading → Voltage
        2. Voltage → Sensor resistance (using voltage divider)
        3. Resistance → Force in kilograms-force (using given equation: y = 153.18 * x^(-0.699))
        4. Convert to desired output unit (default: Newtons)

        Args:
            adc_reading: 12-bit ADC value (0-4095)
            output_unit: 'kilograms-force', 'grams-force', or 'newtons'
        """
        # Step 1: Convert ADC reading to voltage
        voltage = (adc_reading / self.ADC_MAX) * self.VCC

        # Step 2: Convert voltage to sensor resistance using voltage divider equation
        # Vout = Vcc * R / (Rs + R)
        # Solving for Rs: Rs = R * (Vcc - Vout) / Vout
        # Handle edge cases where voltage is too close to VCC or 0
        if voltage <= 0.01:  # Prevent division by zero
            voltage = 0.01
        elif voltage >= self.VCC - 0.01:  # Sensor resistance near zero
            voltage = self.VCC - 0.01

        sensor_resistance = self.R_DIVIDER * (self.VCC - voltage) / voltage

        # Step 3: Convert resistance to force using y = 153.18 * x^(-0.699)
        # This gives force directly in KILOGRAMS-FORCE (kgf)
        if sensor_resistance <= 0:
            return 0

        # Apply sensor equation to get force in kilograms-force
        force_kgf = self.FORCE_COEFF * (sensor_resistance ** self.FORCE_EXPONENT)
        force_kgf = max(0, force_kgf)  # Ensure force is never negative

        # Step 4: Convert to desired unit if needed
        if output_unit == 'kilograms-force':
            return force_kgf
        elif output_unit == 'grams-force':
            # Convert kilograms-force to grams-force (1 kgf = 1000 gf)
            return force_kgf * 1000.0
        elif output_unit == 'newtons':
            # Convert kilograms-force to Newtons (1 kgf = 9.81 N)
            return force_kgf * 9.81
        else:
            raise ValueError("output_unit must be 'kilograms-force', 'grams-force', or 'newtons'")

    def adc_to_force_kgf(self, adc_reading):
        """Convenience method to get force in kilograms-force"""
        return self.adc_to_force(adc_reading, output_unit='kilograms-force')

    def adc_to_force_gf(self, adc_reading):
        """Convenience method to get force in grams-force"""
        return self.adc_to_force(adc_reading, output_unit='grams-force')

    def adc_to_force_newtons(self, adc_reading):
        """Convenience method to get force in Newtons"""
        return self.adc_to_force(adc_reading, output_unit='newtons')

    def analyze(self, data, measure_type):
        """Analyze force data from sensors and convert to actual force units"""
        if not data or len(data) < 10:
            self.app.show_error("Not enough data for force analysis")
            return

        # Only proceed if this is a stiffness measurement (mode 3)
        mode = data[0][2]  # Get mode from first data point
        if mode != 3:
            messagebox.showinfo("Info", "Force analysis is only applicable for Stiffness tests")
            return

        # Create force analysis window
        force_window = tk.Toplevel(self.app.root)
        force_window.title("Force Analysis")
        force_window.geometry("1200x900")

        # Extract relevant data
        time_data = np.array([item[1] / 1000.0 for item in data])  # Convert to seconds

        # Check if we have the new 6-element format or old 5-element format
        if len(data[0]) >= 6:  # New format with 6+ elements
            force1_adc = np.array([item[3] for item in data])  # value1 (Force sensor 1)
            force2_adc = np.array([item[4] for item in data])  # value2 (Force sensor 2)
            angle_data = np.array([item[5] for item in data])  # value3 (Angle)

            # Convert ADC readings to force values
            force1_values = np.array([self.adc_to_force(adc, self.force_unit) for adc in force1_adc])
            force2_values = np.array([self.adc_to_force(adc, self.force_unit) for adc in force2_adc])

            self.app.log(f"Using new data format with ADC to force conversion (output: {self.force_unit})")
        else:  # Old format - assume already converted values
            force1_values = np.array([item[3] for item in data])  # Already in force units
            force2_values = np.array([item[4] for item in data])  # Already in force units
            angle_data = np.array([item[5] for item in data]) if len(data[0]) > 5 else np.zeros(len(data))

            self.app.log("Using old data format - assuming values already in force units")

        # Calculate metrics
        max_force1 = np.max(force1_values)
        max_force2 = np.max(force2_values)
        avg_force1 = np.mean(force1_values)
        avg_force2 = np.mean(force2_values)
        total_force = np.max(force1_values + force2_values)

        # Calculate combined force over time
        combined_force = force1_values + force2_values

        # Work calculation using force and angle
        work_done = self.calculate_work(force1_values, force2_values, angle_data, time_data)

        # Calculate force rate (how quickly force changes)
        force_rate1 = np.max(np.abs(np.diff(force1_values))) / np.mean(np.diff(time_data))
        force_rate2 = np.max(np.abs(np.diff(force2_values))) / np.mean(np.diff(time_data))

        # Create results frame
        results_frame = ttk.LabelFrame(force_window, text="Force Metrics", padding=10)
        results_frame.pack(fill='x', padx=10, pady=10)

        # Display results in a grid layout
        ttk.Label(results_frame, text="Maximum Force (Sensor 1):").grid(row=0, column=0, sticky='w', padx=5, pady=5)
        ttk.Label(results_frame, text=f"{max_force1:.2f} {self.force_label}", font=('Arial', 10, 'bold')).grid(
            row=0, column=1, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Maximum Force (Sensor 2):").grid(row=1, column=0, sticky='w', padx=5, pady=5)
        ttk.Label(results_frame, text=f"{max_force2:.2f} {self.force_label}", font=('Arial', 10, 'bold')).grid(
            row=1, column=1, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Average Force (Sensor 1):").grid(row=2, column=0, sticky='w', padx=5, pady=5)
        ttk.Label(results_frame, text=f"{avg_force1:.2f} {self.force_label}", font=('Arial', 10, 'bold')).grid(
            row=2, column=1, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Average Force (Sensor 2):").grid(row=3, column=0, sticky='w', padx=5, pady=5)
        ttk.Label(results_frame, text=f"{avg_force2:.2f} {self.force_label}", font=('Arial', 10, 'bold')).grid(
            row=3, column=1, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Peak Combined Force:").grid(row=0, column=2, sticky='w', padx=15, pady=5)
        ttk.Label(results_frame, text=f"{total_force:.2f} {self.force_label}", font=('Arial', 10, 'bold')).grid(
            row=0, column=3, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Work Done:").grid(row=1, column=2, sticky='w', padx=15, pady=5)
        ttk.Label(results_frame, text=f"{work_done:.3f} J", font=('Arial', 10, 'bold')).grid(
            row=1, column=3, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Max Force Rate (Sensor 1):").grid(row=2, column=2, sticky='w', padx=15, pady=5)
        ttk.Label(results_frame, text=f"{force_rate1:.2f} {self.force_label}/s", font=('Arial', 10, 'bold')).grid(
            row=2, column=3, sticky='w', padx=5, pady=5)

        ttk.Label(results_frame, text="Max Force Rate (Sensor 2):").grid(row=3, column=2, sticky='w', padx=15, pady=5)
        ttk.Label(results_frame, text=f"{force_rate2:.2f} {self.force_label}/s", font=('Arial', 10, 'bold')).grid(
            row=3, column=3, sticky='w', padx=5, pady=5)

        # Create visualization
        self.fig = plt.Figure(figsize=(12, 10))



        # Individual sensor analysis
        ax2 = self.fig.add_subplot(222)
        ax2.plot(time_data, force1_values, 'r-', linewidth=2,
                 label=f'Sensor 1 (Max: {max_force1:.2f}{self.force_label})')
        ax2.plot(time_data, force2_values, 'b-', linewidth=2,
                 label=f'Sensor 2 (Max: {max_force2:.2f}{self.force_label})')
        ax2.set_title("Individual Sensor Forces")
        ax2.set_xlabel("Time (s)")
        ax2.set_ylabel(f"Force ({self.force_label})")
        ax2.legend()
        ax2.grid(True, alpha=0.3)





        # Add figure to canvas
        self.canvas = FigureCanvasTkAgg(self.fig, master=force_window)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill='both', expand=True, padx=10, pady=10)

        # Log the analysis
        self.app.log(
            f"Force analysis completed: Max force: {total_force:.2f} {self.force_label}, Work done: {work_done:.3f} J")
        self.app.log(
            f"Force range: Sensor1={max_force1:.2f}{self.force_label}, Sensor2={max_force2:.2f}{self.force_label}")

    def calculate_work(self, force1_values, force2_values, angle_data, time_data):
        """
        Calculate work done using force and either angle or time data
        Work = Force × distance, always output in Joules (N⋅m)
        """
        try:
            total_force = force1_values + force2_values

            if np.any(angle_data != 0):
                # If we have angle data, use it to estimate displacement
                angle_radians = np.radians(angle_data)
                lever_arm = 0.05  # Assume 5cm lever arm

                # Calculate displacement from angle changes
                angle_diff = np.diff(angle_radians)
                displacement = angle_diff * lever_arm

                # Use average force for each step
                avg_force = (total_force[:-1] + total_force[1:]) / 2

                # Calculate work for each step
                work_steps = avg_force * np.abs(displacement)
                total_work = np.sum(work_steps)

                # Convert to Joules based on current unit
                if self.force_unit == 'kilograms-force':
                    # Convert kgf⋅m to Joules: 1 kgf⋅m = 9.81 J
                    total_work = total_work * 9.81
                elif self.force_unit == 'grams-force':
                    # Convert gf⋅m to Joules: 1 gf⋅m = 0.00981 J
                    total_work = total_work * 0.00981
                # If newtons, already in correct units (N⋅m = J)

            else:
                # Estimate work from force-time curve (power integration)
                # This is a rough approximation
                dt = np.mean(np.diff(time_data))
                force_changes = np.abs(np.diff(total_force))

                # Rough estimate: assume movement velocity proportional to force change
                estimated_velocity = force_changes * 0.001  # m/s, adjust scaling factor as needed

                # Work = Force × distance, where distance = velocity × time
                # Convert force to Newtons for consistent work calculation
                if self.force_unit == 'kilograms-force':
                    force_newtons = total_force[:-1] * 9.81
                elif self.force_unit == 'grams-force':
                    force_newtons = total_force[:-1] * 0.00981
                else:  # already in newtons
                    force_newtons = total_force[:-1]

                work_steps = force_newtons * estimated_velocity * dt
                total_work = np.sum(work_steps)

            return max(0, total_work)

        except Exception as e:
            self.app.log(f"Work calculation error: {str(e)}")
            return 0