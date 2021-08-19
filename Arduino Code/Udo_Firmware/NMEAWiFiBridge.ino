/*

   Copyright (c) Sept 8-2015. @ Larry Littlefield TCLS.com kb7kmo.blogspot.com
   GPL v2 
   https://github.com/svkatielee/

   
   Serial Wifi Bridge ^
   
   The desired purpose of this program is to take an rs-232 data stream and send it via 
   a UDP network protocol via WiFi to openCPN eleminating any data wires to the navagation system.
   Planning for a boat with a single network, the UDP is sent to the broadcast address
   of your network. ie.: 192.168.xx.255:10110

   Configure multiple openCPN systems connections, each with: network:UDP:localhost(0.0.0.0):10110 
   my testing with no checksum, no filtering at 4800 baud input
   
   It is written for an ESP8266-01 on a board similar to:
   http://www.forward.com.au/pfod/CheapWifiShield/ESP2866_01_WiFi_Shield/ESP8266_01_WiFi_Shield_R1/index.html

   It expects a NMEA 0183 rs-232 data stream input on the in pin connected to RX 
   via a 2N2222 to convert and invert -3.3v-+3.3v into 0-3.3v
   as seen: http://www.scienceprog.com/alternatives-of-max232-in-low-budget-projects/

   And some code and concepts borrowed from a friend, https://github.com/wa0uwh/ERB-EspWebServer

*/

 /**
 *  Cheap and Simple Wifi Shield for Arduino and other micros
 * http://www.forward.com.au/pfod/CheapWifiSheild/index.html
 *
 * (c)2015 Forward Computing and Control Pty. Ltd.
 * This code may be freely used for both private and commerical use.
 * Provide this copyright is maintained.
 */
#include <AS5600.h>
AS5600 encoder;
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "pfodWifiConfig.h"
void ICACHE_RAM_ATTR WinSensInterupt();
extern "C" {
    #include "user_interface.h"
    uint16 readvdd33(void);
}

#define DEBUG
#define Rev "LGL.00.06"
// ERB - Force format stings and string constants into FLASH Memory
    #define sF(x) (String) F(x)                // Used as an F() is being used as the first Element of a Multi-Element Expression
    #define FMT(x) strcpy_P(gFmtBuf, PSTR(x))  // Used with printf() for the format string
    // USE WITH CAUTION !
char gFmtBuf[64+2];



WiFiServer server(80);
WiFiClient client;
WiFiUDP udp;
pfodWifiConfig pfodWifiConfig;

// =============== start of pfodWifiWebConfig settings ==============
//#define pfodWifiWebConfigAP "WiFiBridge"
//#define pfodWifiWebConfigPASSWORD "lgl"
String pfodWifiWebConfigAP = "NMEA WiFi Bridge";
String pfodWifiWebConfigPASSWORD = "12345678";


// note pfodSecurity uses 19 bytes of eeprom usually starting from 0 so
// start the eeprom address from 20 for configureWifiConfig
int eepromAddress = 20;
int wifiSetup_pin = 2; // name the input pin for setup mode detection GPIO2 on most ESP8266 boards
// =============== end of pfodWifiWebConfig settings ==============

// On ESP8266-01 and Adafruit HAZZAH ESP8266, connect LED + 270ohm resistor from D0 (GPIO0) to +3V3 to indicate when in config mode

char ssid[pfodWifiConfig::MAX_SSID_LEN + 1]; // allow for null  field 1
char security[pfodWifiConfig::MAX_SECURITY_LEN + 1]; // field 2
char password[pfodWifiConfig::MAX_PASSWORD_LEN + 1]; // field 3
char portNo[pfodWifiConfig::MAX_PORTNO_STR_LEN + 1]; // field 4
char staticIP[pfodWifiConfig::MAX_STATICIP_LEN + 1]; // field 5 use dhcp if empty
char hostName[pfodWifiConfig::MAX_HOSTNAME_LEN + 1]; // field 6
char userName[pfodWifiConfig::MAX_USERNAME_LEN + 1]; // field 7
char userPw[pfodWifiConfig::MAX_USER_PASSWORD_LEN + 1]; // field 8
char e[50] = "0";
char d[5] = "0";
char c[50] = "0";

