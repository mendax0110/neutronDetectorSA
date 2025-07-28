### Neutron Detector Standalone

This Repo contains the code for the Neutron Dector for NE213 Scintillator.
The ESP8266 is used to read the analog signal from the neutron detector and process it to detect neutron pulses. The ESP8266 also servers as a WIFI AP, from which the HAS can get the data via the HTTP API.

## Structure
1. `neutronDetector.h`: Contains the class definition for the Neutron Detector, including methods for initialization, pulse detection, and data processing.

2. `neutronDetector.cpp`: Implements the methods defined in `neutronDetector.h`, handling the logic for detecting neutron pulses and analyzing them.

3. `neutronDetectorSA.ino`: The main Arduino sketch that initializes the Neutron Detector, sets up the WiFi connection, and handles incoming HTTP requests to provide data.

## Usage
Build with Arduino IDE, PlatformIO or Sloeber IDE, select the ESP8266 NodeMCU board, and upload the code to the ESP8266. The device will start a WiFi access point and serve an HTTP API for data retrieval.