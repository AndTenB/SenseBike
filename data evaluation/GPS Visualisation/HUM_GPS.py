import pandas as pd
import folium
from folium.plugins import HeatMap
from branca.colormap import linear

# CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
df = pd.read_csv('datalog40.csv', delimiter=';')

# Farbpalette für die Luftfeuchtigkeit erstellen
humidity_values = df['Hum'].values
humidity_min = humidity_values.min()
humidity_max = humidity_values.max()
colormap = linear.YlGnBu_09.scale(humidity_min, humidity_max)
colormap.caption = 'Luftfeuchtigkeit in %'

# Karte erstellen, zentriert auf den Mittelwert der GPS-Koordinaten
map_osm = folium.Map(location=[df['Latitude'].mean(), df['Longitude'].mean()], zoom_start=10)

# GPS-Koordinaten extrahieren und Punkte mit Luftfeuchtigkeitswerten hinzufügen
for index, row in df.iterrows():
    humidity = row['Hum']
    color = colormap(humidity)
    folium.CircleMarker(
        location=[row['Latitude'], row['Longitude']],
        radius=7,
        color='black',  # Kein Rand
        fill=True,
        fill_color=color,
        fill_opacity=0.9,
        weight=0.3  # Gewicht des Randes auf 0 setzen
    ).add_to(map_osm)

# Farbskala zur Karte hinzufügen
colormap.add_to(map_osm)

# Karte speichern
map_osm.save("Hum_Karte.html")

print("Karte wurde erfolgreich unter 'Hum_Karte.html' gespeichert.")
