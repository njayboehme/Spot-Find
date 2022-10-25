import matplotlib.pyplot as plt
import numpy as np
import matplotlib.cm as cm
from scipy.ndimage.filters import gaussian_filter

# fig = plt.figure(figsize=(11.69,8.27))
# ax = fig.gca()
fig, ax = plt.subplots()
# ax.set_xlim(40.235000, 40.245000)
# ax.set_ylim(-111.583500, -111.584500)


def myplot(x, y, s, first_plot, bins=1000):
    heatmap, xedges, yedges = np.histogram2d(x, y, bins=bins)
    heatmap = gaussian_filter(heatmap, sigma=s)
    extent = []
    if first_plot:
        extent = [xedges[0], xedges[-1], yedges[0], yedges[-1]]
    # fig.canvas.draw_idle()
    # fig.canvas.flush_events()
    return heatmap.T, extent


if __name__ == "__main__":
    # Generate some test data
    x = 0.00005 * np.random.randn(1000) + 40.240000
    y = 0.00005 * np.random.randn(1000) - 111.585000

    # sigma could be 16, 32, or 64
    # img, extent = myplot(x, y, sigma)
    #
    # # cmap stands for color map
    # ax.imshow(img, extent=extent, origin='lower', cmap=cm.jet)
    # ax.set_title("Smoothing with sigma = %d" % 64, fontsize=11)
    # plt.show()
    sigma = 64
    count = 0
    first_plot = True
    extent_arr = []
    while count < 50:
        more_x = 0.00008 * np.random.randn(100) + 40.241000
        more_y = 0.00008 * np.random.randn(100) - 111.586000
        x = np.append(x, more_x)
        y = np.append(y, more_y)
        img, extent = myplot(x, y, sigma, first_plot)
        if first_plot:
            extent_arr = extent
            first_plot = False

        # cmap stands for color map
        ax.imshow(img, extent=extent_arr, origin='lower', cmap=cm.jet)
        ax.set_title("Smoothing with sigma = %d" % 64, fontsize=11)
        plt.imshow(img, extent=extent_arr, origin='lower', cmap=cm.jet)
        plt.pause(0.05)
        count += 1

