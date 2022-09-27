import datetime

import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
import serial.tools.list_ports
import serial
import time
import random

from matplotlib.patches import Circle
from scipy.ndimage.filters import gaussian_filter
import matplotlib.ticker as mtick
from enum import IntEnum
import csv
import datetime as dt

class Value(IntEnum):
    latitude = 0
    longitude = 1
    wifiRSSI_1 = 2
    wifiRSSI_2 = 3
    RSSI = 4
    SNR = 5
    packet_count = 6

class Bounds(IntEnum):
    x_min = 0
    x_max = 1
    y_min = 2
    y_max = 3

#fig, ax = plt.subplots()

def use_csv_lines(file_name):
    with open(file_name, mode='r') as file:
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


def print_ports():
    ports = serial.tools.list_ports.comports()
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))

def read_serial():
    plt.subplots(figsize=(9.5, 7.5), sharex='all', sharey='all')
    fields = ["latitude", "longitude", "Wi-Fi RSSI", "RSSI", "SNR", "packet_num"]
    filename = "data_points_" + str(dt.datetime.now()) + ".csv"
    print("Filename:", filename)
    entries = 0
    unique_pts = 0
    location_x = []
    location_y = []
    most_recent_x = 0
    most_recent_y = 0
    rssi_avg_value = "nan"

    ### if you decide to use csv data to input,
    # change the file name for use_csv_lines to the file of data you want to use,
    # and change use_csv_data to True
    use_csv_data = False
    csv_data = use_csv_lines("data_points_2022-03-15 09:48:07.083579.csv")

    # opens a csv file to write to
    with open(filename, "w") as csvfile:
        csvwriter = csv.writer(csvfile)

        # the sleep is necessary so that the serial monitor has time to be read
        time.sleep(3)

        # reads the serial monitor, change the port that it reads from as needed
        # based on the ports outputted from print_ports, change the port number
        ser = serial.Serial("/dev/cu.usbserial-1420", 9600) ### change port number as needed

        line = ""

        ### determines the detail with which the points are plotted - 16-64 are good values to test out
        sigma = 48
        first_plot = True
        extent_arr = []

        # Infinite loop to read data back:
        while True:
            try:
                start_time = time.time()
                # gets the bytes available at the input queue
                bytes_to_read = ser.inWaiting()
            except IOError:
                raise IOError()
            if bytes_to_read or use_csv_data:
                # read the bytes and print it out:
                line = line + ser.read(bytes_to_read).decode("utf-8")
                if use_csv_data:
                    if len(csv_data) == 0:
                        print("Data finished")
                        exit()
                    line = csv_data.pop()
                print(line)
                # * indicates the beginning of a data line, and \r\n indicates the end
                # this if statement indicates we have at least one data packet to process
                if "\r\n" in line and "*" in line:
                    end_time = time.time()
                    output = line[line.index("*") + 1:line.index("\r\n")]
                    output_arr = output.split(",")

                    print("output array:", output_arr)
                    # length greater than 2 means we have a GPS coordinate to process
                    if len(output_arr) >= 2:
                        most_recent_x = float(output_arr[Value.latitude]) / 10000000
                        most_recent_y = float(output_arr[Value.longitude]) / 10000000
                        location_x.append(most_recent_x)
                        location_y.append(most_recent_y)
                        output_arr[Value.latitude] = most_recent_x
                        output_arr[Value.longitude] = most_recent_y
                        output_arr.append(datetime.datetime.now())
                        ### TODO: FIX CSV
                        csvwriter.writerow(output_arr) # writes datapoint to file
                    line = ""

                    ### checks if there are valid RSSI values
                    if len(output_arr) >= 3 and ("nan" not in output_arr[Value.wifiRSSI_1] or "nan" not in output_arr[Value.wifiRSSI_2]):
                        print("wifi rssi is:", output_arr[Value.wifiRSSI_1], output_arr[Value.wifiRSSI_2])
                        if first_plot:
                            print("Starting latitude and longitude values:", most_recent_x, most_recent_y)
                            # bounds value can be changed based on the area you're searching;
                            # since our max range is only about 70-100, +- 0.00035 degrees of lat/long works well for us
                            bounds = 0.00045
                            x = np.array([most_recent_x - bounds, most_recent_x + bounds])
                            y = np.array([most_recent_y - bounds, most_recent_y + bounds])
                            first_plot = False

                        # expands the heatmap's bounds if a new point is out of bounds
                        # new error checking to make sure bounds don't go way out of range
                        if min(x) > most_recent_x and abs(most_recent_x - min(x)) < 1:
                            x = np.append(x, most_recent_x - bounds)
                            y = np.append(y, most_recent_y)
                        elif most_recent_x > max(x) and abs(most_recent_x - max(x)) < 1:
                            x = np.append(x, most_recent_x + bounds)
                            y = np.append(y, most_recent_y)
                        if min(y) > most_recent_y and abs(most_recent_y - min(y)) < 1:
                            y = np.append(y, most_recent_y - bounds)
                            x = np.append(x, most_recent_x)
                        elif most_recent_y > max(y) and abs(most_recent_y - max(x)) < 1:
                            y = np.append(y, most_recent_y + bounds)
                            x = np.append(x, most_recent_x)

                        duplicate = ((most_recent_x in x) or (most_recent_y in y))
                        print("DUPLICATE:", duplicate)

                        # converts the RSSI value to a float if it's valid
                        rssi_avgs = []
                        if output_arr[Value.wifiRSSI_1] != "nan":
                            rssi_1 = float(output_arr[Value.wifiRSSI_1])
                            rssi_avgs.append(rssi_1)

                        if output_arr[Value.wifiRSSI_2] != "nan":
                            rssi_2 = float(output_arr[Value.wifiRSSI_2])
                            rssi_avgs.append(rssi_2)


                        rssi_avg_value = sum(rssi_avgs) / len(rssi_avgs)
                        # only adds points to the heatmap that aren't a GPS duplicate
                        if not duplicate:
                            unique_pts = unique_pts + 1
                            entries = entries + 1

                            # the number of points added to the heatmap is proportional to the strength of the average RSSI value
                            num_pts = 0.4 * 1.75 ** (0.165 * (120 + rssi_avg_value))
                            print("Num pts to add:", int(num_pts))
                            #TODO: check this
                            for i in range(int(num_pts)): # adds more data points proportional to strength of RSSI
                                x = np.append(x, most_recent_x)
                                y = np.append(y, most_recent_y)
                        img, extent_arr = myplot(x, y, sigma)
                        # cmap stands for color map

                        ### TODO: include error checking for RSSI values when using that
                        plt.close()
                        title_str = "Heatmap Data Visualization with Sigma = " + str(sigma)
                        plt.title(title_str)
                        plt.figure(figsize=(9.5,7.5))
                        plt.xlabel("Longitude")
                        plt.ylabel("Latitude")
                        print("Extent array bounds:", extent_arr)
                        plt.imshow(img, extent=extent_arr, origin='lower', cmap=cm.jet)
                        plt.plot(most_recent_x, most_recent_y, 'o', color='green')
                        plt.plot(location_x, location_y, 'x', color=(0.9, 0.9, 1.0), alpha=0.8)
                        plt.xlim([extent_arr[0], extent_arr[1]])
                        plt.ylim([extent_arr[2], extent_arr[3]])

                        print("$$$$$$$$$$$ GPS coordinate $$$$$$$$$$$$$$$$")
                        print("Latitude:", most_recent_x, "Longitude:", most_recent_y)
                        # printing data
                        print("********New transmission********")
                        print("Latitude:", output_arr[Value.latitude])
                        print("Longitude:", output_arr[Value.longitude])
                        print("Time:", end_time - start_time)
                        print("Wi-Fi RSSI 1:", output_arr[Value.wifiRSSI_1])
                        print("Wi-Fi RSSI 2:", output_arr[Value.wifiRSSI_2])
                        print("Average Wi-Fi RSSI:", rssi_avg_value)
                        print("LoRa RSSI:", output_arr[Value.RSSI])
                        print("Packet count:", output_arr[Value.packet_count])
                        print("RSSI:", output_arr[Value.RSSI])
                        print("SNR:", output_arr[Value.SNR])
                        print("Total number of unique pts:", unique_pts)
                        print("\n")
                        if entries > 10:
                            plt.savefig("plot_" + str(dt.datetime.now()) + ".png")
                            entries = 0

                        plt.pause(0.01)
                    else:
                        print("Time:", end_time - start_time)
                        print("Invalid values")

