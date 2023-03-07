import numpy as np
import serial.tools.list_ports
import serial
import csv
from scipy.ndimage import gaussian_filter
import ee
import requests
import shutil
import cv2 as cv
from matplotlib import pyplot as plt
import re
import os

LATITUDE = 0
LONGITUDE = 1
WIFI_RSSI_1 = 2
WIFI_RSSI_2 = 3
RSSI = 4
SNR = 5
PACKET_COUNT = 6

# NAIP is only valid in the contiguous US
NAIP_COLLECTION = "USDA/NAIP/DOQQ"
SERIAL_PORT = 'COM6' #### Change this according to printPorts
HEADER = ["Coordinates", "Wi-FI RSSI", "LoRa RSSI", "LoRa SNR"]


class HeatMap():
    def __init__(self, csv_file_path=None, read_from_csv=False):
        # Init Google Earth Engine (GEE). 
        # Note, you will need to have a project configured with Google Cloud Platform to use GEE.
        # try:
        #     ee.Initialize()
        # except:
        # ee.Authenticate()
        # ee.Initialize()

        self.sigma = 48
        self.location_x = []
        self.location_y = []
        self.first_plot = True
        self.bounds = 0.00045
        self.x = None
        self.y = None
        self.is_done = False
        # The groups, i.e. the stuff in the parentheses, are as follows: coordinates, Wi-Fi RSSI, LoRa RSSI, and SNR in dB
        self.re_received_data = re.compile(r".\*(-*\d*,-*\d*),(-*\d*.\d*),(-*\d*.\d*),(-*\d*.\d*,)")
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
            if os.path.exists(csv_file_path) == False:
                with open(csv_file_path, "w") as f:
                    writer = csv.writer(f)
                    writer.writerow(HEADER)

    '''
    This is the function to call when using a CSV file to read the data 
    from a csv (see csv files under heatmap_visualization).
    '''
    def useCSV(self):
        # entries = 0
        # unique_pts = 0
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
                    # unique_pts += 1
                    # entries += 1
                    self.addProportionalPoints(rssi_avg, most_recent_x, most_recent_y)
                heatmap = self.getHeatMap()
                # overlay_img = self.getImage(extent_array, heatmap.shape)
                # test = cv.addWeighted(heatmap, 0.5, overlay_img, 0.5, 0)
                # cv.imshow("test img", test)
                # cv.waitKey(0)
                # cv.de1stroyAllWindows()
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
        # unique_pts = 0
        # entries = 0
        extent_array = None
        heatmap = None

        line = ""
        
        try:
            bytes_to_read = self.ser.in_waiting
        except IOError:
            raise IOError()
        
        if bytes_to_read:
            self.serial_output = self.serial_output + self.ser.read(bytes_to_read).decode("utf-8")
            found = self.re_received_data.search(self.serial_output)

            if found != None:
                # This means we want to write the data to a csv for later use
                if self.csv_path is not None:
                    with open(self.csv_path, "a") as f:
                        writer = csv.writer(f)
                        writer.writerow([found.group(1), found.group(2), found.group(3), found.group(4)])

                fin_str = found.group(1) + ',' + found.group(2) + ',' + found.group(3) + ',' + found.group(4)
                output_arr = fin_str.split(",")
                self.serial_output = "" # I think I need to update this here

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
                        # unique_pts += 1
                        # entries += 1
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


    ''' 
    Extent passed in from HeatMap class is like this [40.2455852, 40.2464852, -111.640322, -111.639422], which is [xMin, xMax, yMin, yMax].
    Dimension is just the dimension of the heatmap. This will pull an image from NAIP within the bounds of the extent and it will be the 
    same size as heatmap.
    '''
    def getImage(extent, dimension):
        # This just reorders the extent into the format GEE wants is
        extent = [extent[2], extent[0], extent[3], extent[1]]
        # Below creates our region of interest from which we will pull the photo from ee
        roi = ee.Geometry.Rectangle(extent).bounds()

        # This gets images from a specific collection.
        data = ee.ImageCollection(NAIP_COLLECTION).filter(ee.Filter.date('2017-01-01', '2022-12-31'))

        # Get mosaic from the image collection
        mosaic = data.mosaic()
        mos_bands = mosaic.bandNames().getInfo()
        # Grab just the area we want
        mos_url = mosaic.getThumbURL({'region': roi, 'dimensions': dimension, 'bands': mos_bands[0:3], 'format': 'png'})
        mos_req = requests.get(mos_url, stream=True)

        mos_img_filename = 'mos_aerial_photo.png'
        with open(mos_img_filename, 'wb') as mos_out_file:
            shutil.copyfileobj(mos_req.raw, mos_out_file)

        return cv.imread(mos_img_filename, cv.IMREAD_GRAYSCALE)
