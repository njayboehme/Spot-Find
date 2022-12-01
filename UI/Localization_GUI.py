import numpy as np
import tkinter as tk
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
from matplotlib.figure import Figure
from matplotlib import style
import sys, os
sys.path.append(os.path.abspath(os.getcwd())) # This adds Spot-Find to the path which will let us import the HeatMap Class
from heatmap_visualization.HeatMap import HeatMap

R_D = "Run Diagnostics"
EXE = "Execute"
EXIT = "Exit"
INIT_TITLE = "Signal Localization Start-Up"
HEAT_MAP_TITLE = "Heat Map"

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

        heatMapButton = ttk.Button(self, text="Execute Heat Map", command=lambda: controller.showFrame(HeatMapPage))
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

        # Matplot Figure
        self.figure = Figure(figsize=(5, 5), dpi=100)
        self.ax = self.figure.add_subplot(111)

        # x and y data to plot
        self.x_data = [np.random.random()]
        self.y_data = [np.random.random()]

        # Create plot
        self.plot = self.ax.plot(self.x_data, self.y_data)[0]

        self.canvas = FigureCanvasTkAgg(self.figure, self)
        toolbar = NavigationToolbar2Tk(self.canvas, self)
        toolbar.update()

        button = ttk.Button(self, text='Back to Homepage', command=lambda: controller.showFrame(StartPage))
        button.pack(side='bottom')
        self.canvas.get_tk_widget().pack(side='top', fill=tk.BOTH, expand=True)
        self.animate()
    
    def animate(self):
        self.x_data.append(np.random.random())
        self.y_data.append(np.random.random())
        self.plot.set_xdata(self.x_data)
        self.plot.set_ydata(self.y_data)
        self.ax.set_xlim(min(self.x_data), max(self.x_data))
        self.ax.set_ylim(min(self.y_data), max(self.y_data))
        self.canvas.draw_idle()
        self.after(2000, self.animate)

app = LocalizationGUI()
app.attributes('-fullscreen', True)
app.mainloop()

