/*
This code connects to Azure IoT Hub and blinks the onboard LED on the MKR1000
when there is a message for the particular device.

Written by Mohan Palanisamy (http://mohanp.com)

Instructions are here to properly set up the MKR1000 for SSL connections.
http://mohanp.com/mkr1000-azure-iot-hub-how-to/
Feb 19, 2016
*/

#include <SPI.h>
#include <WiFi101.h>

const int MKR1000_LED = 6 ;

///*** WiFi Network Config ***///
char ssid[] = "YourWiFiNetworkSSID"; //  your network SSID (name)
char pass[] = "YourWiFiNetworkPasswd";    // your network password (use for WPA, or use as key for WEP)

///*** Azure IoT Hub Config ***///
//see: http://mohanp.com/  for details on getting this right if you are not sure.

char hostname[] = "YourIoTHubName.azure-devices.net";    // host name address for your Azure IoT Hub
char feeduri[] = "/devices/YourDeviceName/messages/devicebound?api-version=2016-02-03"; //feed URI
char authSAS[] = "YourSharedAccessKey";

///*** Azure IoT Hub Config ***///

unsigned long lastConnectionTime = 0;            
const unsigned long pollingInterval = 5L * 1000L; // 5 sec polling delay, in milliseconds

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

void setup() {

   pinMode(MKR1000_LED, OUTPUT);

  //check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
}

void loop() 
{
  String response = "";
  char c;
  ///read response if WiFi Client is available
  while (client.available()) {
    c = client.read();
    response.concat(c);
  }

  if (!response.equals(""))
  {
    //if there are no messages in the IoT Hub Device queue, Azure will return 204 status code. 
    if (response.startsWith("HTTP/1.1 204"))
    {
      //turn off onboard LED
      digitalWrite(MKR1000_LED, LOW);
    }
    else
    {
      //turn on onboard LED
      digitalWrite(MKR1000_LED, HIGH);
    }
  }

  // polling..if pollingInterval has passed
  if (millis() - lastConnectionTime > pollingInterval) {
    digitalWrite(MKR1000_LED, LOW);
    azureHttpRequest();
  }
}

// this method makes an HTTPS connection to the Azure IOT Hub Server:
void azureHttpRequest() {

  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(hostname, 443)) {
    //make the GET request to the Azure IOT device feed uri
    client.print("GET ");  //Do a GET
    client.print(feeduri);  // On the feedURI
    client.println(" HTTP/1.1"); 
    client.print("Host: "); 
    client.println(hostname);  //with hostname header
    client.print("Authorization: ");
    client.println(authSAS);  //Authorization SAS token obtained from Azure IoT device explorer
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

