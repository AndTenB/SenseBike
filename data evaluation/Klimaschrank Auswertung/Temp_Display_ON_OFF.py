import matplotlib.pyplot as plt
import pandas as pd
from datetime import timedelta

# Funktion zum Einlesen und Vorbereiten der Daten
def prepare_data(file_path, start_time, time_interval, total_duration):
    # CSV-Datei einlesen, Semikolon als Trennzeichen spezifizieren
    df = pd.read_csv(file_path, delimiter=';')

    # Erstellen einer Zeitreihe bis zur angegebenen Gesamtdauer
    df['Zeit'] = [start_time + i*time_interval for i in range(len(df))]
    df = df[df['Zeit'] <= start_time + total_duration]

    # Zeit in Minuten umwandeln
    df['Minuten'] = (df['Zeit'] - start_time).dt.total_seconds() / 60

    # Erstellen der zweiten Temperaturkurve
    second_temp = []
    for i in range(len(df)):
        minutes_elapsed = df.loc[i, 'Minuten']

        if minutes_elapsed <= 180:
            temp_increase = 5 + (minutes_elapsed // 30) * 5  # Anstieg um 5 Grad alle 30 Minuten, beginnend bei 5 Grad
        else:
            temp_increase = max(0, 30 - (minutes_elapsed - 180) / 60 * 30)  # Abfall nach 180 Minuten innerhalb einer Stunde

        second_temp.append(temp_increase)

    df['SecondTemp'] = second_temp

    # Abweichung berechnen
    df['Abweichung'] = df['Temp'] - df['SecondTemp']

    return df

# Annahme: Startzeitpunkt ist der erste Datenpunkt, und wir fügen alle 20 Sekunden einen weiteren Wert hinzu
start_time = pd.Timestamp('2024-06-01 12:00:00')
time_interval = timedelta(seconds=20)
total_duration = timedelta(hours=4)

# Daten aus den beiden Dateien vorbereiten
df1 = prepare_data('datalog4.csv', start_time, time_interval, total_duration)
df2 = prepare_data('datalog7.csv', start_time, time_interval, total_duration)

# Diagramm für Temperaturkurven erstellen
plt.figure(figsize=(15, 5))
plt.plot(df1['Minuten'], df1['Temp'], color='b',marker='o', markersize=2, linewidth=1, label='Temperatur Sensor Display ON')
plt.plot(df1['Minuten'], df1['SecondTemp'], color='r', marker='x', markersize=2, linestyle='--', linewidth=1, label='Temperatur Klimaschrank')
plt.plot(df2['Minuten'], df2['Temp'],color='g', marker='o', markersize=2, linewidth=1, label='Temperatur Sensor Display OFF')


# Achsenbeschriftungen und Titel
plt.xlabel('Zeit (Minuten)')
plt.ylabel('Temperatur (°C)')
plt.title('Temperaturverlauf über die Zeit')

# Legende hinzufügen
plt.legend()

# Gitter hinzufügen
plt.grid(True)

# Diagramm anzeigen
plt.show()

# Diagramm für Abweichung erstellen
plt.figure(figsize=(15, 5))
plt.plot(df1['Minuten'], df1['Abweichung'], marker='d', markersize=2, linewidth=1, color='red', label='Abweichung Display OFF')
plt.plot(df2['Minuten'], df2['Abweichung'], marker='d', markersize=2, linewidth=1, color='blue', label='Abweichung Display ON')

# Achsenbeschriftungen und Titel
plt.xlabel('Zeit (Minuten)')
plt.ylabel('Abweichung (°C)')
plt.title('Abweichung zur Solltemperatur zwischen den Temperaturkurven über die Zeit')

# Legende hinzufügen
plt.legend()

# Gitter hinzufügen
plt.grid(True)

# Y-Achse genauer einteilen
y_ticks = [i for i in range(-10, 11, 1)]  # Einteilung von -10 bis 10 in 1er Schritten
plt.yticks(y_ticks)
plt.ylim(-3, 5)  # Setze den Bereich der Y-Achse von -10 bis 10

# Diagramm anzeigen
plt.show()
