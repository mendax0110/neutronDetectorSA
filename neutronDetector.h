#ifndef NEUTRON_DETECTOR_H
#define NEUTRON_DETECTOR_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

/// @brief Class for detecting neutron pulses using an analog input. \class NeutronDetector
class NeutronDetector
{
public:

    static constexpr uint8_t SAMPLES_PER_PULSE = 30;
    static constexpr uint16_t MAX_PULSES = 30;
    static constexpr uint16_t SAMPLE_INTERVAL_US = 10;
    static constexpr uint16_t OVERSAMPLE_INTERVAL_US = 2;
    static constexpr uint8_t OVERSAMPLE_COUNT = 16;

    /**
     * @brief Structure representing a detected neutron pulse. \struct Pulse
     */
    struct Pulse
    {
        uint64_t timestamp;
        uint8_t samples[SAMPLES_PER_PULSE];
        uint8_t peakValue;
    };
    
    /**
     * @brief Structure representing the analysis of a neutron pulse. \struct PulseAnalysis
     */
    struct PulseAnalysis
    {
        float decayTime;
        float riseTime;
        float pulseArea;
        bool isNeutron;
        float baseline;
        float threshold;
    };

    /**
     * @brief Construct a new Neutron Detector object
     * 
     * @param analogPin The analog pin to which the neutron detector is connected.
     * @param threshold The threshold value for detecting neutron pulses.
     */
    NeutronDetector(uint8_t analogPin = A0, uint16_t threshold = 100);
    
    /**
     * @brief Initialize the neutron detector.
     */
    void begin();

    /**
     * @brief Check if the detector is initialized.
     * @return true if initialized, false otherwise.
     */
    bool isInitialized() const;

    /**
     * @brief Update the neutron detector state.
     */
    void update();

    /**
     * @brief Reset the neutron detector state.
     */
    void reset();
    
    /**
     * @brief Get the Pulse Count as the number of stored pulses.
     * @return uint16_t The number of stored pulses.
     */
    uint16_t getPulseCount() const;

    /**
     * @brief Get the Pulse object, which contains relevant data about the neutron pulse.
     * @param index The index of the pulse to retrieve.
     * @return const Pulse& The Pulse object at the specified index.
     */
    const Pulse& getPulse(uint16_t index) const;

    /**
     * @brief Get the analysis of a neutron pulse.
     * @param index The index of the pulse to analyze.
     * @return PulseAnalysis The analysis of the pulse at the specified index.
     */
    PulseAnalysis getPulseAnalysis(uint16_t index) const;

    /**
     * @brief Check if the input is connected.
     * @return true if connected, false otherwise.
     */
    bool isInputConnected() const;

    /**
     * @brief Register HTTP endpoints for the neutron detector.
     * @param server The ESP8266WebServer instance to register endpoints with. 
     */
    void registerHTTPEndpoints(ESP8266WebServer& server);

    /**
     * @brief Get the last captured pulse as a JSON string.
     * @return String JSON representation of the last pulse.
     */
    String getLastPulseJSON();

    /**
     * @brief Get the pulse history as a JSON string.
     * @param count The number of pulses to include in the history (default is 5).
     * @return String JSON representation of the pulse history.
     */
    String getPulseHistoryJSON(uint16_t count = 5);

    /**
     * @brief Get the statistics of the neutron detector as a JSON string.
     * @return String JSON representation of the statistics.
     */
    String getStatisticsJSON();

private:
    uint8_t _pin;
    uint16_t _threshold;
    Pulse _pulses[MAX_PULSES];
    uint16_t _writeIndex;
    uint16_t _storedCount;
    
    uint64_t _lastCaptureTime;
    const uint64_t _minInterval = 2000;
    
    float _baseline = 512.0f;
    float _noiseRMS = 40.0f;
    
    static constexpr uint16_t MAX_RAW_VALUE = 1023;
    static constexpr uint8_t MAX_SAMPLE_VALUE = 255;
    static constexpr uint8_t MIN_PULSE_AMPLITUDE = 10;
    static constexpr float NEUTRON_DECAY_TIME_THRESHOLD = 25.0f;
    static constexpr float NEUTRON_RISE_TIME_THRESHOLD = 12.0f;
    static constexpr float NEUTRON_AREA_THRESHOLD = 500.0f;
    static constexpr uint8_t BASELINE_DEVIATION_THRESHOLD = 5;

    bool _initialized = false;
    bool _inputConnected = false;
    uint64_t _lastConnectionCheck = 0;
    const uint64_t CONNECTION_CHECK_INTERVAL = 1000000;

    uint32_t _totalPulses = 0;
    uint32_t _neutronCount = 0;
    uint64_t _lastNeutronTime = 0;
    float _maxPulseArea = 0;
    float _maxDecayTime = 0;

    /**
     * @brief Capture a neutron pulse.
     */
    void capturePulse();

    /**
     * @brief Update the baseline noise level.
     */
    void updateBaseline();

    /**
     * @brief Perform oversampling to improve signal quality.
     * @param active The state of oversampling (true for active, false for inactive).
     * @return uint16_t The oversampled value.
     */
    uint16_t overSample(bool active);

    /**
     * @brief Compute the decay time of a neutron pulse.
     * @param p The Pulse object to analyze.
     * @return float The decay time in microseconds.
     */
    float computeDecayTime(const Pulse& p) const;

    /**
     * @brief Compute the area under the pulse curve.
     * @param p The Pulse object to analyze.
     * @return float The area of the pulse.
     */
    float computePulseArea(const Pulse& p) const;

    /**
     * @brief Compute the rise time of a neutron pulse.
     * @param p The Pulse object to analyze.
     * @return float The rise time in microseconds.
     */
    float computeRiseTime(const Pulse& p) const;

    /**
     * @brief Update the threshold for pulse detection.
     * @param currentDev The current deviation from the baseline.
     */
    void updateThreshold(float currentDev);

    /**
     * @brief Analyze a neutron pulse to determine its characteristics.
     * @param p The Pulse object to analyze.
     * @return PulseAnalysis The analysis result of the pulse.
     */
    PulseAnalysis analyzePulse(const Pulse& p) const;

    /**
     * @brief Check if the input is connected.
     * @return true if the input is connected, false otherwise.
     */
    bool checkInputConnected();

    /**
     * @brief Add a pulse to the JSON document.
     * @param doc The JSON document to which the pulse data will be added.
     * @param index The index of the pulse to add.
     */
    void addPulseToJSON(JsonDocument& doc, uint16_t index) const;
};

#endif // NEUTRON_DETECTOR_H