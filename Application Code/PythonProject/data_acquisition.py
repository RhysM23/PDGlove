import serial
import time


class SerialDataCollector:
    def __init__(self, app):
        self.app = app
        self.serial_port = None

    def collect_data(self, port, measure_type, timeout):
        """Collect data from the ESP32 via serial port"""
        try:
            # Connect to serial port
            self.app.status_var.set("Connecting to device...")
            self.app.log(f"Connecting to serial port {port}")
            self.serial_port = serial.Serial(port, 115200, timeout=2)
            time.sleep(1)  # Allow time for connection to establish

            # Send command based on measurement type
            self.app.status_var.set("Sending command...")
            command = ""
            if measure_type == "Tremor":
                command = "TREM\n"
                mode = 1
            elif measure_type == "Bradykinesia":
                command = "BRAD\n"
                mode = 2
            elif measure_type == "Stiffness":
                command = "STIF\n"
                mode = 3
            else:
                mode = 0

            self.app.log(f"Sending command: {command.strip()}")
            self.serial_port.write(command.encode())

            # Wait for and collect data
            self.app.status_var.set("Waiting for data...")
            max_index = None
            last_data_time = time.time()

            while self.app.collecting:
                if time.time() - last_data_time > timeout:
                    # Timeout occurred - show a message but don't lose data
                    self.app.root.after(0, lambda: self.app.status_var.set(f"Timeout - no data for {timeout} seconds"))
                    self.app.log(f"Data collection timeout after {timeout} seconds")
                    break

                try:
                    line = self.serial_port.readline().decode().strip()

                    if line.startswith("DATA"):
                        # Parse data line with new format for 5 values:
                        # "DATAindex×max_index×time_ms×value1×value2×value3×value4×value5"
                        parts = line[4:].split('x')
                        if len(parts) >= 6:  # Support both old (6 parts) and new (8 parts) formats
                            index = int(parts[0])
                            max_index = int(parts[1])
                            time_ms = int(parts[2])
                            value1 = float(parts[3])
                            value2 = float(parts[4])
                            value3 = float(parts[5])

                            # Handle new format with 5 values (8 parts total)
                            if len(parts) >= 8:
                                value4 = float(parts[6])
                                value5 = float(parts[7])
                                # Store data with mode and all 5 values
                                self.app.data.append([index, time_ms, mode, value1, value2, value3, value4, value5])
                            else:
                                # Backward compatibility: old format with 3 values
                                # Store data with mode and 3 values, pad with zeros for missing values
                                self.app.data.append([index, time_ms, mode, value1, value2, value3, 0.0, 0.0])

                            # Reset timeout timer
                            last_data_time = time.time()

                            # Update progress
                            if max_index > 0:
                                progress = (index / max_index) * 100
                                self.app.root.after(0, lambda p=progress: self.app.progress_var.set(p))
                                self.app.root.after(0, lambda i=index, m=max_index:
                                self.app.status_var.set(f"Receiving data: {i + 1}/{m + 1} points"))

                            # Check if we're done - allow for off-by-one errors
                            if index >= max_index - 1:  # Consider "close enough" to be done
                                self.app.log(
                                    f"Data collection complete: {len(self.app.data)} of {max_index + 1} points received")
                                self.app.root.after(0, lambda: self.app.status_var.set(
                                    f"Measurement complete ({len(self.app.data)}/{max_index + 1} points)"))
                                break
                except UnicodeDecodeError:
                    # Handle potential garbled data
                    continue
                except Exception as e:
                    # Log other errors but keep trying
                    self.app.log(f"Error reading line: {e}")
                    continue

            # Close serial port
            try:
                if self.serial_port and self.serial_port.is_open:
                    self.serial_port.close()
                    self.app.log("Serial port closed")
            except Exception as e:
                self.app.log(f"Error closing serial port: {e}")
            self.serial_port = None

            # Update UI in main thread
            self.app.root.after(0, self.app.measurement_complete)

        except Exception as e:
            # Show error in main thread
            self.app.log(f"Collection error: {e}")
            self.app.root.after(0, lambda: self.app.show_error(f"Error: {str(e)}"))