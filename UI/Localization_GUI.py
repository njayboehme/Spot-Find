import numpy as np
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.figure import Figure
from matplotlib.offsetbox import OffsetImage, AnnotationBbox
from matplotlib import style
import matplotlib.cm as cm
import sys, os
sys.path.append(os.getcwd()) # This adds Spot-Find to the path which will let us import the HeatMap Class
from heatmap_visualization.HeatMap import HeatMap

R_D = "Run Diagnostics"
EXE = "Display Heat Map"
EXIT = "Exit"
INIT_TITLE = "Signal Localization Start-Up"
HEAT_MAP_TITLE = "Heat Map"
CSV_READ_FILE = r"C:\\Users\\njboe\Desktop\\Capstone\Spot-Find\\heatmap_visualization\data_points_2022-03-28 09_30_26.258499.csv"

# Used this tutorial: 
# https://pythonprogramming.net/embedding-live-matplotlib-graph-tkinter-gui/
class LocalizationGUI(tk.Tk):
    def __init__(self, *args, **kwargs):
        tk.Tk.__init__(self, *args, **kwargs)
        tk.Tk.wm_title(self, INIT_TITLE)

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        self.frames = {}

        for F in (StartPage, DiagnosticsPage, HeatMapPage):
            frame = F(container, self)
            self.frames[F] = frame
            frame.grid(row=0, column=0, sticky="nsew")
        
        self.showFrame(StartPage)
    
    def showFrame(self, cont):
        frame = self.frames[cont]
        frame.tkraise()


class StartPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = tk.Label(self)
        label.pack(pady=10, padx=10)

        diagnosticsButton = ttk.Button(self, text=R_D, command=lambda: controller.showFrame(DiagnosticsPage))
        diagnosticsButton.pack(fill='x')

        heatMapButton = ttk.Button(self, text=EXE, command=lambda: controller.showFrame(HeatMapPage))
        heatMapButton.pack(fill='x')
        
        exitButton = ttk.Button(self, text=EXIT, command=controller.destroy)
        exitButton.pack(fill='x')
    

class DiagnosticsPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)

        button = ttk.Button(self, text='Back to Homepage', command=lambda: controller.showFrame(StartPage))
        button.pack(side='bottom')


class HeatMapPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = tk.Label(self, text=HEAT_MAP_TITLE)
        label.pack(pady=10, padx=10, side='top')

        self.heatMap = HeatMap(CSV_READ_FILE)

        self.fig, self.axes = plt.subplots()
        self.axes.set_xlabel("Longitude")
        self.axes.set_ylabel("Latitude")

        # x and y data to plot
        self.x_data = []
        self.y_data = []

        self.canvas = FigureCanvasTkAgg(self.fig, self)
        toolbar = NavigationToolbar2Tk(self.canvas, self)
        toolbar.update()

        button = ttk.Button(self, text='Back to Homepage', command=lambda: controller.showFrame(StartPage))
        button.pack(side='bottom')
        self.canvas.get_tk_widget().pack(side='top', fill=tk.BOTH, expand=True)
        self.animate()
    
    def animate(self):
        extent_array, heatmap = self.heatMap.useCSV()

        # If we have already plotted the first point we will remove the last point, 
        # redraw that point as a grey x and then draw the newest point as a green dot
        if len(self.heatMap.location_x) > 1:
            self.prev_point[0].remove()
            self.axes.plot(self.heatMap.location_x[-2], self.heatMap.location_y[-2], 'x', color=(0.9, 0.9, 1.0), alpha=0.8)
        self.prev_point = self.axes.plot(self.heatMap.location_x[-1], self.heatMap.location_y[-1], 'o', color='green')
        
        # If we have a new heatmap/extent_array to show
        if extent_array is not None:
            plt.imshow(heatmap, extent=extent_array, origin='lower', cmap=cm.jet)
            plt.xlim([extent_array[0], extent_array[1]])
            plt.ylim([extent_array[2], extent_array[3]])
            
        self.canvas.draw_idle()
        self.after(500, self.animate)

app = LocalizationGUI()
# app.attributes('-fullscreen', True)
app.mainloop()
    
