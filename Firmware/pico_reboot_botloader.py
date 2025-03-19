import serial
import serial.tools.list_ports
import time
import shutil
import os

def reset_pico_to_bootloader(baud_rate=1200, retries=3, retry_delay=1):
    """Resets Pico to bootloader with improved error handling."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return False
    selected_port = ports[0].device
    for attempt in range(retries):
        try:
            ser = serial.Serial(selected_port, baud_rate, timeout=1)
            ser.setDTR(False)
            ser.setRTS(True)
            time.sleep(0.1)
            ser.setRTS(False)
            time.sleep(0.1)
            ser.close()
            print(f"Pico reset to bootloader on {selected_port}.")
            return True
        except serial.SerialException as e:
            print(f"Attempt {attempt + 1}: Error: {e}")
            if attempt < retries - 1:
                time.sleep(retry_delay)
            else:
                print("Pico reset failed after multiple attempts.")
                return False
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            return False

def find_pico_drive(drive_name="RPI-RP2"):
    """Simplified drive check."""
    for drive in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        drive_path = os.path.normpath(f"{drive}:\\")
        print(f"Checking drive BEFORE exists: {drive_path}")
        if os.path.exists(os.path.join(drive_path, "INFO_UF2.TXT")):
            print(f"Checking drive AFTER exists: {drive_path}")
            try:
                with open(os.path.join(drive_path, "INFO_UF2.TXT"), "r") as f:
                    if drive_name in f.read():
                        return drive_path
            except:
                pass
        else:
            print(f"INFO_UF2.TXT not found: {drive_path}")
    return None

def copy_uf2_to_pico(uf2_path, pico_drive):
    """Copies .uf2 to the Pico."""
    try:
        shutil.copy2(uf2_path, pico_drive)
        print(f"Copied {uf2_path} to {pico_drive}.")
        return True
    except FileNotFoundError:
        print(f"Error: {uf2_path} not found.")
        return False
    except PermissionError:
        print(f"Error: Permission denied to {pico_drive}.")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == "__main__":
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found. Checking for bootloader mode...")
        time.sleep(5)  # Initial delay
        for i in range(5):  # Try 5 times
            pico_drive = find_pico_drive()
            print(f"find_pico_drive returned: {pico_drive}")
            if pico_drive:
                print(f"Pico drive found: {pico_drive}")
                uf2_file = r"\\wsl.localhost\Ubuntu-22.04\home\envy\Firmware\build\clock.uf2"
                copy_uf2_to_pico(uf2_file, pico_drive)
                break
            else:
                print(f"Attempt {i + 1}: Pico drive not found. Waiting...")
                time.sleep(5)  # Increase delay each time
        else:
            print("Pico drive not found after multiple attempts.")
    else:
        if reset_pico_to_bootloader():
            time.sleep(5)
            pico_drive = find_pico_drive()
            print(f"find_pico_drive returned: {pico_drive}")
            if pico_drive:
                print(f"Pico drive found: {pico_drive}")
                uf2_file = r"\\wsl.localhost\Ubuntu-22.04\home\envy\Firmware\build\clock.uf2"
                copy_uf2_to_pico(uf2_file, pico_drive)
            else:
                print("Pico drive not found after reset.")
        else:
            print("Pico reset failed.")