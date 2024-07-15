import pandas as pd
import matplotlib.pyplot as plt
from datetime import timedelta

# CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
df = pd.read_csv('datalog74.csv', delimiter=';')

# Zeitreihe erstellen, alle 10 Sekunden ein neuer Wert
df['Minutes'] = [10 * i / 60 for i in range(len(df))]

# Filter, um nur die ersten 4 Stunden zu verwenden
end_time = 240  # 240 Minuten = 4 Stunden
df = df[df['Minutes'] <= end_time]

# Diagramm erstellen
fig, ax1 = plt.subplots(figsize=(15, 6))

# Temperatur (Temp) plotten
temp_line, = ax1.plot(df['Minutes'], df['Temp'], 'r-', label='Temperature (°C)')
ax1.set_xlabel('Zeit (Minuten)')
ax1.set_ylabel('Temperatur (°C)', color='black')
ax1.tick_params(axis='y', labelcolor='r')
ax1.grid(True)  # Gitter hinzufügen
ax1.set_ylim(22, 24)

# Luftfeuchtigkeit (Hum) plotten
ax2 = ax1.twinx()
hum_line, = ax2.plot(df['Minutes'], df['Hum'], 'b-', label='Humidity (%)')
ax2.set_ylabel('relative Luftfeuchtigkeit (%)', color='black')
ax2.tick_params(axis='y', labelcolor='b')
ax2.grid(False)  # Sicherstellen, dass das Gitter nur auf der Hauptachse ist
ax2.set_ylim(0, 100)

# Feinstaubwerte (PM10) plotten
ax3 = ax1.twinx()
ax3.spines['right'].set_position(('outward', 60))  # Position der dritten Y-Achse anpassen
pm10_line, = ax3.plot(df['Minutes'], df['PM10'], 'm-', label='PM10 (µg/m³)')
ax3.set_ylabel('PM10 (µg/m³)', color='black')
ax3.tick_params(axis='y', labelcolor='m')
ax3.grid(False)
ax3.set_ylim(0, 10)

# Titel hinzufügen
plt.title('Feinstaub, Temperatur und Luftfeuchtigkeit über die Zeit')

# X-Achsenlimits festlegen
ax1.set_xlim([0, end_time])  # 4 Stunden = 240 Minuten

# X-Achse in 30-Minuten-Intervallen beschriften
ax1.xaxis.set_major_locator(plt.MultipleLocator(30))

# Legende kombinieren und hinzufügen
lines = [temp_line, hum_line, pm10_line]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left', bbox_to_anchor=(0, 1))

# Diagramm anzeigen
plt.show()

# Diagramm speichern
fig.savefig('pm10_temp_hum_diagram.png')

print("Diagramm wurde erfolgreich erstellt und gespeichert.")
