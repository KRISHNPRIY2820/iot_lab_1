#include <ft2_inferencing.h>
#include <edge-impulse-sdk/classifier/ei_run_classifier.h>

#define WINDOW_SIZE 8
#define FEATURES_PER_STEP 4
#define TOTAL_FEATURES (WINDOW_SIZE * FEATURES_PER_STEP)

// Sliding window buffer
float features[TOTAL_FEATURES];
int window_index = 0;
bool window_full = false;

// --------------------------------
// Edge Impulse signal callback
// --------------------------------
int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// --------------------------------
// Read ONE timestep from Serial
// CSV: temp,hum,press,pollution
// --------------------------------
bool readSerialCSV(float *out_features) {
    if (!Serial.available()) return false;

    String line = Serial.readStringUntil('\n');
    line.trim();

    int i1 = line.indexOf(',');
    int i2 = line.indexOf(',', i1 + 1);
    int i3 = line.indexOf(',', i2 + 1);

    if (i1 < 0 || i2 < 0 || i3 < 0) return false;

    float temp = line.substring(0, i1).toFloat();
    float hum  = line.substring(i1 + 1, i2).toFloat();
    float pres = line.substring(i2 + 1, i3).toFloat();
    float poll = line.substring(i3 + 1).toFloat();

    // 🔥 SAME SCALING AS TRAINING
    out_features[0] = temp / 50.0;
    out_features[1] = hum  / 100.0;
    out_features[2] = (pres - 1000.0) / 50.0;
    out_features[3] = poll / 2.0;

    for (int i = 0; i < FEATURES_PER_STEP; i++) {
        if (isnan(out_features[i]) || isinf(out_features[i])) return false;
    }

    return true;
}

// --------------------------------
// Push timestep into sliding window
// --------------------------------
void push_to_window(float *step) {
    // Shift left if window already full
    if (window_full) {
        memmove(
            features,
            features + FEATURES_PER_STEP,
            sizeof(float) * FEATURES_PER_STEP * (WINDOW_SIZE - 1)
        );
        memcpy(
            features + FEATURES_PER_STEP * (WINDOW_SIZE - 1),
            step,
            sizeof(float) * FEATURES_PER_STEP
        );
    } else {
        memcpy(
            features + FEATURES_PER_STEP * window_index,
            step,
            sizeof(float) * FEATURES_PER_STEP
        );
        window_index++;
        if (window_index == WINDOW_SIZE) {
            window_full = true;
        }
    }
}

// --------------------------------
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Nano BLE Sense – Windowed TinyML Ready");
}

// --------------------------------
void loop() {
    float step[FEATURES_PER_STEP];

    if (!readSerialCSV(step)) return;

    push_to_window(step);

    // Wait until window is full
    if (!window_full) {
        Serial.println("Collecting window...");
        return;
    }

    // --------------------------------
    // Run inference
    // --------------------------------
    ei::signal_t signal;
    signal.total_length = TOTAL_FEATURES;
    signal.get_data = &get_signal_data;

    ei_impulse_result_t result;
    if (run_classifier(&signal, &result, false) != EI_IMPULSE_OK) {
        Serial.println("Inference failed");
        return;
    }

    // --------------------------------
    // Print all predictions
    // --------------------------------
    Serial.println("Predictions:");

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print("  ");
        Serial.print(result.classification[i].label);
        Serial.print(" : ");
        Serial.println(result.classification[i].value, 4);
    }

    Serial.println();
}
