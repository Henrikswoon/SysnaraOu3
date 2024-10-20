#!/bin/bash
out_f="_performance_data.csv"
echo "threads,time_seconds" > "$out_f"
for i in $(seq 1 10); do
    for j in $(seq 1 100); do
        echo "iteration: $i, threadcount: $j"
        out=$({ time ./mdu -j "$j" /pkg; } 2>&1)  # Ta bort mellanslaget mellan 'out' och '='
        elapsed_time=$(echo "$out" | grep real | awk '{print $2}' | sed 's/[^0-9.]*//g')
        echo "$j,$elapsed_time" >> "$i$out_f"
    done
done

echo "Results stored to $out_f."
