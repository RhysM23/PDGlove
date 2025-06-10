import tkinter as tk
from app import SensorApp


def main():
    """Main entry point of the application"""
    root = tk.Tk()
    root.title("Sensor Analysis Application")
    root.geometry("1024x800")  # Slightly larger window to accommodate new button

    # Create the application
    app = SensorApp(root)

    # Start the main event loop
    root.mainloop()


if __name__ == "__main__":
    main()