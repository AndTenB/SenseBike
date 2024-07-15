import pandas as pd
import matplotlib.pyplot as plt

# CSV-Datei einlesen
df = pd.read_csv('stromverbrauch1.csv')

# Zeitstempel in datetime umwandeln
df['Zeit'] = pd.to_datetime(df['Zeit'])

# Durchschnittlicher Stromverbrauch
average_strom = df['Strom (mA)'].mean()

# Diagramm erstellen
plt.figure(figsize=(10, 6))

# Stromverbrauch plotten
plt.plot(df['Zeit'], df['Strom (mA)'], label='Stromverbrauch (mA)', color='blue')

# Durchschnittlichen Stromverbrauch plotten
plt.axhline(y=average_strom, color='red', linestyle='--', label=f'Durchschnittlicher Stromverbrauch ({average_strom:.2f} mA)')

# Diagramm anpassen
plt.title('Stromverbrauch über eine Minute')
plt.xlabel('Zeit')
plt.ylabel('Strom (mA)')
plt.legend()
plt.grid(True)

print("Diagram merstellt")
# Diagramm anzeigen
plt.show()

