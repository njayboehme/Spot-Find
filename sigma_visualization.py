import time, serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import decimal
import random
import numpy as np
import matplotlib.cm as cm
from scipy.ndimage.filters import gaussian_filter


first_coordinate = True
x_pts = []
y_pts = []
signal_strength = []

def generate_random_data(datapts):
    for i in range(0, datapts):
        x = float(decimal.Decimal(random.randrange(40240000, 40260000)) / 1000000)
        y = float(decimal.Decimal(random.randrange(111585000, 111605000)) / 1000000)
        #signal = float(decimal.Decimal(random.randrange(0, 12000)) / 100)
        signal = ((x + 40.2450) + (y - 111.5950)) * 1000
        print(signal)
        x_pts.append(x)
        y_pts.append(y * -1)

        signal_strength.append(signal)


def print_ports():
    ports = serial.tools.list_ports.comports()
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))

def read_serial():
    time.sleep(3)
    ser = serial.Serial('/dev/cu.usbserial-1420', 9600) ### change port number as needed
    # Infinite loop to read data back:
    line = ""
    while True:
        try:
            # get the amount of bytes available at the input queue
            bytes_to_read = ser.inWaiting()
        except IOError:
            raise IOError()
        if bytes_to_read:
            # read the bytes and print it out:
            line = line + ser.read(bytes_to_read).decode("utf-8")
            #print(line.strip())
            print(line)
            if "\r\n" in line:
                add_data(line)
                line = ""

def convert_lat(latitude):
    dd = float(latitude[0:2])
    mm = float(latitude[2:])
    return dd + (mm / 60)

def convert_long(longitude):
    dd = float(longitude[0:3])
    mm = float(longitude[3:])
    return dd + (mm / 60)


### receives data in the format of RSSI value [space] SNR value [space] latitude [space] longitude\r\n
def add_data(data_unit):
    values = data_unit.split() #by default splits by space
    rssi = float(values[0])
    snr = float(values[1])
    latitude = convert_lat(values[2])
    longitude = convert_long(values[3])
    plot(rssi, snr, latitude, longitude)


def plot(rssi, snr, lat, long, first_coordinate):
    if first_coordinate:
        # plt.xlim([40.250000, 40.260000])
        # plt.ylim([-111.600000, -111.700000])
        lat_min = lat - 0.01
        lat_max = lat + 0.01
        long_min = long - 0.01
        long_max = long + 0.01

        plt.xlim([lat_min, lat_max])
        plt.ylim([long_min, long_max])
        first_coordinate = False

    x_pts.append(lat)
    y_pts.append(long)
    signal_strength.append(rssi)


def myplot(x, y, s, bins=1000):
    heatmap, xedges, yedges = np.histogram2d(x, y, bins=bins)
    heatmap = gaussian_filter(heatmap, sigma=s)

    extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]
    return heatmap.T, extent

if __name__ == "__main__":
    # generate_random_data(500)
    # plot(-56, 10.5, 40.250000, -111.595000, True)
    # # plot(-90, 10.5, 40.257500, -111.602000, False)
    # # plot(-27, 10.5, 40.257500, -111.587500, False)
    # plt.scatter(x_pts, y_pts, c=signal_strength, cmap='seismic')
    # plt.show()
    #read_serial()

    fig, axs = plt.subplots(2, 2, num="Smoothing GPS data visualization")

    # Generate some test data
    x = 0.00005 * np.random.randn(1000) + 40.240000
    y = 0.00005 * np.random.randn(1000) - 111.585000

    sigmas = [0, 16, 32, 64]

    for ax, s in zip(axs.flatten(), sigmas):
        ax.ticklabel_format(useOffset=False, style='plain')
        ax.set_xlabel("Latitude", fontsize=10)
        ax.set_ylabel("Longitude", fontsize=10)
        if s == 0:
            ax.plot(x, y, 'k.', markersize=5)
            ax.set_title("Random GPS coordinates")
        else:
            img, extent = myplot(x, y, s)
            ax.imshow(img, extent=extent, origin='lower', cmap=cm.jet)
            ax.set_title("Smoothing with  $\sigma$ = %d" % s, fontsize=11)
    fig.tight_layout()

    plt.show()


'''
[SX1262] Waiting for incoming transmission ... success!
[SX1262] Data:		Hello Area 28! 308
[SX1262] RSSI:		-56.00 dBm
[SX1262] SNR:		10.50 dB

b'success!\r\n[SX1262] Data:\t\tHello'
b'Area 28! 2974\r\n[SX1262] RSSI:\t\t-'
b'63.00 dBm\r\n[SX1262] SNR:\t\t10.75'
b'dB\r\n[SX1262] Waiting for incomin'

'''