uint16_t ipSources = pfodFeatures::DHCP | pfodFeatures::STATIC_IP; // bit or these together pfodFeatures::DHCP|pfodFeatures::STATIC_IP if both are available
uint16_t securityFeatures = pfodFeatures::WPA | pfodFeatures::WPA2; // bit or these together e.g. pfodFeatures::OPEN | pfodFeatures::WPA

ESP8266WebServer webserver ( 80 );  // this just sets portNo nothing else happens until begin() is called

String msg;
byte inConfigMode = 0; // false

IPAddress opencpnServer;          // openCPN system server network broadcast
unsigned int localPort = 2948;    // local port to listen for UDP packets only needed for startup
unsigned int cpnPort;             // remote port to send UDP packets
unsigned int baudRate;            // set serial baud for reading NMEA (stored as hostName)

const byte maxLen = 100;          // longest NMEA sentence
byte packetBuffer[maxLen];        // buffer for UDP communication
int len=0;                        // num chars in buffer
boolean stringComplete = false;   // whether the string is complete
#ifdef DEBUG 
int loop_count=1;                 // show progress in debug window
#endif
uint16_t udpPackSent=0;           // the number of UDP packets sent

char gTmpBuf[32+2];  // Generic Temp Buffer
volatile boolean blStatusChange = false; 
long oldtimes = millis(); 
volatile long times = millis(); 
char CSSentence[85] = "" ; //string for sentence assembly
char MWVSentence[85] = "" ; //string for MWV for sentence assembly (Wind Speed and Angle) относительная скорость ветра
int cs ; //checksum

byte bySpeedCorrection = 100; 

