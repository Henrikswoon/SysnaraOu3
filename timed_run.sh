#!/bin/bash
out_f="performance_data.csv"

# Skriv header till CSV-filen
echo "threads,time_seconds" > "$out_f"

# Loop genom antalet trådar från 1 till 100
for j in $(seq 1 100); do
    # Mät tiden för programmet och fånga utdata
    out=$({ time ./mdu -j "$j" /pkg /etc /usr/lib; } 2>&1)  # Ta bort mellanslaget mellan 'out' och '='

    # Extrahera den verkliga tiden från output
    elapsed_time=$(echo "$out" | grep real | awk '{print $2}' | sed 's/[^0-9.]*//g')

    # Skriv resultatet till CSV-filen (utan mellanslag efter kommatecknet)
    echo "$j,$elapsed_time" >> "$out_f"
done

echo "Results stored to $out_f."
