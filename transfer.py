import serial, time, requests

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

    lines = r.text.strip().splitlines()
    new = lines[last_row:]

    for line in new:
        parts = line.split(',')
        if len(parts) < 4:
            continue

        # reorder → temp,hum,press,gas
        send = f"{parts[0]},{parts[2]},{parts[1]},{parts[3]}"
        ser.write((send+"\n").encode())
        print("Sent:", send)

        start = time.time()
        while time.time() - start < 1:
            if ser.in_waiting:
                print("Nano:", ser.readline().decode().strip())
        time.sleep(0.2)

    last_row = len(lines)
    time.sleep(5)