//*******************************************************************
//===================================================================
void setup() {

    oldtimes = millis(); 
  times = millis();
  
  delay(2000); // skip the ESP8266 booting debug output
  pinMode(2, INPUT_PULLUP);  //GPO2 Input Interrupt
  pinMode(0,OUTPUT); // make GPIO0 an output
  digitalWrite(0,LOW); // make it LOW so it will ground GPIO2 if CONFIG_LINK is shorted out.
  // will turn led on, if any connected
  WiFi.mode(WIFI_STA);
  inConfigMode = 0; // non in config mode
  EEPROM.begin(512);
  
  Serial.begin(4800);
  delay(10);     // wait for serial to connect
  Serial.println();
  attachInterrupt(2, WinSensInterupt, FALLING); 

pfodWifiConfig.setDebugStream(&Serial); // add this line if using DEBUG in pfodWifiConfig_ESP8266 library code

  //============ pfodWifiConfigV1 config in Access Point mode ====================
  // see if config button is pressed
  pinMode(wifiSetup_pin, INPUT_PULLUP);
  if (digitalRead(wifiSetup_pin) == LOW) {
    inConfigMode = 1; // in config mode
    WiFi.mode(WIFI_AP_STA);

#ifdef DEBUG
    Serial.println(F("Setting up Access Point for pfodWifiWebConfig"));
    Serial.print("AP: ");
    Serial.print(pfodWifiWebConfigAP);
    Serial.print(" Pw: ");
    Serial.println( pfodWifiWebConfigPASSWORD);
#endif
    // connect to temporary wifi network for setup
    // the features determine the format of the {set...} command
    setupAP(pfodWifiWebConfigAP, pfodWifiWebConfigPASSWORD);
    //   Need to reboot afterwards
    return; // skip rest of setup();
  }  
//============ end pfodWifiConfigV1 config ====================

  // else button was not pressed continue to load the stored network settings
  // make GPIO0 an input to turn led off when not in config mode, if any is connected
  pinMode(0,INPUT);

    // use these local vars
  char ssid[pfodWifiConfig::MAX_SSID_LEN + 1]; // allow for null
  char password[pfodWifiConfig::MAX_PASSWORD_LEN + 1];
  char staticIP[pfodWifiConfig::MAX_STATICIP_LEN + 1];
  char hostName[pfodWifiConfig::MAX_HOSTNAME_LEN + 1]; // use for Serial in baud rate
  uint16_t portNo = 0; // use for UDP portnumber cpnPort
  uint16_t security = 0;
  uint16_t ipSource = 0;
  byte mode = 0;

  pfodWifiConfig.loadNetworkConfigFromEEPROM(eepromAddress, &mode,
      (char*)ssid, pfodWifiConfig::MAX_SSID_LEN + 1, (char*)password,  pfodWifiConfig::MAX_PASSWORD_LEN + 1,
      &security, &portNo, &ipSource, (char*)staticIP,  pfodWifiConfig::MAX_STATICIP_LEN + 1);
  pfodWifiConfig.loadClientConfigFromEEPROM(eepromAddress, (char*)hostName, pfodWifiConfig::MAX_HOSTNAME_LEN, (char*)userName,
      pfodWifiConfig::MAX_USERNAME_LEN, (char*)userPw, pfodWifiConfig::MAX_USER_PASSWORD_LEN);

  // Use hostName for storing the baud rate for the NMEA input
  long longBaud = 0;
      pfodWifiConfig::parseLong((byte*)hostName, &longBaud);
      baudRate = (uint16_t)longBaud;

  Serial.begin(baudRate);
  server = WiFiServer(80);
  cpnPort = portNo;
  // Initialise wifi module
#ifdef DEBUG
  Serial.println(F("Normal boot"));
  Serial.print("Web server on port:"); Serial.println(80);
  Serial.println(F("Connecting to AP"));
  Serial.print("ssid '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("password '");
  Serial.print(password);
  Serial.println("'");
#endif
  // Blink LED while trying to connect to WiFi
  pinMode(0,OUTPUT); // make GPIO0 an output
  int blink0 = 0;
  digitalWrite(0,blink0);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    blink0 = ! blink0;
    digitalWrite(0,blink0);
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }
  digitalWrite(0, HIGH);  // finished using LED for connecting status
  pinMode(0,INPUT);
#ifdef DEBUG
  Serial.println();
  Serial.println(F("Connected!"));
#endif

  if (*staticIP != '\0') {
    // config static IP
    IPAddress ip(pfodWifiConfig::ipStrToNum(staticIP));
    IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1
#ifdef DEBUG
    Serial.print(F("Setting gateway to: "));
    Serial.println(gateway);
#endif
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(ip, gateway, subnet);
  } // else leave as DHCP


  // configure normal boot web server
  webserver.on ( "/", handleRootNormal );
  webserver.onNotFound ( handleNotFound );
  
  // Start listening for connections
#ifdef DEBUG
  Serial.println(F("Start Server"));
#endif
  webserver.begin();
  
  opencpnServer = WiFi.localIP(); // find my local IP address
  opencpnServer[3] = 255;         // Set last octet to 255 for Class C broadcast address of this subnet
  
#ifdef DEBUG
  Serial.println(F("Server Started"));
  // Print the IP address
  Serial.print(WiFi.localIP());
  Serial.print(':');
  Serial.println(80);
  Serial.println(F("Listening for connections..."));
  Serial.print("UDP send address: ");
  Serial.print(opencpnServer);
  Serial.print(":");
  Serial.println(cpnPort);
#endif

#ifdef DEBUG
  Serial.println("Starting UDP");
#endif
  udp.begin(localPort);
#ifdef DEBUG 
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
#endif

#ifdef DEBUG
  Serial.println("+++"); // end of setup
#endif

}  // setup

//===================================================================
void setupAP( String ssid_wifi,  String password_wifi) {
  const char *aps = scanForAPs();
  delay(0);
#ifdef DEBUG
  Serial.println(aps);
#endif

  IPAddress local_ip = IPAddress(10, 1, 1, 1);
  IPAddress gateway_ip = IPAddress(10, 1, 1, 1);
  IPAddress subnet_ip = IPAddress(255, 255, 255, 0);

#ifdef DEBUG
  Serial.println(F("configure pfodWifiWebConfig"));
  Serial.print("AP: ");
  Serial.print(ssid_wifi);
  Serial.print(" Pw: ");
  Serial.println( password_wifi);
#endif

  WiFi.softAP(ssid_wifi.c_str(), password_wifi.c_str(), 6);
  //WiFi.softAP(ssid_wifi, 0, 6);

#ifdef DEBUG
  Serial.println();
  Serial.println(F("Access Point setup"));
#endif
  WiFi.softAPConfig(local_ip, gateway_ip, subnet_ip);

#ifdef DEBUG
  Serial.println("done");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("My address: "));
  Serial.println(myIP);
