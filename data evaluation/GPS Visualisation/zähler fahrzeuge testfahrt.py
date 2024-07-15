import pandas as pd
import folium

# CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
df = pd.read_csv('datalog40.csv', delimiter=';')

# Funktion zum Berechnen des Mittelwerts der nächsten 5 Werte
def calculate_mean_pm_values(index, df):
    start_index = max(0, index - 2)
    end_index = min(len(df), index + 3)
    subset = df.iloc[start_index:end_index]
    mean_pm1 = subset['PM1'].mean()
    mean_pm25 = subset['PM25'].mean()
    mean_pm4 = subset['PM4'].mean()
    mean_pm10 = subset['PM10'].mean()
    return mean_pm1, mean_pm25, mean_pm4, mean_pm10

# Karte erstellen, zentriert auf den Mittelwert der GPS-Koordinaten
map_osm = folium.Map(location=[df['Latitude'].mean(), df['Longitude'].mean()], zoom_start=14)

# GPS-Koordinaten extrahieren und Strecke hinzufügen
route = []
count_green = 0
count_orange = 0
count_red = 0

for index, row in df.iterrows():
    latitude = row['Latitude']
    longitude = row['Longitude']
    
    # Punkt zur Route hinzufügen
    route.append((latitude, longitude))
    
    # Marker für erkannte Fahrzeuge hinzufügen und Farbe nach Abstand
    if row['VehicleApproaching'] == 'Yes':
        distance = row['D2']
        try:
            distance = float(distance)
        except ValueError:
            print(f"Ungültiger Wert für Distance: {distance} bei {latitude}, {longitude}")
            continue
        
        mean_pm1, mean_pm25, mean_pm4, mean_pm10 = calculate_mean_pm_values(index, df)
        
        if distance > 150:
            color = 'green'
            count_green += 1
        elif 75 < distance <= 150:
            color = 'orange'
            count_orange += 1
        else:
            color = 'red'
            count_red += 1
        
        popup_text = (f"Fahrzeug erkannt bei {latitude}, {longitude} mit Abstand {distance} cm<br>"
                      f"PM1: {mean_pm1:.2f} µg/m³, PM2.5: {mean_pm25:.2f} µg/m³, PM4: {mean_pm4:.2f} µg/m³, PM10: {mean_pm10:.2f} µg/m³")
        
        folium.Marker(
            location=[latitude, longitude],
            popup=popup_text,
            icon=folium.Icon(color=color, icon='info-sign')
        ).add_to(map_osm)

# Route zur Karte hinzufügen
folium.PolyLine(route, color='blue', weight=5, opacity=0.7).add_to(map_osm)

# Karte speichern
map_osm.save("Vehicle_Route_Karte.html")

# Anzahl der überholenden Fahrzeuge ausgeben
print("Anzahl der überholenden Fahrzeuge:")
print(f"Grün (> 150 cm): {count_green}")
print(f"Orange (75-150 cm): {count_orange}")
print(f"Rot (<= 75 cm): {count_red}")

print("Karte wurde erfolgreich unter 'Vehicle_Route_Karte.html' gespeichert.")
