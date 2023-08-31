#include <Arduino.h>
#include <TinyGPS++.h>
#include <SPI.h>
#include <WiFi101.h>

#include <avr/dtostrf.h>

#include "arduino_secrets.h"
#include "wiring_private.h"

char ssid[] = SECRET_SSID; // Network SSID (name)
char pass[] = SECRET_PASS; // Network password 
int status = WL_IDLE_STATUS; // WiFi status

IPAddress server(192,168,1,133); // The servers IP address
int serverPort = 5257; // The port we need to connect to

WiFiClient client; // Creating our WiFi client object
TinyGPSPlus gps; // Create our gps object

Uart sercomSerial (&sercom3, 1, 0, SERCOM_RX_PAD_1, UART_TX_PAD_0); // Create the new UART instance assigning it to pin 1 and 0
// UART means universal asynchronous receiver-transmitter, it is a hardware communication protocol

// This is our hardcoded post requests, these are unfortunately necessary,
// because when we use strcat to create our package, it causes some memory fragmentation I believe
char PostRequest1[] = "POST /api/gps/gps-data HTTP/1.1\r\nContent-Type: application/json\r\nHost: 192.168.1.133:5257\r\nContent-Length: 140\r\n\n{\n\"ID\": 3,\n\"Name\":\"Van\",\n\"Latitude\": 37.7749,\n\"Longitude\": -122.4194,\n\"TimeStamp\": \"2023-08-11T15:30:00Z\",\n\"Active\": true,\n\"Altitude\": 1500}";
char PostRequest2[] = "POST /api/gps/gps-data HTTP/1.1\r\nContent-Type: application/json\r\nHost: 192.168.1.133:5257\r\nContent-Length: 144\r\n\n{\n\"ID\": 1,\n\"Name\":\"Toolbox\",\n\"Latitude\": 37.7749,\n\"Longitude\": -122.4194,\n\"TimeStamp\": \"2023-08-11T15:30:00Z\",\n\"Active\": true,\n\"Altitude\": 1500}";
char PostRequest3[] = "POST /api/gps/gps-data HTTP/1.1\r\nContent-Type: application/json\r\nHost: 192.168.1.133:5257\r\nContent-Length: 144\r\n\n{\n\"ID\": 1,\n\"Name\":\"Toolbox\",\n\"Latitude\": 36.7749,\n\"Longitude\": -121.4194,\n\"TimeStamp\": \"2023-08-11T15:30:00Z\",\n\"Active\": true,\n\"Altitude\": 1500}";

// GPS location data
float latitude;
float longitude;
float altitude;

// Data used to create a HTTP package (p means package)
char p_headerBase[115] = "POST /api/gps/gps-data HTTP/1.1\r\nContent-Type: application/json\r\nHost: 192.168.1.133:5257\r\nContent-Length: ";
char p_gps_id[20] = "\r\n\n{\n\"ID\": 1";
char p_gps_name[25] = ",\n\"Name\":\"Toolbox\"";
char p_latitude[18] = ",\n\"Latitude\": ";
char p_longitude[19] = ",\n\"Longitude\": ";
char p_timestamp[44] = ",\n\"TimeStamp\": \"2023-08-11T15:30:00Z\"";
char p_status[23] = "\",\n\"Active\": true";
char p_altitude[18] = ",\n\"Altitude\": ";


void setup() {
  // Initialize serial communication at 9600 bits per second, for both the serial and the UART sercom serial
  Serial.begin(9600);
  sercomSerial.begin(9600); 

  pinPeripheral(1, PIO_SERCOM); //Assign RX function to pin 1
  pinPeripheral(0, PIO_SERCOM); //Assign TX function to pin 0

  // Attempt to connect to the WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WiFi with credentials
    status = WiFi.begin(ssid, pass);

    // Wait 10 seconds for connection
    delay(10000);
  }

  // Here we print the network data
  Serial.println("You're connected to the network");
  Serial.println("----------------------------------------");
  PrintNetworkData();
  Serial.println("----------------------------------------");
  
  // And here we connect to the IIS server
  Serial.println("\nStarting connection to server...");
  // If you get a connection, print a message through serial
  if (client.connect(server, serverPort)) {
    // Make a post request to complete the connection
    client.println(PostRequest1); 
    PrintClientResponse();
    Serial.println("connected to server");    
  }  
}

