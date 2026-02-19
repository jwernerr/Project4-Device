
import argparse
import serial
from serial.tools import list_ports   
import sys

import keyboard

from playsound import playsound


#(stolen) port logic

def choose_port(provided_port=None):
    if provided_port:
        return provided_port
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found. Connect the ESP32 and try again.")
        sys.exit(1)
    if len(ports) == 1:
        return ports[0].device
    # prefer common USB-to-UART descriptors if multiple ports exist
    for p in ports:
        desc = (p.description or "").lower()
        if any(x in desc for x in ("silicon", "cp210", "ch340", "ftdi", "usb-serial")):
            return p.device
    # fallback: return first
    return ports[0].device

BAUD_RATE = 115200

parser = argparse.ArgumentParser(description="Send 'Hello ESP32!' to an ESP32 over serial.")
parser.add_argument("-p", "--port", help="Serial port (e.g. /dev/ttyUSB0 or COM3)")
parser.add_argument("-b", "--baud", type=int, default=BAUD_RATE, help="Baud rate (default: 115200)")
parser.add_argument("-t", "--timeout", type=float, default=1.0, help="Read timeout in seconds")
args = parser.parse_args()

port = choose_port(args.port)

#main loop
running=True

def stop_program():
    running=False # currently not working

keyboard.add_hotkey("a",stop_program)


try:
    with serial.Serial(port, args.baud, timeout=args.timeout) as ser:
        while running:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8').strip()
                if data == "HOLD1":
                    print ("hold1")
                elif data == "RELEASE1":
                    print("release1")
                elif data=="HOLD2":
                    print ("hold2") 
                elif data == "RELEASE2":
                    print("release2")
                elif data == "WAVE":
                    playsound("C:/Users/jenny/Documents/GMB/ProjectIV/Project4-Device/Audio/testAudio.wav")
                    print("should have played")

            
except serial.SerialException as e:
    print(f"Could not open port {port}: {e}")
    sys.exit(1)

