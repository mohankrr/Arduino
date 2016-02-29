/*
This code connects to Azure IoT Hub and blinks the onboard LED on the MKR1000
when there is a message for the particular device. In addition this sketch demonstrates
completing, rejecting or abondoning a Azure IoT cloud-to-device message.

http://mohanp.com/complete-reject-abandon-mkr1000-azure-iot-hub-how-to-part2

Written by Mohan Palanisamy (http://mohanp.com)

Instructions are here to properly set up the MKR1000 for SSL connections.
http://mohanp.com/mkr1000-azure-iot-hub-how-to/

Feb 26, 2016
*/

#include <SPI.h>
#include <WiFi101.h>

const int MKR1000_LED = 6 ;

///*** WiFi Network Config ***///
char ssid[] = "YouRWifiSSID"; //  your network SSID (name)
char pass[] = "yourWifiPassword";    // your network password (use for WPA, or use as key for WEP)

///*** Azure IoT Hub Config ***///
//see: http://mohanp.com/mkr1000-azure-iot-hub-how-to/  for details on getting this right if you are not sure.

char hostname[] = "YourIoTHub.azure-devices.net";    // host name address for your Azure IoT Hub
char authSAS[] = "YourSASToken";

//message receive URI
String azureReceive = "/devices/MyMKR1000/messages/devicebound?api-version=2016-02-03"; 

// message Complete/Reject/Abandon URIs.  "etag" will be replaced with the message id E-Tag recieved from recieve call.
String azureComplete = "/devices/MyMKR1000/messages/devicebound/etag?api-version=2016-02-03";         
String azureReject   = "/devices/MyMKR1000/messages/devicebound/etag?reject&api-version=2016-02-03";  
String azureAbandon  = "/devices/MyMKR1000/messages/devicebound/etag/abandon?&api-version=2016-02-03"; 

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
    //Serial.println(response);
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

      //Now that the message is processed.. do either Complete, Reject or Abandon HTTP Call.

      //first get the ETag from the received message response 
      String eTag=getHeaderValue(response,"ETag");

      //Uncomment the following line and comment out Reject and Abandon calls to verify Complete
      azureIoTCompleteMessage(eTag);

      //Uncomment the following line and comment out Complete and Abandon calls to verify Reject
      //azureIoTRejectMessage(eTag);

      //Uncomment the following line and comment out Complete and Reject calls to verify Abandon
      //azureIoTAbandonMessage(eTag); 
      
    }
  }

  // polling..if pollingInterval has passed
  if (millis() - lastConnectionTime > pollingInterval) {
    digitalWrite(MKR1000_LED, LOW);
    azureIoTReceiveMessage();
    // note the time that the connection was made:
    lastConnectionTime = millis();

  }
}

//Receive Azure IoT Hub "cloud-to-device" message
void azureIoTReceiveMessage()
{
  httpRequest("GET", azureReceive, "","");
}

//Tells Azure IoT Hub that the message with the msgLockId is handled and it can be removed from the queue.
void azureIoTCompleteMessage(String eTag)
{
  String uri=azureComplete;
  uri.replace("etag",trimETag(eTag));

  httpRequest("DELETE", uri,"","");
}


//Tells Azure IoT Hub that the message with the msgLockId is rejected and can be moved to the deadletter queue
void azureIoTRejectMessage(String eTag)
{
  String uri=azureReject;
  uri.replace("etag",trimETag(eTag));

  httpRequest("DELETE", uri,"","");
}

//Tells Azure IoT Hub that the message with the msgLockId is abondoned and can be requeued.
void azureIoTAbandonMessage(String eTag)
{
  String uri=azureAbandon;
  uri.replace("etag",trimETag(eTag));

  httpRequest("POST", uri,"text/plain","");
}


void httpRequest(String verb, String uri,String contentType, String content)
{
    if(verb.equals("")) return;
    if(uri.equals("")) return;

    // close any connection before send a new request.
    // This will free the socket on the WiFi shield
    client.stop();
  
    // if there's a successful connection:
    if (client.connect(hostname, 443)) {
      client.print(verb); //send POST, GET or DELETE
      client.print(" ");  
      client.print(uri);  // any of the URI
      client.println(" HTTP/1.1"); 
      client.print("Host: "); 
      client.println(hostname);  //with hostname header
      client.print("Authorization: ");
      client.println(authSAS);  //Authorization SAS token obtained from Azure IoT device explorer
      client.println("Connection: close");

      if(verb.equals("POST"))
      {
          client.print("Content-Type: ");
          client.println(contentType);
          client.print("Content-Length: ");
          client.println(content.length());
          client.println();
          client.println(content);
      }else
      {
          client.println();
      }
    }
}

String getHeaderValue(String response, String headerName)
{
  String headerSection=getHeaderSection(response);
  String headerValue="";
  
  int idx=headerSection.indexOf(headerName);
  
  if(idx >=0)
  { 
  int skip=0;
  if(headerName.endsWith(":"))
    skip=headerName.length() + 1;
  else
    skip=headerName.length() + 2;

  int idxStart=idx+skip;  
  int idxEnd = headerSection.indexOf("\r\n", idxStart);
  headerValue=response.substring(idxStart,idxEnd);  
  }
  
  return headerValue;
}

//For some reason Azure IoT sets ETag string enclosed in double quotes 
//and that's not in sync with its other endpoints. So need to remove the double quotes
String trimETag(String value)
{
    String retVal=value;

    if(value.startsWith("\""))
      retVal=value.substring(1);

    if(value.endsWith("\""))
      retVal=retVal.substring(0,retVal.length()-1);

    return retVal;     
}

//To get all the headers from the HTTP Response
String getHeaderSection(String response)
{
  int idxHdrEnd=response.indexOf("\r\n\r\n");

  return response.substring(0, idxHdrEnd);
}

//To get only the message payload from http response.
String getResponsePayload(String response)
{
  int idxHdrEnd=response.indexOf("\r\n\r\n");

  return response.substring(idxHdrEnd + 4);
}

