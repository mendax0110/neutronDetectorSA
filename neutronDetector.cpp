#include "neutronDetector.h"

NeutronDetector::NeutronDetector(uint8_t analogPin, uint16_t threshold)
    : _pin(analogPin)
    , _threshold(threshold)
    , _writeIndex(0)
    , _storedCount(0)
    ,_lastCaptureTime(0)
{

}

void NeutronDetector::begin()
{    
    _initialized = true;
    Serial.println("[INFO] NeutronDetector initialized with 10-bit ADC resolution");
}

bool NeutronDetector::isInitialized() const
{
    return _initialized;
}

void NeutronDetector::update()
{
    uint64_t now = micros();

    if (now - _lastConnectionCheck > CONNECTION_CHECK_INTERVAL)
    {
        _inputConnected = checkInputConnected();
        _lastConnectionCheck = now;

        if (!_inputConnected)
        {
            reset();
            return;
        }
    }

    if (!_inputConnected) return;

    updateBaseline();
    
    if (now - _lastCaptureTime >= _minInterval)
    {
        uint16_t val = overSample(true);
        if (val - _baseline >= _threshold)
        {
            capturePulse();
            _lastCaptureTime = now;
            _totalPulses++;
        }
    }
}

void NeutronDetector::capturePulse()
{
    Pulse& p = _pulses[_writeIndex];
    p.timestamp = micros();
    uint8_t peak = 0;
    uint64_t sampleStart = micros();

    for (uint8_t i = 0; i < SAMPLES_PER_PULSE; ++i)
    {
        while (micros() - sampleStart < i * SAMPLE_INTERVAL_US)
        {

        }
        
        uint16_t raw = overSample(true);
        if (raw >= MAX_RAW_VALUE) return;

        p.samples[i] = raw >> 2;  // 10-bit to 8-bit (1023/255 = 4)
        if (p.samples[i] > peak) peak = p.samples[i];
    }

    p.peakValue = peak;
    _writeIndex = (_writeIndex + 1) % MAX_PULSES;
    _storedCount = (_storedCount + 1) < MAX_PULSES ? (_storedCount + 1) : MAX_PULSES;

    PulseAnalysis analysis = analyzePulse(p);
    if (analysis.isNeutron)
    {
        _neutronCount++;
        _lastNeutronTime = p.timestamp;
    }
    if (analysis.pulseArea > _maxPulseArea) _maxPulseArea = analysis.pulseArea;
    if (analysis.decayTime > _maxDecayTime) _maxDecayTime = analysis.decayTime;
}

uint16_t NeutronDetector::getPulseCount() const
{
    return _storedCount;
}

const NeutronDetector::Pulse& NeutronDetector::getPulse(uint16_t index) const
{
    if (index >= _storedCount)
    {
        static Pulse defaultPulse = {0};
        return defaultPulse;
    }
    uint16_t actualIndex = (_writeIndex + MAX_PULSES - _storedCount + index) % MAX_PULSES;
    return _pulses[actualIndex];
}

NeutronDetector::PulseAnalysis NeutronDetector::getPulseAnalysis(uint16_t index) const
{
    return analyzePulse(getPulse(index));
}

bool NeutronDetector::isInputConnected() const
{
    return _inputConnected;
}

void NeutronDetector::reset()
{
    _writeIndex = 0;
    _storedCount = 0;
}

uint16_t NeutronDetector::overSample(bool active)
{
    if (!active) return analogRead(_pin);

    uint32_t sum = 0;
    uint64_t start = micros();

    for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++)
    {
        sum += analogRead(_pin);
        while (micros() - start < i * OVERSAMPLE_INTERVAL_US)
        {

        }
    }

    return sum >> 4;  // dvide by 16 (average of the OVERSAMPLE_COUNT)
}

void NeutronDetector::updateBaseline()
{
    uint16_t newReading = overSample(true);
    float dev = newReading - _baseline;
    _baseline = 0.95f * _baseline + 0.05f * newReading; // filter to stabilize

    if (fabs(dev) > BASELINE_DEVIATION_THRESHOLD)
    {
        updateThreshold(dev);
    }
}

void NeutronDetector::updateThreshold(float currentDev)
{
    _noiseRMS = 0.95f * _noiseRMS + 0.05f * fabs(currentDev);
    _noiseRMS = max(_noiseRMS, 2.0f);
    _threshold = _baseline + 4 * _noiseRMS;
}

float NeutronDetector::computeDecayTime(const Pulse& p) const
{
    uint8_t peak = 0;
    uint8_t peakIndex = 0;

    for (uint8_t i = 0; i < SAMPLES_PER_PULSE; ++i)
    {
        if (p.samples[i] > peak)
        {
            peak = p.samples[i];
            peakIndex = i;
        }
    }

    if (peak < MIN_PULSE_AMPLITUDE) return -1.0f;

    const uint8_t threshold = peak * 0.1f;
    for (uint8_t i = peakIndex; i < SAMPLES_PER_PULSE; ++i)
    {
        if (p.samples[i] < threshold)
        {
            return (i - peakIndex) * SAMPLE_INTERVAL_US;
        }
    }

    return -1.0f;
}

float NeutronDetector::computePulseArea(const Pulse& p) const
{
    float area = 0;
    for (uint8_t i = 0; i < SAMPLES_PER_PULSE - 1; ++i)
    {
        area += (p.samples[i] + p.samples[i + 1]) * 0.5f * SAMPLE_INTERVAL_US;
    }
    return area;
}