def myplot(x, y, s, bins=1000):
    heatmap, xedges, yedges = np.histogram2d(x, y, bins=bins)
    heatmap = gaussian_filter(heatmap, sigma=s)
    extent_array = [min(x), max(x), min(y), max(y)]
    return heatmap.T, extent_array

def plot_csv(filename, sigma):
    plt.subplots(figsize=(9.5, 7.5), sharex='all', sharey='all')
    first_plot = True
    print("Plotting from a csv file")
    with open(filename, mode="r") as file:
        csvfile = csv.reader(file)
        next(file)
        location_x = []
        location_y = []

        for line in csvfile:
            if first_plot:
                print("Starting latitude and longitude values:", line[Value.latitude],
                      line[Value.longitude])
                start_lat = float(line[Value.latitude])
                start_long = float(line[Value.longitude])
                bounds = 0.00050
                x = np.array([start_lat - bounds, start_lat + bounds])
                y = np.array([start_long - bounds, start_long + bounds])
                first_plot = False
            if len(line) >= 3:
                location_x.append(float(line[Value.latitude]))
                location_y.append(float(line[Value.longitude]))
            if len(line) >= 3 and "nan" not in line[Value.wifiRSSI] and float(line[Value.latitude]) not in x and float(line[Value.longitude]) not in y:
                print("new lat and long:", line[Value.latitude], line[Value.longitude])
                #print("valid line:", line)
                num_pts = 0.4 * 3 ** (.135 * (120 + int(float(line[Value.wifiRSSI]))))
                print("num_pts:", num_pts)
                if num_pts > 10000: # capping at 10000 points, since Wi-Fi RSSI values rare
                    num_pts = 10000
                more_x = float(line[Value.latitude])
                more_y = float(line[Value.longitude])

                if min(x) > more_x:
                    x = np.append(x, more_x - bounds)
                    y = np.append(y, more_y)
                elif more_x > max(x):
                    x = np.append(x, more_x + bounds)
                    y = np.append(y, more_y)
                if min(y) > more_y:
                    y = np.append(y, more_y - bounds)
                    x = np.append(x, more_x)
                elif more_y > max(y):
                    y = np.append(y, more_y + bounds)
                    x = np.append(x, more_x)

                for i in range(int(num_pts)):  # adds more data points proportional to strength of RSSI
                    x = np.append(x, more_x)
                    y = np.append(y, more_y)

        img, extent = myplot(x, y, sigma)
        # ax.yaxis.set_major_formatter(mtick.FormatStrFormatter('%.2e'))
        # ax.xaxis.set_major_formatter(mtick.FormatStrFormatter('%.2e'))

        title_str = "Heatmap Data Visualization with Sigma = " + str(sigma)
        plt.title(title_str)
        plt.xlabel("Latitude")
        plt.ylabel("Longitude")
        plt.imshow(img, extent=extent, origin='lower', cmap=cm.jet)

        print("location_x:", location_x)
        print("location_y:", location_y)
        plt.plot(location_x, location_y, 'x', color=(0.9, 0.9, 1.0), alpha=0.8)
        plt.xlim([extent[0], extent[1]])
        plt.ylim([extent[2], extent[3]])
        plt.show()

if __name__ == "__main__":
    print_ports()
    # use_csv_lines("data_points_2022-03-15 09:48:07.083579.csv")
    read_serial()
    #print(np.random.random((100, 100)))
    #plot_csv("data_points_2022-03-15 09:48:07.083579.csv", 48)

    ### uncomment read_serial for real-time testing, or the plot_csv line with the correct csv file to plot a heatmap of data points



