import numpy as np
import serial.tools.list_ports
import serial
import csv
import pandas as pd
from scipy.ndimage import gaussian_filter
import re

LATITUDE = 0
LONGITUDE = 1
WIFI_RSSI_1 = 2
WIFI_RSSI_2 = 3
RSSI = 4
SNR = 5
PACKET_COUNT = 6


SERIAL_PORT = 'COM6' #### Change this according to printPorts
HEADER = ["Latitude", "Longitude", "Wi-FI RSSI", "LoRa RSSI", "LoRa SNR"]


class HeatMap():
    def __init__(self, csv_file_path=None, read_from_csv=False):
        self.sigma = 48
        self.location_x = []
        self.location_y = []
        self.first_plot = True
        self.bounds = 0.00045
        self.x = None
        self.y = None
        self.is_done = False
        # The groups, i.e. the stuff in the parentheses, are as follows: lat, lon, Wi-Fi RSSI (which matches an RSSI value or nan), LoRa RSSI, and SNR in dB
        if read_from_csv == False:
            self.re_received_data = re.compile(r".\*(-*\d*),(-*\d*),(-*\d*.\d*|\bnan\b),(-*\d*.\d*),(-*\d*.\d*)")
            self.serial_output = ""
            self.ser = serial.Serial(SERIAL_PORT, 9600) ### change port number as needed
        # If we want to use useCSV, we want to read from a csv
        if read_from_csv is not None and read_from_csv:
            self.csv_data = self.prepareCSV(csv_file_path)
        # If just a path is provided, that means we want to write to a csv while using useSerial. 
        # Note, the path to the file shouldn't actually exist, i.e. the file should not be 
        # created beforehand. Just pass in a path and the file will be located there
        elif csv_file_path is not None:
            self.csv_path = csv_file_path
            dict = {'Latitude':[], 'Longitude':[], 'Wi-Fi RSSI':[], 'LoRa RSSI':[], 'LoRa SNR':[]}
            self.df = pd.DataFrame(dict)

    '''
    This is the function to call when using a CSV file to read the data 
    from a csv (see csv files under heatmap_visualization).
    '''
    def useCSV(self):
        # Initialize these so we can return at the end
        extent_array = None
        heatmap = None
        
        # Since we are popping the lines from the file, eventually the file will
        # have no more lines left.
        if len(self.csv_data) == 0:
            print("Finished reading data")
            exit()
        
        line = self.csv_data.pop()

        if "\r\n" in line and "*" in line:
            output = line[line.index("*") + 1:line.index("\r\n")]
            output_arr = output.split(",")
            if len(output_arr) > 1:
                most_recent_x, most_recent_y = self.getRecentValues(output_arr)
            
            # If the array is longer than 2, it will have RSSI values
            if len(output_arr) > 2 and ("nan" not in output_arr[WIFI_RSSI_1] or "nan" not in output_arr[WIFI_RSSI_2]):
                if self.first_plot:
                    self.x = np.array([most_recent_x - self.bounds, most_recent_x + self.bounds])
                    self.y = np.array([most_recent_y - self.bounds, most_recent_y + self.bounds])
                    self.first_plot = False
            
                extent_array = self.updateBounds(most_recent_x, most_recent_y)
                is_duplicate = ((most_recent_x in self.x) or (most_recent_y in self.y))
                rssi_avg = self.getRSSIAvg(output_arr)

                if not is_duplicate:
                    self.addProportionalPoints(rssi_avg, most_recent_x, most_recent_y)
                heatmap = self.getHeatMap()
        return extent_array, heatmap
                    

    '''
    Read the csv file into an array
    '''
    def prepareCSV(self, csv_file_path):
        with open(csv_file_path, mode='r') as file:
            next(file) # skip header line
            csv_file = csv.reader(file)
            data = []
            for line in csv_file:
                line[0] = str(float(line[0]) * 10000000)
                line[1] = str(float(line[1]) * 10000000)
                delim = ","
                line_data = delim.join(line)
                line_data = "*" + line_data + "\r\n"
                data.append(line_data)
        return data


    '''
    Get the lat long info from output_arr, add it to location_x and y, return those values
    '''
    def getRecentValues(self, output_arr):
        most_recent_x = float(output_arr[LONGITUDE]) / 10000000
        most_recent_y = float(output_arr[LATITUDE]) / 10000000
        self.location_x.append(most_recent_x)
        self.location_y.append(most_recent_y)
        output_arr[LONGITUDE] = most_recent_x
        output_arr[LATITUDE] = most_recent_y
        return most_recent_x, most_recent_y

    
    '''
    This will update the bounds of the heatmap if needed. It returns the extent array
    '''
    def updateBounds(self, most_recent_x, most_recent_y):
        min_x = min(self.x)
        max_x = max(self.x)
        min_y = min(self.y)
        max_y = max(self.y)

        if min_x > most_recent_x and abs(most_recent_x - min_x) < 1:
            min_x = most_recent_x - self.bounds
            self.x = np.append(self.x, min_x) # note, min_x was just changed to be the most recent point minus the bounds
            self.y = np.append(self.y, most_recent_y)
        elif most_recent_x > max_x and abs(most_recent_x - max_x) < 1:
            max_x = most_recent_x + self.bounds
            self.x = np.append(self.x, max_x)
            self.y = np.append(self.y, most_recent_y)

        if min_y > most_recent_y and abs(most_recent_y - min_y) < 1:
            min_y = most_recent_y - self.bounds
            self.y = np.append(self.y, min_y)
            self.x = np.append(self.x, most_recent_x)
        elif most_recent_y > max_y and abs(most_recent_y - max_y) < 1:
            max_y = most_recent_y + self.bounds
            self.y = np.append(self.y, max_y)
            self.x = np.append(self.x, most_recent_x)
        return [min_x, max_x, min_y, max_y]

    
    '''
    This will read all the RSSI values and return the average
    '''
    def getRSSIAvg(self, output_arr):
        rssi_vals = []
        if output_arr[WIFI_RSSI_1] != "nan":
            rssi_1 = float(output_arr[WIFI_RSSI_1])
            rssi_vals.append(rssi_1)
        if output_arr[WIFI_RSSI_2] != "nan":
            rssi_2 = float(output_arr[WIFI_RSSI_2])
            rssi_vals.append(rssi_2)
        return sum(rssi_vals) / len(rssi_vals) # this is the average
    

    '''
    The stronger (higher) the rssi_avg, the more points will be put at the location
    '''
    def addProportionalPoints(self, rssi_avg, most_recent_x, most_recent_y):
        # The number of points added to the heatmap is proportional to the strength of the avg RSSI value
        num_pts = 0.4 * 1.75**(0.165 * (120 + rssi_avg))
        for _ in range(int(num_pts)):
            self.x = np.append(self.x, most_recent_x)
            self.y = np.append(self.y, most_recent_y)
    

    '''
    This creates the heatmap. Note, the transpose is returned to get the orientation right
    '''
    def getHeatMap(self, num_bins=1000):
        heatmap, xedges, yedges = np.histogram2d(self.x, self.y, bins=num_bins)
        heatmap = gaussian_filter(heatmap, sigma=self.sigma)
        return heatmap.T


    '''
    This method will read from the serial monitor on Arduino and then create a heatmap. 
    Note, the serial monitor in Arduino has to be closed so this program can read from it (see TP-002 from Team 28)
    '''
    def useSerial(self):
        extent_array = None
        heatmap = None
        
        try:
            bytes_to_read = self.ser.in_waiting
        except IOError:
            raise IOError()
        
        if bytes_to_read:
            try:
                self.serial_output = self.serial_output + self.ser.read(bytes_to_read).decode("utf-8")
                found = self.re_received_data.search(self.serial_output)
                print(found)
            except UnicodeDecodeError:
                found = None
                print("Bad start bit")
            

            if found != None:
                # This means we want to write the data to a csv for later use
                if self.csv_path is not None:
                    dict = {'Latitude':[found.group(1)], 'Longitude': [found.group(2)], 'Wi-Fi RSSI': [found.group(3)], 'LoRa RSSI': [found.group(4)], 'LoRa SNR': [found.group(5)]}
                    self.df = pd.concat([self.df, pd.DataFrame(dict)], ignore_index=True)
                    self.df.to_csv(self.csv_path, index=True)

                fin_str = found.group(1) + ',' + found.group(2) + ',' + found.group(3) + ',' + found.group(4)
                output_arr = fin_str.split(",")
                self.serial_output = ""

                if len(output_arr) > 1:
                    most_recent_x, most_recent_y = self.getRecentValues(output_arr)
                
                if len(output_arr) > 2 and ("nan" not in output_arr[WIFI_RSSI_1] or "nan" not in output_arr[WIFI_RSSI_2]):
                    if self.first_plot:
                        self.x = np.array([most_recent_x - self.bounds, most_recent_x + self.bounds])
                        self.y = np.array([most_recent_y - self.bounds, most_recent_y + self.bounds])
                        self.first_plot = False

                    extent_array = self.updateBounds(most_recent_x, most_recent_y)
                    is_duplicate = ((most_recent_x in self.x) or (most_recent_y in self.y))
                    rssi_avg = self.getRSSIAvg(output_arr)

                    if not is_duplicate:
                        self.addProportionalPoints(rssi_avg, most_recent_x, most_recent_y)
                    heatmap = self.getHeatMap()
        return extent_array, heatmap
                


    '''
    This will print the available ports on your computer. This is used when 
    running useSerial.
    '''
    def printPorts(self):
        ports = serial.tools.list_ports.comports()
        for port, desc, hwid in sorted(ports):
            print("{}: {} [{}]".format(port, desc, hwid))


