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
        print("\nMain Menu:")
        print("1. Set Time")
        print("2. Select Animation")
        print("3. Exit")

        choice = input("Enter your choice (1, 2, or 3): ")

        if choice == '1':
            now = datetime.datetime.now()
            time_str = now.strftime("%Y-%m-%d %H:%M:%S")
            command = f"settime {time_str}\n"

            try:
                ser.write(command.encode())
                print(f"Sent: {command.strip()}")
            except serial.SerialException as e:
                print(f"Error writing to serial port: {e}")
                break  # stop sending if there is an issue

            time.sleep(1)  # Small delay after sending time

        elif choice == '2':
            animation_number = input("Enter animation number (0-5, or adjust as needed): ")
            try:
                anim_num = int(animation_number)
                command = f"anim {anim_num}\n"

                try:
                    ser.write(command.encode())
                    print(f"Sent: {command.strip()}")
                except serial.SerialException as e:
                    print(f"Error writing to serial port: {e}")
                    break

                time.sleep(1)

            except ValueError:
                print("Invalid animation number. Please enter an integer.")

        elif choice == '3':
            break

        else:
            print("Invalid choice. Please enter 1, 2, or 3.")

except serial.SerialException as e:
    print(f"Error opening serial port: {e}")

finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")