float NeutronDetector::computeRiseTime(const Pulse& p) const
{
    uint8_t peak = 0;
    for (uint8_t i = 0; i < SAMPLES_PER_PULSE; ++i)
    {
        if (p.samples[i] > peak) peak = p.samples[i];
    }

    const float threshold10 = 0.1f * peak;
    const float threshold90 = 0.9f * peak;
    uint8_t t10 = 0;
    uint8_t t90 = 0;

    for (uint8_t i = 0; i < SAMPLES_PER_PULSE; ++i)
    {
        if (p.samples[i] >= threshold10)
        {
            t10 = i;
            break;
        }
    }

    for (uint8_t i = t10; i < SAMPLES_PER_PULSE; ++i)
    {
        if (p.samples[i] >= threshold90)
        {
            t90 = i;
            break;
        }
    }

    return (t90 - t10) * SAMPLE_INTERVAL_US;
}

NeutronDetector::PulseAnalysis NeutronDetector::analyzePulse(const Pulse& p) const
{
    PulseAnalysis result;
    result.decayTime = computeDecayTime(p);
    result.riseTime = computeRiseTime(p);
    result.pulseArea = computePulseArea(p);
    result.baseline = _baseline;
    result.threshold = _threshold;

    result.isNeutron = (result.decayTime > NEUTRON_DECAY_TIME_THRESHOLD) &&
                      (result.riseTime > NEUTRON_RISE_TIME_THRESHOLD) &&
                      (result.pulseArea > NEUTRON_AREA_THRESHOLD);

    return result;
}

bool NeutronDetector::checkInputConnected()
{
    int stableReadings = 0;
    for (int i = 0; i < 10; i++)
    {
        int val = analogRead(_pin);
        if (val > 10 && val < MAX_RAW_VALUE - 10)
        {
            stableReadings++;
        }
        delayMicroseconds(100);
    }
    return (stableReadings >= 8);
}

void NeutronDetector::registerHTTPEndpoints(ESP8266WebServer& server)
{
    server.on("/neutron/last", HTTP_GET, [this, &server]()
    {
        server.send(200, "application/json", getLastPulseJSON());
    });
    
    server.on("/neutron/history", HTTP_GET, [this, &server]()
    {
        String countParam = server.arg("count");
        uint16_t count = countParam.toInt();
        if (count == 0) count = 5;
        server.send(200, "application/json", getPulseHistoryJSON(count));
    });
    
    server.on("/neutron/stats", HTTP_GET, [this, &server]()
    {
        server.send(200, "application/json", getStatisticsJSON());
    });
}

String NeutronDetector::getLastPulseJSON()
{
    if (getPulseCount() == 0)
    {
        return "{\"status\":\"error\",\"message\":\"no_pulses_detected\"}";
    }

    DynamicJsonDocument doc(1024);
    uint16_t lastIndex = getPulseCount() - 1;
    addPulseToJSON(doc, lastIndex);
    
    String output;
    serializeJson(doc, output);
    return output;
}

String NeutronDetector::getPulseHistoryJSON(uint16_t count)
{
    DynamicJsonDocument doc(2048);
    JsonArray pulses = doc.createNestedArray("pulses");

    uint16_t actualCount = min(count, getPulseCount());
    uint16_t startIndex = (getPulseCount() >= count) ? (getPulseCount() - count) : 0;

    for (uint16_t i = startIndex; i < startIndex + actualCount; i++)
    {
        JsonObject pulseObj = pulses.createNestedObject();
        DynamicJsonDocument pulseDoc(512);
        addPulseToJSON(pulseDoc, i);
        pulseObj.set(pulseDoc.as<JsonObject>());
    }

    doc["count"] = actualCount;
    doc["total_pulses"] = _totalPulses;
    doc["neutron_count"] = _neutronCount;

    String output;
    serializeJson(doc, output);
    return output;
}

String NeutronDetector::getStatisticsJSON()
{
    DynamicJsonDocument doc(512);
    
    doc["total_pulses"] = _totalPulses;
    doc["neutron_count"] = _neutronCount;
    doc["last_neutron_time"] = _lastNeutronTime;
    doc["max_pulse_area"] = _maxPulseArea;
    doc["max_decay_time"] = _maxDecayTime;
    doc["current_baseline"] = _baseline;
    doc["current_threshold"] = _threshold;
    doc["input_connected"] = _inputConnected;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void NeutronDetector::addPulseToJSON(JsonDocument& doc, uint16_t index) const
{
    const Pulse& pulse = getPulse(index);
    PulseAnalysis analysis = getPulseAnalysis(index);

    doc["timestamp"] = pulse.timestamp;
    doc["decay_time"] = analysis.decayTime;
    doc["rise_time"] = analysis.riseTime;
    doc["pulse_area"] = analysis.pulseArea;
    doc["is_neutron"] = analysis.isNeutron;
    doc["baseline"] = analysis.baseline;
    doc["threshold"] = analysis.threshold;
    doc["peak_value"] = pulse.peakValue;

    JsonArray samples = doc.createNestedArray("raw_samples");
    for (uint8_t i = 0; i < SAMPLES_PER_PULSE; i++)
    {
        samples.add(pulse.samples[i]);
    }
}