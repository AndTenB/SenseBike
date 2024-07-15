import gpxpy
import folium
import pandas as pd
import os

# Überprüfen, ob die Datei existiert
gpx_file_path = '/Users/andre/GPS/route.gpx'
print(os.path.exists(gpx_file_path))

# GPX-Datei lesen und parsen
def read_gpx_file(file_path):
    with open(file_path, 'r') as gpx_file:
        gpx = gpxpy.parse(gpx_file)
    return gpx

# GPX-Datei parsen und die Route extrahieren
def extract_route(gpx):
    route = []
    for track in gpx.tracks:
        for segment in track.segments:
            for point in segment.points:
                route.append((point.latitude, point.longitude, point.elevation, point.time))
    return route

# Route in Segmente aufteilen und auf einer OpenStreetMap plotten
def plot_route_with_custom_segments(route, segments, colors, line_widths, output_file='gpx_route22.html'):
    if not route:
        raise ValueError("Die Route ist leer.")
    
    if len(segments) != len(colors) or len(segments) != len(line_widths):
        raise ValueError("Die Länge der Segmente, Farben und Linienbreiten muss übereinstimmen.")
    
    # Karte erstellen, zentriert auf den ersten Punkt der Route
    map_osm = folium.Map(location=route[0][:2], zoom_start=14)
    
    for i, (start_index, end_index) in enumerate(segments):
        segment = [point[:2] for point in route[start_index:end_index]]
        color = colors[i]
        line_width = line_widths[i]
        folium.PolyLine(segment, color=color, weight=line_width, opacity=1.0).add_to(map_osm)
    
    # Karte speichern
    map_osm.save(output_file)

    print(f"Karte wurde erfolgreich unter '{output_file}' gespeichert.")

# Funktion zum Speichern der Punkte in einer CSV-Datei
def save_points_to_csv(route, start_index, end_index, output_file='filtered_points.csv'):
    filtered_points = route[start_index:end_index]
    data = {
        'Latitude': [point[0] for point in filtered_points],
        'Longitude': [point[1] for point in filtered_points],
        'Elevation': [point[2] for point in filtered_points],
        'Time': [point[3] for point in filtered_points]
    }
    df = pd.DataFrame(data)
    df.to_csv(output_file, index=False)
    print(f"Punkte von Index {start_index} bis {end_index} wurden erfolgreich in '{output_file}' gespeichert.")

# Hauptfunktion
def main():
    # GPX-Datei lesen und Route extrahieren
    gpx = read_gpx_file(gpx_file_path)
    route = extract_route(gpx)
    
    # Manuell definierte Segmente (start_index, end_index), Farben und Linienbreiten
    segments = [
        (0, 50),  # Segment von Punkt 0 bis Punkt 50
        (50, 100),  # Segment von Punkt 50 bis Punkt 100
        (100, 320),  # Segment von Punkt 100 bis Punkt 320
        (320, 415),  # Segment von Punkt 320 bis Punkt 415
        (415, 630),  # Segment von Punkt 415 bis Punkt 630
        (630, 670),  # Segment von Punkt 630 bis Punkt 670
        (670, 1000)  # Segment von Punkt 670 bis Punkt 1000
    ]
    colors = ['red', 'blue', 'red', 'yellow', 'blue', 'green', 'blue']  # Farben für die Segmente
    line_widths = [4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5]  # Linienbreiten für die Segmente
    
    # Route in Segmente plotten und als HTML-Datei speichern
    plot_route_with_custom_segments(route, segments, colors, line_widths)

    # Punkte zwischen Index 320 und 415 in einer CSV-Datei speichern
    save_points_to_csv(route, 320, 415)

if __name__ == "__main__":
    main()
