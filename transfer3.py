import serial, time, requests
import pandas as pd
import numpy as np
from io import StringIO

# ----------------------------
# SERIAL CONFIG
# ----------------------------
SERIAL_PORT = "COM7"
BAUD_RATE = 115200

# ----------------------------
# SERVER CONFIG
# ----------------------------
BASE_URL = "https://azure.abhi.dedyn.io/iot/csv_data_{}.csv"

# ----------------------------
# FIXED NORMALIZATION CONSTANTS
# (FROM TRAINING DATASET)
# ----------------------------
GAS_Q80 = 37.0      # <-- replace with exact training value
AQI_Q80 = 1000.0    # <-- replace with exact training value

# ----------------------------
# SERVER CSV HAS NO HEADERS
# ----------------------------
COLUMNS = [
    "temperature",
    "pressure",
    "humidity",
    "gas",
    "aqi",
    "time_stamp_esp",
    "time_stamp_server"
]

# ----------------------------
# SERIAL INIT
# ----------------------------
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
time.sleep(2)

file_idx = 6
last_row = 0

# ============================
# MAIN LOOP
# ============================
while True:
    url = BASE_URL.format(file_idx)
    r = requests.get(url)

    if r.status_code != 200:
        time.sleep(5)
        continue

    # ----------------------------
    # Load CSV WITHOUT header
    # ----------------------------
    df = pd.read_csv(StringIO(r.text), header=None)

    # Validate column count
    if df.shape[1] != len(COLUMNS):
        print("❌ Column mismatch — skipping batch")
        time.sleep(5)
        continue

    # Assign correct column names
    df.columns = COLUMNS

    # ----------------------------
    # GAS + AQI AGGREGATION
    # (IDENTICAL TO TRAINING)
    # ----------------------------
    df["gas_norm"] = df["gas"] / GAS_Q80
    df["aqi_norm"] = df["aqi"] / AQI_Q80

    df["pollution_index"] = (
        0.6 * df["gas_norm"] +
        0.4 * df["aqi_norm"]
    )

    # ----------------------------
    # PROCESS ONLY NEW ROWS
    # ----------------------------
    new = df.iloc[last_row:]

    for _, row in new.iterrows():

        # Basic safety check
        if any(pd.isna([
            row["temperature"],
            row["humidity"],
            row["pressure"],
            row["pollution_index"]
        ])):
            continue

        # ----------------------------
        # SEND EXACTLY 4 FEATURES
        # (MATCHES TRAINING)
        # ----------------------------
        send = (
            f"{row['temperature']},"
            f"{row['humidity']},"
            f"{row['pressure']},"
            f"{row['pollution_index']}"
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
