import pandas as pd
import folium
from folium.plugins import HeatMap
from branca.colormap import LinearColormap

# CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
df = pd.read_csv('datalog40.csv', delimiter=';')

# Farben für die Temperaturwerte definieren
colormap = LinearColormap(
    colors=['darkblue', 'blue', 'green', 'yellow', 'orange', 'red'],
    index=[-5, 0, 10, 20, 35, 40],
    vmin=20,
    vmax=40,
    caption='Temperatur in ° Celsius'
)

# Karte erstellen, zentriert auf den Mittelwert der GPS-Koordinaten
map_osm = folium.Map(location=[df['Latitude'].mean(), df['Longitude'].mean()], zoom_start=10)

# GPS-Koordinaten extrahieren und Punkte mit Temperaturwerten hinzufügen
for index, row in df.iterrows():
    temperature = row['Temp']
    color = colormap(temperature)
    folium.CircleMarker(
        location=[row['Latitude'], row['Longitude']],
        radius=7,
        color='white',
        fill=True,
        fill_color=color,
        fill_opacity=0.9,
        weight=0.5
    ).add_to(map_osm)

# Farbskala zur Karte hinzufügen
colormap.add_to(map_osm)

# Karte speichern
map_osm.save("Temp_Karte.html")

print("Karte wurde erfolgreich unter 'Temp_Karte.html' gespeichert.")
