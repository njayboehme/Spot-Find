import datetime
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import serial.tools.list_ports
import serial
import time
from enum import IntEnum
import csv
import datetime as dt
from scipy.ndimage import gaussian_filter

LATITUDE = 0
LONGITUDE = 1
WIFI_RSSI_1 = 2
WIFI_RSSI_2 = 3
RSSI = 4
SNR = 5
PACKET_COUNT = 6

class HeatMap():
    def __init__(self, csv_file_path=None):
        self.sigma = 48
        self.location_x = []
        self.location_y = []
        self.first_plot = True
        self.bounds = 0.00045
        self.x = None
        self.y = None
        self.is_done = False
        if csv_file_path != None:
            self.csv_data = self.prepareCSV(csv_file_path)
            

    '''
    This is the function to call when using a CSV file to read the data 
    from (see csv files under heatmap_visualization).
    '''
    def useCSV(self):
        entries = 0
        unique_pts = 0
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
                    unique_pts += 1
                    entries += 1
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
        most_recent_x = float(output_arr[LATITUDE]) / 10000000
        most_recent_y = float(output_arr[LONGITUDE]) / 10000000
        self.location_x.append(most_recent_x)
        self.location_y.append(most_recent_y)
        output_arr[LATITUDE] = most_recent_x
        output_arr[LONGITUDE] = most_recent_y
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
    This creates the heatmap. Note, the transpose is returned to get the direction right
    '''
    def getHeatMap(self, num_bins=1000):
        heatmap, xedges, yedges = np.histogram2d(self.x, self.y, bins=num_bins)
        heatmap = gaussian_filter(heatmap, sigma=self.sigma)
        return heatmap.T


    '''
    '''
    def useSerial(self):
        pass

    '''
    '''
    def getDataPoints(self):
        pass

