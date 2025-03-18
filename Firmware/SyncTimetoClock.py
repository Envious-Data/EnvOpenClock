import serial
import datetime
import time

# Configure serial port (adjust as needed)
serial_port = 'COM3'  # Example: COM3 (Windows)
baud_rate = 115200

try:
    ser = serial.Serial(serial_port, baud_rate, timeout=1)
    print(f"Serial port {serial_port} opened successfully.")

    while True:
        now = datetime.datetime.now()
        time_str = now.strftime("%Y-%m-%d %H:%M:%S")
        command = f"settime {time_str}\n"

        try:
            ser.write(command.encode())
            print(f"Sent: {command.strip()}")
        except serial.SerialException as e:
            print(f"Error writing to serial port: {e}")
            break #stop sending if there is an issue

        time.sleep(60)  # Send time every 60 seconds (adjust as needed)

except serial.SerialException as e:
    print(f"Error opening serial port: {e}")

finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")