#endif


  msg = "<html>"
        "<head>"
        "<title>Serial to Wifi Bridge Setup</title>"
        "<meta charset=\"utf-8\" />"
        "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
        "</head>"
        "<body>"
        "<h2>Serial to Wifi Bridge Setup</h2>"
        "<p>Use this form to configure this device to connect to your Wifi network and start as a Status server listening"
        " on port 80. Also configure the port that the NMEA data will be broadcast via UDP. And the baud rate for the NMEA data.</p>"
        "<p>Note: UDP packets will be sent to the broadcast IP of your class C network. ie.: nnn.nnn.nnn.255</P>"

        "<form class=\"form\" method=\"post\" action=\"/config\" >"
        "<p class=\"name\">"
        "<label for=\"name\">Network SSID</label><br>"
        "<input type=\"text\" name=\"1\" id=\"ssid\" placeholder=\"wifi network name\"  required "; // field 1

  if (*aps != '\0') {
    msg += " value=\"";
    msg += aps;
    msg += "\" ";
  }
  msg += " />"
         "</p> "
         "<p class=\"security\">"
         "<label for=\"security\">Security</label><br>"
         "<select name=\"2\" id=\"security\" required>" // field 2
         "<option value=\"WPA/WPA2\">WPA/WPA2</option>"
         "</select>"
         "</p>"
         "<p class=\"password\">"
         "<label for=\"password\">Password</label><br>"
         "<input type=\"text\" name=\"3\" id=\"password\" placeholder=\"wifi network password\" autocomplete=\"off\" required " // field 3
         "</p>"
         "<p class=\"static_ip\">"
         "<label for=\"static_ip\">Set the Static IP for this device</label><br>"
         "(If this field is empty, DHCP will be used to get an IP address)<br>"
         "<input type=\"text\" name=\"5\" id=\"static_ip\" placeholder=\"192.168.4.99\" "  // field 5
         " pattern=\"\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b\"/>"
         "</p>"
         "<p class=\"portNo\">"
         "<label for=\"portNo\">UDP broadcast port.</label><br>"
         "<input type=\"text\" name=\"4\" id=\"portNo\" placeholder=\"10110\" required"  // field 4
         " pattern=\"\\b([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])\\b\" />"
         "</p>"
         "<p class=\"hostName\">"
         "<label for=\"hostName\">Baud rate for NMEA serial line.</label><br>"
        //"<input type=\"text\" name=\"6\" id=\"hostName\" placeholder=\"38400\" required"  // field 6
         "<select name=\"6\" id=\"hostName\" required>" // field 6 was hostName, using for Baud
         "<option value=\"4800\">4800</option>"
         "<option value=\"9600\">9600</option>"
         "<option value=\"38400\" selected>38400</option>"
         "<option value=\"115200\">115200</option>"
         "</select>"
         "</p>"
"<p class=\"submit\">"
         "<input type=\"submit\" value=\"Configure\"  />"
         "</p>"
         "</form>"
         "</body>"
         "</html>";


  delay(100);

  webserver.on ( "/", handleRoot );
  webserver.on ( "/config", handleConfig );
  webserver.onNotFound ( handleNotFound );
  webserver.begin();
#ifdef DEBUG
  Serial.println ( "HTTP webserver started" );
#endif
}  //end of setupAp



