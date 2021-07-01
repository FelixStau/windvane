 /*******************************************************************************
 * Copyright (c) 2021 Norbert Walter
 * 
 * License: GNU GPL V3
 * https://www.gnu.org/licenses/gpl-3.0.txt
 *
 * LoRa_boat_monitor_abp.ino
 * 
 * Based on work of Thomas Telkamp and Matthijs Kooijman
 *
 * This uses ABP (Activation-by-personalisation), where a DevAddr and
 * Session keys are preconfigured (unlike OTAA, where a DevEUI and
 * application key is configured, while the DevAddr and session keys are
 * assigned/generated in the over-the-air-activation procedure).
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!
 *
 * To use this sketch, first register your application and device with
 * the things network, to set or generate a DevAddr, NwkSKey and
 * AppSKey. Each device should have their own unique values for these
 * fields.
 *
 *******************************************************************************/
// Includes
#include <WiFi.h>               // WiFi lib with TCP server and client
#include <WiFiClient.h>         // WiFi lib for clients
#include <WebServer.h>          // WebServer lib
#include <ESPmDNS.h>            // mDNS lib
#include <Update.h>             // Web Update server
#include <MD5Builder.h>         // MD5 lib

#include <U8x8lib.h>            // OLED Lib
#include <lmic.h>               // LoRa Lib
#include <hal/hal.h>            // LoRa Lib
#include <Wire.h>
#include <SPI.h>                // SPI/I2C Lib for OLED and BME280
#include <Adafruit_Sensor.h>    // BME280
#include <Adafruit_BME280.h>    // BME280
#include <Ticker.h>             // Timer lib
#include <EEPROM.h>             // EEPROM lib
#include <WString.h>            // Needs for structures
#include <OneWire.h>            // 1Wire lib
#include <DallasTemperature.h>  // DS18B20 lib

#include "Configuration.h"      // Configuration

configData actconf;             // Actual configuration, Global variable
                                // Overload with old EEPROM configuration by start. It is necessarry for port and serspeed
                                // Don't change the position!

#include "Definitions.h"        // Global definitions
#include "GPS.h"                // GPS parsing functions
#include "Functions.h"          // Function lib
#include "LoRa.h"               // LoRa Lib

#include "icon_html.h"          // Favorit icon
#include "css_html.h"           // CCS cascading style sheets
#include "js_html.h"            // JavaScript functions
#include "main_html.h"          // Main webpage
#include "sensorv_html.h"       // Sensor Values webpage
#include "lora_html.h"          // LoRa Info webpage
#include "settings_html.h"      // Settings webpage
#include "firmware_html.h"      // Firmware update webpage
#include "json_html.h"          // JSON webpage
#include "MD5_html.h"           // JavaScript crypt password with MD5
#include "fwupdate.h"           // Webpage for firmware update
#include "restart_html.h"       // Reset info webpage
#include "devinfo_html.h"       // Device info webpage
#include "error_html.h"         // Error 404 webpage

// Declarations
int value;                      // Value from first byte in EEPROM
int empty;                      // If EEPROM empty without configutation then set to 1 otherwise 0
configData defconf;             // Definition of default configuration data
configData oldconf;             // Configuration stucture for old config data in EEPROM
configData newconf;             // Configuration stucture for new config data in EEPROM

WebServer httpServer(actconf.httpport);   // Port for HTTP server
MDNSResponder mdns;                       // Activate DNS responder
WiFiServer server(actconf.dataport);      // Declare WiFi NMEA server port

Ticker Timer1;                  // Declare Timer for GPS data reading
Ticker Timer2;                  // Declare Timer for relay ontime

