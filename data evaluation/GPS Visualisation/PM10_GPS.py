import pandas as pd
import folium
from branca.colormap import LinearColormap

# CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
df = pd.read_csv('datalog40.csv', delimiter=';')

# Farbpalette für die PM10-Werte erstellen
pm10_values = df['PM10'].values
pm10_min = pm10_values.min()
pm10_max = pm10_values.max()
colormap = LinearColormap(
    ['green', 'yellow', 'orange', 'red'],
    vmin=0, vmax=40,
    caption='PM10 in µg/m³'
)

# Karte erstellen, zentriert auf den Mittelwert der GPS-Koordinaten
map_osm = folium.Map(location=[df['Latitude'].mean(), df['Longitude'].mean()], zoom_start=10)

# GPS-Koordinaten extrahieren und Punkte mit PM10-Werten hinzufügen
for index, row in df.iterrows():
    pm10 = row['PM10']
    color = colormap(pm10)
    folium.CircleMarker(
        location=[row['Latitude'], row['Longitude']],
        radius=7,
        color=None,  # Kein Rand
        fill=True,
        fill_color=color,
        fill_opacity=0.9,
        weight=0  # Gewicht des Randes auf 0 setzen
    ).add_to(map_osm)

# Farbskala zur Karte hinzufügen
colormap.add_to(map_osm)

# Karte speichern
map_osm.save("PM10_Karte.html")

print("Karte wurde erfolgreich unter 'PM10_Karte.html' gespeichert.")
