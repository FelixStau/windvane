## Windsensor Yachta

![Windsnesor Yachta](Pictures/Yachta_Windsensor.jpg)

Image: Wind Sensor Yachta

The initial idea for the Yachta wind sensor is based on a work by the user Yachta. He presented and published the wind sensor on [Thingiverse](https://www.thingiverse.com/thing:2261719). Udo took up the idea of ​​Yachta and modified the mechanics a little. The arms of the cup wheel have been separated. This makes 3D printing easier to implement. The electronics were completely redesigned by him. The microcontroller was removed and replaced with an ESP8266. A DC/DC converter module was installed so that the wind sensor can also be operated at 12V. The circuit board was created with the Windows software Sprint Layout and converted into Gerber files.

The board without components can be ordered from [Aisler](https://aisler.net/p/GIPYPYTC).

There are different firmware for the wind sensor. Both Udo and Julian ([Jukolein](https://github.com/jukolein/NMEA0183-Windsensor)) have created their own firmware versions with different functionality. Udo has also designed another wind sensor, which is a further development of the Yachta wind sensor. The functional principle and the electronics are identical. But you need a different board because the wind sensor is slimmer. Julian published the whole thing on [Github](https://github.com/jukolein/NMEA0183-Windsensor) and wrote his own firmware for the wind sensor, which can also be used for this wind sensor. 

The WiFi 1000 firmware is a firmware that can be used universally for various wind sensors and was originally developed by Norbert for the [WiFi 1000 wind sensor](https://gitlab.com/norbertwalter67/Windsensor_WiFi_1000). The current versions of the firmware can be found in the respective own repositories at Github and GitLab.

A lively exchange of experiences takes place in the German-speaking sailing forum. You can also participate in English.

## Technical characteristics

* Measuring wind speed and wind direction
* Robust mechanics
* Without special metal parts
* All parts can be found in the hardware store
* Easy 3D parts
* Weight < 200g
* Weatherproof and UV stable
* No cabling for sensor signals
* Digital signal transmission with WiFi only
* 12V power supply utilizing top light power cable
* Using a ESP8266 for WiFi and data handling
* Update rate 1 samples per second
* No onboard instrument required
* Visualisation in OpenPlotter via laptop, mobile fone or tablett
* Web interface for operation
* No extra software required (display is the browser)
* Supporting  NMEA 0183 protocol
* Firmware update via Internet

## How can I use it?

I use the windsensor in combination with [OpenPlotter](http://www.sailoog.com/openplotter) on a Raspberry Pi 3B. OpenPlotter serves as access point for WiFi devices. The windsensor connects to OpenPlotter via WiFi and transmits the NMEA0183 data telegrams every second. The windsensor sends 4 types of telegrams. 3 standard telegrams (MWV, VWR, VPW) and one custom telegram (INF). Check here [link](http://www.nmea.de/nmea0183datensaetze.html) for more information on NMEA0183. 

![Connectivity](Pictures/Windsensor_Raspi_Handy_Laptop_M5Stack.png)

Network devices can be connected to OpenPlotter. A simple browser displays all data from the wind transducer. I use a 7" Android tablet as display for my windsensor.

![Instrument panel](Pictures/OpenPlotter_InstrumetPanel.png)

(2018-11-02) Now also available as Android App. It is is a WebApp connecting directly to the windsensor to display all data.

![Android App](Pictures/AppStart.png)
![Android App](Pictures/AppInstrument1.png)
![Android App](Pictures/AppInstrument2.png)

## What is the current status?

I have completed many tasks but still not everything is perfect so far.

- [x] Technical recherche
- [x] Evaluation of function
    - [x] LoRa trasmission tests
    - [x] Accessibility of gateways
    - [x] Measurement of telegram losses 
    - [x] Test spreeding factors
    - [x] Antenna test
    - [x] TTN configuration
    - [x] Ubitots configutarion and restrictions
- [x] Mechanical design
    - [x] 3D parts für Onshape
- [ ] Electronic design
    - [x] Circuit design
    - [x] Board design
    - [ ] Customer sample circuit
- [ ] Software
    - [x] Basic functionality for LoRa
    - [x] LoRa back channel for configuration
    - [x] Dynamic spreedung factor
    - [x] Voltage measurement
    - [x] Tank sensors
    - [x] GPS modul
    - [x] BME280
    - [x] Alarm sensors
    - [x] Relay function
    - [x] JSON
    - [x] Web-Updater
    - [x] Web-Configurator
    - [x] Web server as data display
    - [ ] Ubidots connection via WiFi
    - [ ] NMEA0183 WiFi
    - [ ] NMEA0183 Serial
    - [ ] MQTT
    - [x] Andoid App
    - [x] LoRa gateway firmware
- [x] Components ordering
    - [x] Electronic board
    - [x] Mechanical parts
    - [x] Fittings
- [x] Prototyp creation
    - [x] LoRa boat monitor
    - [x] LoRa gateway
- [ ] Production Zero series
    - [ ] DIY Kits
    - [x] Assembled LoRa boat monitor
- [x] Shipping Zero Series
- [ ] Documentation
    - [x] Documentation and update server on GitLab
    - [ ] Technical description
    - [ ] Construction manual
    - [x] Part list
    - [x] User manual
    - [ ] Data sheet
    - [ ] Flash instructions
- [x] Application tests
    - [x] Temperature 20...40°C
    - [x] LoRa range test (SF7...SF12)
    - [x] WiFi stability test
    - [x] Connection test
    - [x] Long time test
    - [x] User experiences
- [ ] Extensions
    - [ ] Battery current sensor
    - [ ] Actions and alarms depends on events and measuring values
    - [ ] Build a LoRa gateway structure on two towers for the IJsselmeer in Netherlands 
- [ ] Certification
    - [ ] CE Certification


## Questions?

For questions or suggestions please get in contact via email at norbert-walter(at)web.de.

If you like to help or consider this project useful, please donate. Thanks for your support!

[![Donate](Pictures/Donate.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5QZJZBM252F2L)