void loop() 
{
  while (sercomSerial.available() > 0)
  { 
    if (gps.encode(sercomSerial.read()))
    {
  
    PrintGPSData();

    // Put the GPS data into these floats
    latitude = (gps.location.lat());
    longitude = (gps.location.lng());
    altitude = gps.altitude.meters();
    

    
    SendPostRequest(PostRequest1);
    PrintClientResponse();
    delay(5000);
    
    SendPostRequest(PostRequest2);
    PrintClientResponse();
    delay(5000);
  
    SendPostRequest(PostRequest3);
    PrintClientResponse();
    delay(5000);

    //CreatePackage();
    }
  }
  
  // This statement checks, whether the gps has received sufficient data within a period of time, to confirm the GPS connection
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected");
    while(true);
  }
  delay(500);
}


void SERCOM3_Handler()
{
  // Attach the interrupt handler to the SERCOM
  sercomSerial.IrqHandler();
}

void SendPostRequest(char PostRequest[])
{
  // This method sends a post request to the GPS' client, which is an IIS server
  if(client.available())
  { 
    client.println(PostRequest); 
  }
}

void PrintClientResponse() 
{
  // If there are bytes available, ie. a response from the server, we print it
  while (client.available()) 
  {
    char c = client.read();
    Serial.write(c);
  }
}

void PrintNetworkData() 
{
  // In this method we print the basic network information which includes the name of the network, and the GPS' assigned IP address
  Serial.println();
  Serial.println("Network Information:");
  Serial.print("Network name: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void PrintGPSData()
{
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6); // the , 6, is det amount of decimals to print, can also be applied to altitude
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());    
  }
  else
  {
    Serial.println("Location: Not Available");
  }
}  


void CreatePackage(){
  //Please note that the entirety of this block, is incomplete.
  // It should be split into several different methods, we simply ran out of time

  char c_latitude[sizeof(latitude)]; // Here we set c_latitude to the size to the appropriate amount of bytes
  dtostrf(latitude, 2, 6, c_latitude); // Here we convert our double latitude to our char[] c_latitude. 
  // 2 is the minimum width of int part of the double. 6 is the decimals
  Serial.print("Latitude: ");
  Serial.println(c_latitude);

  char c_longitude[sizeof(longitude)];
  dtostrf(longitude, 2, 6, c_longitude);
  Serial.print("Longitude: ");
  Serial.println(c_longitude);

  char c_altitude[sizeof(altitude)];
  dtostrf(altitude, 2, 6, c_altitude);
  Serial.print("Altitude: ");
  Serial.println(c_altitude);

  // Here we calculate the content size 
  int contentSize = sizeof(p_gps_id);
  contentSize += sizeof(p_gps_name);
  contentSize += sizeof(p_latitude);
  contentSize += sizeof(c_latitude);
  contentSize += sizeof(p_longitude);
  contentSize += sizeof(c_longitude);
  contentSize += sizeof(p_timestamp);
  contentSize += sizeof(p_status);
  contentSize += sizeof(p_altitude);
  contentSize += sizeof(c_altitude);
      
  // We create a base body for our content, and append all the data to it
  // However, I have read about strcat and how it can cause memory fragmentation, i do not know whether this is true, but it causes the arduino to interrupt the serial sometimes
  // We have tried a lot of different solutions to this problem, but none have solved them, and we had to look the other way, due to limited time.
  char bodyBase[sizeof(contentSize)];
  strcat(bodyBase, p_gps_id);
  strcat(bodyBase, p_gps_name);
  strcat(bodyBase, p_latitude);
  strcat(bodyBase, c_latitude);
  strcat(bodyBase, p_longitude);
  strcat(bodyBase, c_longitude);
  strcat(bodyBase, p_timestamp);
  strcat(bodyBase, p_status);      
  strcat(bodyBase, p_altitude);
  strcat(bodyBase, c_altitude);

  // Here we get the content length, and append it tot the header
  int i;
  char p_contentLength[4];
  itoa (i, p_contentLength, sizeof(bodyBase));
  strcat(p_header, p_contentLength);

  // And down here we append the body to the header, and create our new postrequest, and call the post request method
  strcat(p_header, bodyBase);
  char newPostRequest[sizeof(p_header)];
  strcat(newPostRequest, header);

  SendPostRequest(newPostRequest);
}