//********************************************************
// not really event driven serial input
void Daten() {
  //Create MWV Sentence - Wind Speed and Angle

 detachInterrupt(2);  
  int Z = 0;
  long WS = 0;

// attachInterrupt(2, WinSensInterupt, FALLING);
// blStatusChange = true;
 
if (blStatusChange) { 
//Serial.println(times);
    WS = ((33 * bySpeedCorrection) / (times));
     
   
   // Drehfrequenz = K * V / r. 
   // Wobei K der Koeffizient = 0,379 ist,
   // V die entgegenkommende Strömungsgeschwindigkeit ist, 
   // r der Radius von der Achse zur Mitte des Bechers ist
   // Laufradradius 40mm. Drehzahl (im Bogenmaß) = r (40 mm) * 2,64 (1 / 0,379) / t. 
   // Wir übersetzen in Umdrehungen und alles wird zehnmal erhöht, um Zehntel der Geschwindigkeit zu erreichen
   // neu: w (Winkelgeschwindigkeit) = K * V / r. 
   // Wobei K der Koeffizient = 0,379 ist, 
   // V die entgegenkommende Strömungsgeschwindigkeit ist, 
   // r der Radius von der Achse zur Mitte des Bechers ist
   // V = ​​(w * r) / K => V = ((2 * 3,14 / T) * r) / K. T-Periode. 
   // V = 0,66 / T. TK Vorperiode haben wir eine halbe Umdrehung, dann ist V = 0,33 / T.
   
  
  //  Serial.println(WS);
  } 
  else {
    WS = 0;
  }
  // Prüfsumme errechnen ******************************************
  //Assemble a sentance of the various parts so that we can calculate the proper checksum NMEA
  MWVSentence[0] = '\0';
  strcpy(MWVSentence, "PIMWV,"); //The string could be kept the same as what it came in as, but I chose to use the P (prefix for all proprietary devices) and I for instrument 
  Z =  encoder.getAngle(); 
  Z = Z /11.375;        //  4095/360  von 0 bis 360 Grad
  sprintf(e, "%d", Z);
  strcat(MWVSentence, e);
  strcat(MWVSentence, ",R,");
  sprintf(d, "%d", WS/10); //d represent the meters/seconds
  sprintf(c, ".%d", (WS%10)); //c represent the deci knots

 // sprintf(c, ".%d", (WS - ((WS / 10) * 10))); //c represent the deci knots
  strcat(MWVSentence, d);
  strcat(MWVSentence, c);
  strcat(MWVSentence, ",M,A");
 //****************************************************************** 
///////////////////// NMEA to serial //////////////////////////
 // NMEA(MWVSentence);

  Serial.print("$PIMWV,"); // Assemble the final message and send it out the serial port
  Serial.print(e);
  Serial.print(",R,");
  Serial.print(d);
  Serial.print(c);
  Serial.print(",M,A");
  Serial.print("*");
  Serial.print(Checksum(MWVSentence)); // Call Checksum function
  Serial.print("\r");
  Serial.print("\n");
  Serial.println();
}


//********************************************************
// send the unidirectional UDP buffer to the specific address and preconfigured port
unsigned long sendUDPpacket(IPAddress& address)
{
// udp Ausgabe*******
udp.beginPacket(address, cpnPort);
udp.print("$PIMWV,"); // Assemble the final message and send it out the serial port
udp.print(e);
udp.print(",R,");
udp.print(d);
udp.print(c);
udp.print(",M,A");
udp.print("*");
udp.print(Checksum(MWVSentence)); // Call Checksum function
udp.print("\r");
udp.print("\n");
udp.println(); 
udp.endPacket();
 attachInterrupt(2, WinSensInterupt, FALLING);
delay(500);
}



//*******************************************************************
//===================================================================
void loop() {
  if (inConfigMode) {
    webserver.handleClient();
    delay(0);
    return;
  } 
  delay(10);

#ifdef DEBUG 
  loop_count++;
  if ((loop_count % 10) == 0) Serial.print(".");
  if ((loop_count % 1000) == 0) Serial.println("!");
#endif  

  Daten();          // check for more NMEA data
  yield();
 // if (stringComplete) {   // have a whole NMEA sentence?
    sendUDPpacket(opencpnServer); 
    len=0;
    stringComplete = false; 
 
  yield();

  webserver.handleClient();  // check if client wants a webpage
  yield();
}

// ###########################################################
//////////////////////////////////////////////////////////////
//
// Provides UpTime in Seconds
//
unsigned long
upTime()
{
  return millis() / 1000;
}


