import serial
import serial.tools.list_ports
import time

def monitor_serial(port, baudrate):
    """Monitors a serial port and prints received data, including non-UTF-8 bytes."""
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Monitoring {port} at {baudrate} baud...")
        while True:
            try:
                if ser.in_waiting > 0:  # Check if there's data in the buffer
                    data = ser.read(ser.in_waiting) # Read all available bytes
                    try:
                        line = data.decode('utf-8') # Try to decode as UTF-8
                        print(line, end='') # Print the decoded string immediately.
                    except UnicodeDecodeError:
                        # If not UTF-8, print the raw bytes as hexadecimal
                        print(f"Received non-UTF-8 bytes: {data.hex()}")

            except serial.SerialException as e:
                print(f"Serial port error: {e}")
                break
            except KeyboardInterrupt:
                print("Monitoring stopped by user.")
                break
            time.sleep(0.01) # Small delay

    except serial.SerialException as e:
        print(f"Error opening serial port {port}: {e}")

def find_com_port(target_port):
    """Finds a COM port by name, and returns the full port name"""
    ports = serial.tools.list_ports.comports()
    for port, desc, hwid in sorted(ports):
        if target_port in port:
            return port
    return None

if __name__ == "__main__":
    target_port = "COM3"  # Change to your port
    baudrate = 115200

    full_port_name = find_com_port(target_port)

    if full_port_name:
        monitor_serial(full_port_name, baudrate)
    else:
        print(f"{target_port} not found.")