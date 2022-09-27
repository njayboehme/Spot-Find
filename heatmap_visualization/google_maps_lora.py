import pandas as pd
import gmaps
import gmaps.datasets

### google maps section
gmaps.configure(api_key='AIzaSyBE_CHJPnOl4lfluoP3zJVyiKfAS9etF3M')  # configure key

# need to define project name as a file
lat_long_df = pd.read_csv('/Users/sarahquan/Em/BYU/BYU_2021_Fall/ECEn_475/Matplotlib_example/data_points.csv')

# remove null and duplicate values
lat_long_df = lat_long_df[~lat_long_df['latitude'].isnull()]
lat_long_df = lat_long_df[~lat_long_df['longitude'].isnull()]
lat_long_df = lat_long_df.drop_duplicates(['latitude', 'longitude'])
print(lat_long_df['latitude'], lat_long_df['longitude'])

# remove rows where temperature mean is < 0, since the heatmap doesn't take negative values
#lat_long_df = lat_long_df[lat_long_df['Arithmetic Mean'] >= 0]

locations = lat_long_df[['latitude', 'longitude']]
weights = pd.to_numeric(lat_long_df['Wi-Fi RSSI']).apply(lambda x: 1.9 * (x + 120))
print(weights)
print(locations)
heatmap_layer = gmaps.heatmap_layer(locations, weights=weights)
heatmap_layer.max_intensity = 100
heatmap_layer.point_radius = 15
# heatmap_layer.gradient = [
#     'silver',
#     'green',
#     'yellow'
# ]
fig = gmaps.figure()
fig.add_layer(heatmap_layer)
fig
