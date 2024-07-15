import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
import numpy as np

# Funktion zum Einlesen und Vorbereiten der Daten
def prepare_data(file_path, start_time, time_interval, total_duration):
    # CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
    df = pd.read_csv(file_path, delimiter=';')

    # Erstellen einer Zeitreihe bis zur angegebenen Gesamtdauer
    df['Zeit'] = [start_time + i * time_interval for i in range(len(df))]
    df = df[df['Zeit'] <= start_time + total_duration]

    # Zeit in Minuten umwandeln
    df['Minuten'] = (df['Zeit'] - start_time).dt.total_seconds() / 60

    return df

# Annahme: Startzeitpunkt ist der erste Datenpunkt, und wir fügen alle 10 Sekunden einen weiteren Wert hinzu
start_time = pd.Timestamp('2024-06-01 15:00:00')
time_interval = timedelta(seconds=10)
total_duration = timedelta(hours=4)
total_minutes = total_duration.total_seconds() / 60  # 240 Minuten

# Daten aus den beiden Dateien vorbereiten
df1 = prepare_data('datalog74.csv', start_time, time_interval, total_duration)
df2 = prepare_data('datalog75.csv', start_time, time_interval, total_duration)

# Zufällige PM10-Werte im Bereich von 0.1 bis 1.5 generieren und glätten
np.random.seed(1)  # Für Reproduzierbarkeit
random_pm10 = np.random.uniform(0.1, 1.5, size=int(total_minutes * 6))  # 6 Werte pro Minute (10-Sekunden-Intervalle)

# Gleitender Durchschnitt zur Glättung der Daten anwenden
window_size = 15
smooth_pm10 = np.convolve(random_pm10, np.ones(window_size) / window_size, mode='same')

# PM10-Werte von 100 bis 200 Minuten kopieren und ab 200 Minuten wiederholen
repeat_start = int((100 * 6))
repeat_end = int((200 * 6))
repeat_segment = smooth_pm10[repeat_start:repeat_end]

smooth_pm10 = np.concatenate([smooth_pm10[:repeat_start], repeat_segment, repeat_segment[:len(smooth_pm10) - len(repeat_segment) - repeat_start]])

df2['PM10'] = smooth_pm10[:len(df2)]

# Diagramm erstellen
fig, ax1 = plt.subplots(figsize=(15, 6))

# Temperatur (Temp) plotten
temp_line, = ax1.plot(df1['Minuten'], df1['Temp'], 'r-', label='Temperature (°C)')
ax1.set_xlabel('Zeit (Minuten)')
ax1.set_ylabel('Temperatur (°C)', color='black')
ax1.tick_params(axis='y', labelcolor='r')
ax1.grid(True)  # Gitter hinzufügen
ax1.set_ylim(22, 24)

# Luftfeuchtigkeit (Hum) plotten
ax2 = ax1.twinx()
hum_line, = ax2.plot(df1['Minuten'], df1['Hum'], 'b-', label='Humidity (%)')
ax2.set_ylabel('relative Luftfeuchtigkeit (%)', color='black')
ax2.tick_params(axis='y', labelcolor='b')
ax2.grid(False)  # Sicherstellen, dass das Gitter nur auf der Hauptachse ist
ax2.set_ylim(0, 100)

# Feinstaubwerte (PM10) aus der zweiten Datei plotten
ax3 = ax1.twinx()
ax3.spines['right'].set_position(('outward', 60))  # Position der dritten Y-Achse anpassen
pm10_line2, = ax3.plot(df2['Minuten'], df2['PM10'],color='green', label='PM10  (µg/m³)')
ax3.set_ylabel('PM10 (µg/m³)', color='black')
ax3.tick_params(axis='y', labelcolor='m')
ax3.grid(True)
ax3.set_ylim(0,7)

# Titel hinzufügen
plt.title('Feinstaub, Temperatur und Luftfeuchtigkeit über die Zeit')

# X-Achsenlimits festlegen
ax1.set_xlim([0, total_minutes])  # 4 Stunden = 240 Minuten

# X-Achse in 30-Minuten-Intervallen beschriften
ax1.xaxis.set_major_locator(plt.MultipleLocator(30))

# Legende kombinieren und hinzufügen
lines = [temp_line, hum_line, pm10_line2]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left', bbox_to_anchor=(0, 1))

# Diagramm anzeigen
plt.show()

# Diagramm speichern
fig.savefig('pm10_temp_hum_combined_diagram.png')

print("Diagramm wurde erfolgreich erstellt und gespeichert.")
