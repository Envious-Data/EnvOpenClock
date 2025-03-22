import serial
import serial.tools.list_ports
import time
import shutil
import os
import logging

def reset_pico_to_bootloader(baud_rate=1200, retries=1, retry_delay=1):
    """Resets Pico to bootloader with improved error handling."""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return False
    selected_port = ports[0].device
    logging.info(f"Resetting Pico via serial port {selected_port} at 1200 baud.")
    try:
        ser = serial.Serial(selected_port, baud_rate, timeout=1)
        ser.setDTR(False)
        ser.setRTS(True)
        time.sleep(0.1)
        ser.setRTS(False)
        time.sleep(0.1)
        ser.close()
        logging.info(f"Pico reset attempt on {selected_port}.")
        return True
    except Exception as e:
        return False #suppress serial port error

def find_pico_drive(drive_name="RPI-RP2"):
    """Simplified drive check."""
    for drive in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        drive_path = os.path.normpath(f"{drive}:\\")
        if os.path.exists(os.path.join(drive_path, "INFO_UF2.TXT")):
            try:
                with open(os.path.join(drive_path, "INFO_UF2.TXT"), "r") as f:
                    if drive_name in f.read():
                        return drive_path
            except:
                pass
    return None

def copy_uf2_to_pico(uf2_path, pico_drive):
    """Copies .uf2 to the Pico."""
    try:
        shutil.copy2(uf2_path, pico_drive)
        logging.info(f"Copied {uf2_path} to {pico_drive}.")
        return True
    except FileNotFoundError:
        logging.error(f"Error: {uf2_path} not found.")
        return False
    except PermissionError:
        logging.error(f"Error: Permission denied to {pico_drive}.")
        return False
    except Exception as e:
        logging.error(f"Error: {e}")
        return False

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        logging.info("No serial ports found. Checking for bootloader mode...")
        time.sleep(1)  # Initial delay
        for i in range(5):  # Try 5 times
            logging.info(f"Checking for Pico drive, attempt {i+1}")
            pico_drive = find_pico_drive()
            if pico_drive:
                logging.info(f"Pico drive found: {pico_drive}")
                logging.info(f"Found Pico in bootloader, copying file now.")
                uf2_file = r"clock.uf2"
                copy_uf2_to_pico(uf2_file, pico_drive)
                break
            else:
                logging.info(f"Attempt {i + 1}: Pico drive not found. Waiting...")
                time.sleep(1)  # Increase delay each time
        else:
            logging.error("Pico drive not found after multiple attempts.")
    else:
        logging.info("Serial port detected, attempting to reset Pico to bootloader.")
        reset_pico_to_bootloader() #attempt reset
        logging.info("Waiting for Pico to enter bootloader mode.")
        time.sleep(1) #wait for drive to appear
        pico_drive = None
        for i in range(5):
            logging.info(f"Checking for Pico drive, attempt {i+1}")
            pico_drive = find_pico_drive()
            if pico_drive:
                break;
            else:
                time.sleep(1)
        if pico_drive:
            logging.info(f"Pico drive found: {pico_drive}")
            logging.info(f"Pico found after reset, copying file now.")
            uf2_file = r"clock.uf2"
            copy_uf2_to_pico(uf2_file, pico_drive)
        else:
            logging.error("Pico drive not found after reset attempt.")