//==============================================================================
String upTimeStr()
{
//    char buf[PBUFSIZE];
    
    int uptimesec = upTime();
    int uptimemin = uptimesec / 60;
    int uptimehr  = uptimemin / 60;
    int uptimeday = uptimehr  / 24;
    yield();

    uptimesec %= 60;
    uptimemin %= 60;
    uptimehr  %= 24;

    snprintf( gTmpBuf, sizeof(gTmpBuf),
        FMT("%d Days, %02d:%02d:%02d"), uptimeday, uptimehr, uptimemin, uptimesec );
    yield();
    
    return gTmpBuf;
} 



//==============================================================================
const size_t MAX_SSID_LEN = 32;
char ssid_found[MAX_SSID_LEN + 1]; // max 32 chars + null

// will always put '\0\ at dest[maxLen]
// return the number of char copied excluding the terminating null
size_t strncpy_safe(char* dest, const char* src, size_t maxLen) {
  size_t rtn = 0;
  if (src == NULL) {
    dest[0] = '\0';
  } else {
    strncpy(dest, src, maxLen);
    rtn = strlen(src);
    if ( rtn > maxLen) {
      rtn = maxLen;
    }
  }
  dest[maxLen] = '\0';
  return rtn;
}


//==============================================================================
const char* scanForAPs() {
  // WiFi.scanNetworks will return the number of networks found
  int8_t n = WiFi.scanNetworks();
#ifdef DEBUG
  Serial.print ("Scan done\n");
#endif
  delay(0);
  int32_t maxRSSI = -1000;
  strncpy_safe((char*)ssid_found, "", MAX_SSID_LEN); // empty
  if (n <= 0) {
#ifdef DEBUG
    Serial.print("No networks found\n");
#endif
  } else {
#ifdef DEBUG
    Serial.print("Networks found:");
    Serial.println(n);
#endif
    for (int8_t i = 0; i < n; ++i) {
      String ssid_scan = WiFi.SSID(i);
      int32_t rssi_scan = WiFi.RSSI(i);
      uint8_t sec_scan = WiFi.encryptionType(i);
      if (rssi_scan > maxRSSI) {
        maxRSSI = rssi_scan;
        strncpy_safe((char*)ssid_found, ssid_scan.c_str(), MAX_SSID_LEN);
      }
#ifdef DEBUG
      Serial.print(ssid_scan);
      Serial.print(" ");
      Serial.print(encryptionTypeToStr(sec_scan));
      Serial.print(" ");
      Serial.println(rssi_scan);
#endif

      delay(0);
    }
  }
  return ssid_found;
}

//==============================================================================
void handleRoot() {                  
  webserver.send ( 200, "text/html", msg );
}
 
//==============================================================================
void handleRootNormal() {     

  msg = "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>NMEA to Wifi Bridge</title>"
        "<meta http-equiv=\"refresh\" content=\"10\" />"
        "<meta charset=\"utf-8\" />"
        "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
        "<meta http-equiv='Pragma' content='no-cache'>"
        "<link rel='shortcut icon' href='http://espressif.com/favicon.ico'>"
        "<style>"
        "  body {background-color: #ddddee;}"
        "</style> "
        "</head>"
        "<body>"
        "<center>"
        "<h1>Serial to Wifi Bridge</h1>"
        "<p>This device broadcasts the input <br>"
        "NMEA data stream to your Wifi network.</p>"
        "<p>Configure multiple openCPN <br>"
        "systems\' connections, each with: <br>"
        "Properties   <b>network</b><br>"
        "Protocol <b>UDP</b><br>"
        "Address <b>localhost</b><br>"
        "DataPort <b>" + String((int)cpnPort) + "</b></p>"
        "<br>";
        
  msg += "    UDP to: <b>" + String(opencpnServer[0]) + "." + String(opencpnServer[1])
                     + "." + String(opencpnServer[2]) + "." + String(opencpnServer[3]);
  msg += ":" + String((int)cpnPort) + F("</b><br>");
  yield();
  msg += "    UDP packets sent: <b>" + String(udpPackSent) + F("</b><br>");
  msg += "    Uptime: <b>"          + String(upTimeStr()) + F("</b><br>");
  msg += "    Batt Voltage: <b>"    + String(readvdd33()/1000.0, 2) + F("V</b><br>");
 // msg += "    My IPA: <b>"fddd5819         + String(server.client().remoteIP()) + F("</b><br>");

  msg += "<br><br>To reconfigure, press and hold config button, press reset, "
         "continue to hold config until the yellow LED stays lit.<br>";
  msg += "<br>Revision: " + String(Rev) +  "<br>";
  msg += F("Powered by: <a href='http://espressif.com/'>Esp8266</a>");
  
  msg += "</center>"
         "</body>"
         "</html>";
  yield();      
#ifdef DEBUG
  Serial.print ("in handleRootNormal\n");
#endif                                        
  webserver.send ( 200, "text/html", msg );
}
//==============================================================================
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += ( webserver.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";

  for ( uint8_t i = 0; i < webserver.args(); i++ ) {
    message += " " + webserver.argName ( i ) + ": " + webserver.arg ( i ) + "\n";
  }

  webserver.send ( 404, "text/plain", message );
}

