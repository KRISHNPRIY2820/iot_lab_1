#include <ft2_inferencing.h>
#include <edge-impulse-sdk/classifier/ei_run_classifier.h>

float features[5];   // 🔥 NOW 5 FEATURES

// --------------------------------
// Edge Impulse signal callback
// --------------------------------
int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = features[offset + i];
    }
    return 0;
}

// --------------------------------
// Read CSV:
// temp, hum, press, eff_poll, wind
// --------------------------------
bool readSerialCSV() {
    if (!Serial.available()) return false;

    String line = Serial.readStringUntil('\n');
    line.trim();

    int i1 = line.indexOf(',');
    int i2 = line.indexOf(',', i1 + 1);
    int i3 = line.indexOf(',', i2 + 1);
    int i4 = line.indexOf(',', i3 + 1);

    if (i1 < 0 || i2 < 0 || i3 < 0 || i4 < 0) return false;

    float temp = line.substring(0, i1).toFloat();
    float hum  = line.substring(i1 + 1, i2).toFloat();
    float pres = line.substring(i2 + 1, i3).toFloat();
    float effp = line.substring(i3 + 1, i4).toFloat();
    float wind = line.substring(i4 + 1).toFloat();

    // 🔥 EXACT SAME SCALING AS TRAINING
    features[0] = temp / 50.0;
    features[1] = hum / 100.0;
    features[2] = (pres - 1000.0) / 50.0;
    features[3] = effp / 2.0;
    features[4] = wind / 20.0;

    for (int i = 0; i < 5; i++) {
        if (isnan(features[i]) || isinf(features[i])) return false;
    }

    return true;
}

// --------------------------------
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Nano BLE Sense – Edge Impulse TinyML Ready");
}

// --------------------------------
void loop() {
    if (!readSerialCSV()) return;

    ei::signal_t signal;
    signal.total_length = 5;   // 🔥 UPDATED
    signal.get_data = &get_signal_data;

    ei_impulse_result_t result;
    if (run_classifier(&signal, &result, false) != EI_IMPULSE_OK) {
        Serial.println("Inference failed");
        return;
    }

    Serial.println("Predictions:");

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print("  ");
        Serial.print(result.classification[i].label);
        Serial.print(" : ");
        Serial.println(result.classification[i].value, 4);
    }

    Serial.println();
}
