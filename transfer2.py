import serial, time, requests
import pandas as pd
import numpy as np
from io import StringIO

SERIAL_PORT = "COM7"
BAUD_RATE = 115200
BASE_URL = "https://azure.abhi.dedyn.io/iot/csv_data_{}.csv"

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)

file_idx = 6
last_row = 0

while True:
    url = BASE_URL.format(file_idx)
    r = requests.get(url)
    if r.status_code != 200:
        time.sleep(5)
        continue

    # Load CSV into DataFrame
    df = pd.read_csv(StringIO(r.text))

    # Required columns:
    # temperature, humidity, pressure, gas, aqi, wind_speed

    # ---- SAME AGGREGATION AS TRAINING ----
    gas_q80  = df['gas'].quantile(0.8)
    aqi_q80  = df['aqi'].quantile(0.8)
    wind_q80 = df['wind_speed'].quantile(0.8)

    df['gas_norm'] = df['gas'] / gas_q80
    df['aqi_norm'] = df['aqi'] / aqi_q80

    df['pollution_index'] = (
        0.6 * df['gas_norm'] +
        0.4 * df['aqi_norm']
    )

    df['wind_norm'] = df['wind_speed'] / wind_q80

    df['effective_pollution'] = (
        df['pollution_index'] / (1.0 + df['wind_norm'])
    )

    new = df.iloc[last_row:]

    for _, row in new.iterrows():
        send = (
            f"{row['temperature']},"
            f"{row['humidity']},"
            f"{row['pressure']},"
            f"{row['effective_pollution']},"
            f"{row['wind_speed']}"
        )

        ser.write((send + "\n").encode())
        print("Sent:", send)

        start = time.time()
        while time.time() - start < 1:
            if ser.in_waiting:
                print("Nano:", ser.readline().decode().strip())

        time.sleep(0.2)

    last_row = len(df)
    time.sleep(5)