//==============================================================================
void handleConfig() {
  ssid[0] = '\0';  // field 1
  security[0] = '\0'; // field 2 this field skipped always WPA-WPA2
  password[0] = '\0'; // field 3
  portNo[0] = '\0'; // field 4
  staticIP[0] = '\0'; // field 5
  hostName[0] = '\0'; // field 6
  userName[0] = '\0'; // field 7
  userPw[0] = '\0'; // field 8
  uint16_t portNoInt = 80; // default

  byte mode = pfodFeatures::SERVER;
  uint16_t securityMode = pfodFeatures::WPA_WPA2;
  uint16_t portNo = 80;
  uint16_t ipSource = 0;

#ifdef DEBUG
  String message = "Config results\n\n";
  message += "URI: ";
  message += webserver.uri();
  message += "\nMethod: ";
  message += ( webserver.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webserver.args();
  message += "\n";

  for ( uint8_t i = 0; i < webserver.args(); i++ ) {
    message += " " + webserver.argName ( i ) + ": " + webserver.arg ( i ) + "\n";
  }
  Serial.println(message);
#endif

  uint8_t numOfArgs = webserver.args();
  const char *strPtr;
  uint8_t i = 0;
  for (; (i < numOfArgs); i++ ) {
    // check field numbers
    if (webserver.argName(i)[0] == '1') {
      strncpy_safe(ssid, (webserver.arg(i)).c_str(), pfodWifiConfig::MAX_SSID_LEN);
      urldecode2(ssid, ssid); // result is always <= source so just copy over
    } else if (webserver.argName(i)[0] == '3') {
      strncpy_safe(password, (webserver.arg(i)).c_str(), pfodWifiConfig::MAX_PASSWORD_LEN);
      urldecode2(password, password); // result is always <= source so just copy over
    } else if (webserver.argName(i)[0] == '6') {
      strncpy_safe(hostName, (webserver.arg(i)).c_str(), pfodWifiConfig::MAX_HOSTNAME_LEN);
      urldecode2(hostName, hostName); // result is always <= source so just copy over
    } else if (webserver.argName(i)[0] == '4') {
      // convert portNo to uint16_6
      const char *portNoStr = (( webserver.arg(i)).c_str());
      long longPort = 0;
      pfodWifiConfig::parseLong((byte*)portNoStr, &longPort);
      portNo = (uint16_t)longPort;
      if (portNo == 0) {
        portNo = 10110;
      }
    } else if (webserver.argName(i)[0] == '5') {
      strncpy_safe(staticIP, (webserver.arg(i)).c_str(), pfodWifiConfig::MAX_STATICIP_LEN);
      urldecode2(staticIP, staticIP); // result is always <= source so just copy over
      if (pfodWifiConfig::isEmpty(staticIP)) {
        // use dhcp
        staticIP[0] = '\0';
        ipSource = pfodFeatures::DHCP;
      }  else {
        ipSource = pfodFeatures::STATIC_IP;
      }
    }
  }


//==============================================================================
  pfodWifiConfig.storeValues( eepromAddress, mode, (byte *)ssid, (byte *)password, securityMode,
                              portNo, ipSource, (byte *)staticIP, (byte *)hostName, (byte *)userName, (byte *)userPw);
  delay(0);
  EEPROM.commit();
  delay(0);
  pfodWifiConfig.loadNetworkConfigFromEEPROM(eepromAddress, &mode,
      (char*)ssid, pfodWifiConfig::MAX_SSID_LEN + 1, (char*)password,  pfodWifiConfig::MAX_PASSWORD_LEN + 1,
      &securityMode, &portNo, &ipSource, (char*)staticIP,  pfodWifiConfig::MAX_STATICIP_LEN + 1);
  pfodWifiConfig.loadClientConfigFromEEPROM(eepromAddress, (char*)hostName, pfodWifiConfig::MAX_HOSTNAME_LEN, (char*)userName,
      pfodWifiConfig::MAX_USERNAME_LEN, (char*)userPw, pfodWifiConfig::MAX_USER_PASSWORD_LEN);

  if (webserver.args() == 0) {
    webserver.send ( 200, "text/html", msg );
  } else {
    String rtnMsg = "<html>"
                    "<head>"
                    "<title>NMEA WiFi Bridge Setup</title>"
                    "<meta charset=\"utf-8\" />"
                    "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
                    "</head>"
                    "<body>"
                    "<h3>NMEA WiFi Bridge Server Settings</h3><center><b>saved</center></b><br>";
    rtnMsg += ssid;
    if (ipSource == pfodFeatures::DHCP) {
      rtnMsg += "<br>IP address: <b>DCHP</b>";
    } else { // staticIP
      rtnMsg += "<br><IP addess: <b>";
      rtnMsg += staticIP;
    }
    rtnMsg += "</b><br>NMEA serial baud rate: <b>";
    rtnMsg += hostName;
    rtnMsg += "</b><br>Webserver status on port: <b>80</b><br>NMEA sentences on UDP port: <b>";
    rtnMsg += portNo;
    rtnMsg += "<b><br><br><center>Press reset to start</center><br>"
              "</body>"
              "</html>";

    webserver.send ( 200, "text/html", rtnMsg );
  }
}


//==============================================================================
const char WEP[] = "WEP";
const char TKIP[] = "TKIP";
const char CCMP[] = "CCMP";
const char NONE[] = "NONE";
const char AUTO[] = "WEP/WPA/WPA2";
const char UNKNOWN_ENCRY[] = "--UNKNOWN--";

const char* encryptionTypeToStr(uint8_t type) {
  if (type == ENC_TYPE_WEP) {
    return  WEP;
  } else if (type == ENC_TYPE_TKIP) {
    return  TKIP;
  } else if (type == ENC_TYPE_CCMP) {
    return  CCMP;
  } else if (type == ENC_TYPE_NONE) {
    return  NONE;
  } else if (type == ENC_TYPE_AUTO) {
    return  AUTO;
  } //else {
  return UNKNOWN_ENCRY;
}



//==============================================================================
#include <stdlib.h>
#include <ctype.h>

void urldecode2(char *dst, const char *src) {
  char a, b, c;
  if (dst == NULL) return;
  while (*src) {
    if (*src == '%') {
      if (src[1] == '\0') {
        // don't have 2 more chars to handle
        *dst++ = *src++; // save this char and continue
        // next loop will stop
        continue;
      }
    }
    if ((*src == '%') &&
        ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      // here have at least src[1] and src[2] (src[2] may be null)
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    }
    else {
      c = *src++;
      if (c == '+')c = ' ';
      *dst++ = c;
    }
  }
  dst = '\0'; // terminate result
}
//****************************************************
void WinSensInterupt() 
{ 
detachInterrupt(2); 
Serial.print("I");
  if (times == 0) { 
    oldtimes = millis(); 
    blStatusChange = false; 
    times = 10;
   
  }
  else
  {
    blStatusChange = true; 
    times = millis() - oldtimes; 
    oldtimes = millis(); 
Serial.println(times);
 attachInterrupt(2, WinSensInterupt, FALLING);
}}
//*****************************************************
String Checksum(char* CSSentence) 
{
  cs = 0; //clear any old checksum
 char e[50] = "0";
 for (int n = 0; n < strlen(CSSentence); n++) {
    cs ^= CSSentence[n]; //calculates the checksum
  }
  sprintf(e, "%02X", cs);
  return e;
}
//==============================================================================
