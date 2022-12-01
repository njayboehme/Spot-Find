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


class HeatMap():
    def __init__(self):
        self.entries = 0
        self.unique_pts = 0
        self.location_x = []
        self.location_y = []
        self.most_recent_x = 0
        self.most_recent_y = 0
        self.rssi_avg_value = 'nan'
        


    def useCSV(csv_file_path):
        pass


    def useSerial():
        pass


    def getDataPoints():
        pass

