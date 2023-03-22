# This file was just used for testing
import numpy as np
import serial.tools.list_ports
import serial
import re
import csv
import os

CSV_FILE_NAME = "data_2_14_2023"
BAUD_RATE = 9600
PORT = "COM6"
HEADER = ["Coordinates", "Wi-FI RSSI", "LoRa RSSI", "LoRa SNR"]

# This will print the available ports
def printPorts():
    ports = serial.tools.list_ports.comports()
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))


# Write the header if this is a new file. If the file already exists we will not write the header
def writeHeader():
    if os.path.exists(CSV_FILE_NAME) == False:
        with open(CSV_FILE_NAME, "w") as f:
            writer = csv.writer(f)
            writer.writerow(HEADER)


def main():
    printPorts()
    
    writeHeader()
    
    
    # The groups, i.e. the stuff in the parentheses, are as follows: coordinates, Wi-Fi RSSI, LoRa RSSI, and SNR in dB
    re_received_data = re.compile(r".\*(-*\d*,-*\d*),(-*\d*.\d*),(-*\d*.\d*),(-*\d*.\d*,)")

    # Start the serial connection
    ser = serial.Serial(PORT, BAUD_RATE)
    line = ""

    # This will just keep looping over the serial monitor on the receiver and write the data to a CSV
    while(1):
        bytes_to_read = ser.in_waiting
        # If there are bytes to read
        if bytes_to_read:
            line = line + ser.read(bytes_to_read).decode("utf-8")
            found = re_received_data.search(line)
            # If the regex finds a match
            if found != None:
                # Append data to the csv file
                with open(CSV_FILE_NAME, "a") as f:
                    writer = csv.writer(f)
                    writer.writerow([found.group(1), found.group(2), found.group(3), found.group(4)])
                line = ""


if __name__ == '__main__':
    main()