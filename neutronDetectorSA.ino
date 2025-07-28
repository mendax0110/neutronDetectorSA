#include "neutronDetector.h"

NeutronDetector detector(A0);
ESP8266WebServer server(80);

void setup()
{
    Serial.begin(115200);

    IPAddress local_IP(192, 168, 1, 6);
    IPAddress gateway(192, 168, 1, 6);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("NeutronDetector", "admin");

    detector.begin();
    detector.registerHTTPEndpoints(server);
    server.begin();

    Serial.println("Access Point started");
    Serial.print("Connect to SSID: NeutronDetector, Password: admin\n");
    Serial.print("Access the web interface at: http://");
    Serial.println(WiFi.softAPIP());
}

void loop()
{
    detector.update();
    server.handleClient();
    delay(1);
}