void setup() {
  
  // Read the first byte from the EEPROM
  EEPROM.begin(sizeEEPROM);
  value = EEPROM.read(cfgStart);
  EEPROM.end(); 
  // If the fist Byte not identical to the first value in the default configuration then saving a default configuration in EEPROM.
  // Means if the EEPROM empty then saving a default configuration.
  if(value == defconf.valid){
    empty = 0;                  // Marker for configuration is present
  }
  else{
    saveEEPROMConfig(defconf);
    empty = 1;                  // Marker for configuration is missing
  }

  // Loading EEPROM configuration
  actconf = loadEEPROMConfig(); // Overload with old EEPROM configuration by start. It is necessarry for serspeed

  // If the firmware version in EEPROM different to defconf then save the new version in EEPROM
  if(String(actconf.fversion) != String(defconf.fversion)){
    String fver = defconf.fversion;
    fver.toCharArray(actconf.fversion, 6);
    saveEEPROMConfig(actconf);
  }

  //##### Start serial 0 and serial 2 connections #####
  Serial.begin(actconf.serspeed);
  delay(200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(200);

  macAddress = ESP.getEfuseMac();
  macAddressTrunc = macAddress << 40;
  chipId = macAddressTrunc >> 40;

  // ESP8266 Information Data
  DebugPrintln(3, "Booting Sketch...");
  DebugPrintln(3, "");
  DebugPrint(3, actconf.devname);
  DebugPrint(3, " ");
  DebugPrint(3, actconf.fversion);
  DebugPrintln(3, " (C) Norbert Walter");
  DebugPrintln(3, "******************************************");
  DebugPrintln(3, "");
  DebugPrintln(3, "Modul Type: Heltec LoRa-32");
  DebugPrint(3, "SDK-Version: ");
  DebugPrintln(3, ESP.getSdkVersion());
  DebugPrint(3, "ESP32 Chip-ID: ");
  DebugPrintln(3, chipId);
  DebugPrint(3, "ESP32 Speed [MHz]: ");  
  DebugPrintln(3, ESP.getCpuFreqMHz());
  DebugPrint(3, "Free Heap Size [Bytes]: ");
  DebugPrintln(3, ESP.getFreeHeap());
  DebugPrintln(3, "");

  // Debug info for initialize the EEPROM
  if(empty == 1){
    DebugPrintln(3, "EEPROM config missing, initialization done");
  }
  else{
    DebugPrintln(3, "EEPROM config present");
  }

  // Loading EEPROM config
  DebugPrintln(3, "Loading actual EEPROM config");
  actconf = loadEEPROMConfig();
  DebugPrintln(3, "");
 
  DebugPrint(3, "Sensor ID: ");
  DebugPrintln(3, actconf.deviceID);
  DebugPrintln(3, "Wind Sensor Type: LoRa1000");
  DebugPrintln(3, "Sensor Type: LoRa Boat Monitor");
  DebugPrintln(3, "Voltage Input [V]: 0...12 ");
  DebugPrint(3, "Input Pin: GPIO ");
  DebugPrintln(3, ANALOG_IN);
  DebugPrintln(3, "Tank1 [%]: 0...100");
  DebugPrint(3, "Input Pin: GPIO ");
  DebugPrintln(3, TANK1_IN);
  DebugPrintln(3, "Tank2 [%]: 0...100");
  DebugPrint(3, "Input Pin: GPIO ");
  DebugPrintln(3, TANK2_IN); 
  DebugPrintln(3, "Temp Sensor: SD18B20 1Wire");
  DebugPrintln(3, "Value Range [°C]: -55...125");
  DebugPrint(3, "Input Pin: GPIO ");
  DebugPrintln(3, OneWIRE_PIN);
  DebugPrint(3, "Temp Unit: ");
  DebugPrintln(3, actconf.tempUnit);
  DebugPrintln(3, "");

  DebugPrintln(3,"Serial0 Txd is on pin: "+String(TX));
  DebugPrintln(3,"Serial0 Rxd is on pin: "+String(RX));
  DebugPrintln(3,"Serial2 Txd is on pin: "+String(TXD2));
  DebugPrintln(3,"Serial2 Rxd is on pin: "+String(RXD2));
  DebugPrintln(3, "");

  DebugPrint(3, "LoRa Frequency: ");
  DebugPrintln(3, actconf.lorafrequency);
  DebugPrint(3, "LoRa Channel: ");
  DebugPrintln(3, actconf.lchannel);
  DebugPrint(3, "Send Period [x30s]: ");
  DebugPrintln(3, actconf.tinterval);
  DebugPrint(3, "Spreading Factor: ");
  DebugPrintln(3, actconf.spreadf);
  DebugPrint(3, "Dynamic SF: ");
  DebugPrintln(3, actconf.dynsf);
  DebugPrintln(3, "");

//*****************************************************************************************
  // Starting access point for update server
  DebugPrint(3, "Access point started with SSID: ");
  DebugPrintln(3, actconf.sssid);
  DebugPrint(3, "Access point channel: ");
  DebugPrintln(3, WiFi.channel());
//  DebugPrintln(3, actconf.apchannel);
  DebugPrint(3, "Max AP connections: ");
  DebugPrintln(3, actconf.maxconnections);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(actconf.sssid, actconf.spassword, actconf.apchannel, false, actconf.maxconnections);
  hname = String(actconf.hostname) + "-" + String(actconf.deviceID);
  WiFi.hostname(hname);   // Provide the hostname
  DebugPrint(3, "Host name: ");
  DebugPrintln(3, hname);
  if(actconf.mDNS == 1){
    MDNS.begin(hname.c_str());                              // Start mDNS service
    MDNS.addService("http", "tcp", actconf.httpport);       // HTTP service
    MDNS.addService("nmea-0183", "tcp", actconf.dataport);  // NMEA0183 dada service for AVnav
  }  
  DebugPrintln(3, "mDNS service: activ");
  DebugPrint(3, "mDNS name: ");
  DebugPrint(3, hname);
  DebugPrintln(3, ".local");

  // Sart update server
  httpServer.begin();
  DebugPrint(3, "HTTP Update Server started at port: ");
  DebugPrintln(3, actconf.httpport);
  DebugPrint(3, "Use this URL: ");
  DebugPrint(3, "http://");
  DebugPrint(3, WiFi.softAPIP());
  DebugPrintln(3, "/update");
  DebugPrintln(3, "");

  #include "ServerPages.h"    // Webserver pages request functions
  
  // Connect to WiFi network
  DebugPrint(3, "Connecting WiFi client to ");
  DebugPrintln(3, actconf.cssid);

  // Load connection timeout from configuration (maxccount = (timeout[s] * 1000) / 500[ms])
  maxccounter = (actconf.timeout * 1000) / 500;

  // Wait until is connected otherwise abort connection after x connection trys
  WiFi.begin(actconf.cssid, actconf.cpassword);
  ccounter = 0;
  while ((WiFi.status() != WL_CONNECTED) && (ccounter <= maxccounter)) {
    delay(500);
    DebugPrint(3, ".");
    ccounter ++;
  }
  DebugPrintln(3, "");
  if (WiFi.status() == WL_CONNECTED){
    DebugPrint(3, "WiFi client connected with IP: ");
    DebugPrintln(3, WiFi.localIP());
    DebugPrintln(3, "");
    delay(100);
  }
  else{
    WiFi.disconnect(true);                // Abort connection
    DebugPrintln(3, "Connection aborted");
    DebugPrintln(3, "");
  }
  
  // Start the NMEA TCP server
  server.begin();
  DebugPrint(3, "NMEA-Server started at port: ");
  DebugPrintln(3, actconf.dataport);
  // Print the IP address
  DebugPrint(3, "Use this URL : ");
  DebugPrint(3, "http://");
  if (WiFi.status() == WL_CONNECTED){
    DebugPrintln(3, WiFi.localIP());
  }
  else{
    DebugPrintln(3, WiFi.softAPIP());
  };
  DebugPrintln(3, "");
//*****************************************************************************************

  
  //##### Pin Settings #####
  pinMode(ledPin, OUTPUT);          // LED Pin output
  pinMode(relayPin, OUTPUT);        // Relay Pin output
  pinMode(alarmPin, INPUT_PULLUP);  // Alarm Pin input
  
  //##### Start 1Wire sensors #####
  sensors.begin();

  //##### Cyclic timer #####
  Timer1.attach_ms(5000, readGPSValues);     // Start timer 1 all 5s cyclic GPS data reading
  Timer2.attach_ms(300000, relayTimer);      // Start timer 2 all 5min cyclic counter increment
  
  //##### Start OLED #####
  u8x8.begin();
  u8x8.setPowerSave(0);

  //####### Starting BME280 ######
  DebugPrint(3, "BME280 test at address: ");
  DebugPrintln(3, "0x"+String(address, HEX));

  I2CBME.begin(I2C_SDA, I2C_SCL, I2C_SPEED); // Redefinition of I2C pins see Definition.h
  
//  if (! bme.begin(address, &Wire)) { // Standard using I2C
  if (! bme.begin(address, &I2CBME)) {
    DebugPrintln(3,"Could not find a valid BME280 sensor, check wiring!");
    
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0,0,"NoWa(C)OBP");
    u8x8.drawString(11,0,actconf.fversion);
    u8x8.drawString(0,1,"Could not find a");
    u8x8.drawString(0,2,"valid BME280!");
    u8x8.drawString(0,4,"System stop");
    u8x8.refreshDisplay();    // Only required for SSD1606/7  
    while (1);
  }

  DebugPrint(3, "BME280 found at address: ");
  DebugPrintln(3, "0x"+String(address, HEX));
  DebugPrintln(3, "");
  
  // For more details on the following scenarious, see chapter
  // 3.5 "Recommended modes of operation" in the datasheet
  bme.setSampling(Adafruit_BME280::MODE_FORCED,   // Mode [NORMAL|FORCED|SLEEP]
                  Adafruit_BME280::SAMPLING_X2,   // Temperature [NONE|X2|X4|X8|X16]
                  Adafruit_BME280::SAMPLING_X16,  // Pressure [NONE|X2|X4|X8|X16]
                  Adafruit_BME280::SAMPLING_X1,   // Humidity [NONE|X2|X4|X8|X16]
                  Adafruit_BME280::FILTER_OFF     // Filter [OFF|X1...X16]
//                  Adafruit_BME280::STANDBY_MS_0_5 ) //Only used in Normal Mode 0,5ms stand by time
                  );
  
  //####### Starting LoRaWAN ######
  DebugPrintln(3,"Starting LoRaWAN");
  DebugPrint(3, "LoRa Frequency: ");
  DebugPrintln(3, actconf.lorafrequency);
  DebugPrint(3, "LoRa Channel: ");
  DebugPrintln(3, actconf.lchannel);
  DebugPrint(3, "Send Period [x30s]: ");
  DebugPrintln(3, actconf.tinterval);
  DebugPrint(3, "Spreading Factor: ");
  DebugPrintln(3, actconf.spreadf);
  DebugPrint(3, "Dynamic SF: ");
  DebugPrintln(3, actconf.dynsf);
  DebugPrintln(3, "");

  
  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  // Set send interval
  TX_INTERVAL = actconf.tinterval * 30;

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
//  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);  // For better receiving results

  // Set static session parameters. Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  #ifdef PROGMEM
  // On AVR, these values are stored in flash and only copied to RAM
  // once. Copy them to a temporary buffer here, LMIC_setSession will
  // copy them into a buffer of its own again.
  uint8_t appskey[sizeof(actconf.appkey)];
  uint8_t nwkskey[sizeof(actconf.nskey)];
  memcpy_P(appskey, actconf.appkey, sizeof(actconf.appkey));
  memcpy_P(nwkskey, actconf.nskey, sizeof(actconf.nskey));
  LMIC_setSession (0x1, actconf.devaddr, nwkskey, appskey);
  #else
  // If not running an AVR with PROGMEM, just use the arrays directly
  LMIC_setSession (0x1, actconf.devaddr, actconf.nskey, actconf.appkey);
  #endif

  #if defined(CFG_eu868)
  // Set up the channels used by the Things Network, which corresponds
  // to the defaults of most gateways. Without this, only three base
  // channels from the LoRaWAN specification are used, which certainly
  // works, so it is good for debugging, but can overload those
  // frequencies, so be sure to configure the full frequency range of
  // your network here (unless your network autoconfigures them).
  // Setting up channels should happen after LMIC_setSession, as that
  // configures the minimal channel set.
  // NA-US channels 0-71 are configured automatically
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
  // TTN defines an additional channel at 869.525Mhz using SF9 for class B
  // devices' ping slots. LMIC does not have an easy way to define set this
  // frequency and support for class B is spotty and untested, so this
  // frequency is not configured here.
  #elif defined(CFG_us915)
  // NA-US channels 0-71 are configured automatically
  // but only one group of 8 should (a subband) should be active
  // TTN recommends the second sub band, 1 in a zero based count.
  // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
  LMIC_selectSubBand(1);
  #endif

  // Enable the used channels
  setChannel(actconf.lchannel);

  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Set spreading factor depends on transmit slot and transmit power for uplink
  // (note: txpow seems to be ignored by the library)
  setSF(slot, actconf.spreadf, actconf.dynsf);

  // Start job
  do_send(&sendjob);
}

void loop() {
    // LoRa activities
    os_runloop_once();
    
    // BME280 measuerement
    bme.takeForcedMeasurement(); // has no effect in normal mode

    httpServer.handleClient();   // HTTP Server-handler for HTTP update server
}
