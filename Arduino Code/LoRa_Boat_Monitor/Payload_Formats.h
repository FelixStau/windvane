// This component is not for the Arduino code. The component is neccessary for implementation in the TTN network.
// The function Decoder() decode the received payload in the TTN network. Please copy the function in the TTN console
// under PAYLOAD FORMATS and save it.
// https://console.thethingsnetwork.org/applications/lora-testsensor/payload-formats

// The paylod has 21 bytes
// Test payload: 

// mydata[0] Low Byte Counter
// mydata[1] High Byte Counter
// mydata[2] Low Byte Temperature
// mydata[3] High Byte Temperature
// mydata[4] Low Byte Pressure
// mydata[5] High Byte Pressure
// mydata[6] Low Byte Humidity
// mydata[7] High Byte Humidity
// mydata[8] Low Byte Dewpoint
// mydata[9] High Byte Dewpoint
// mydata[10] Low Byte Voltage
// mydata[11] High Byte Voltage
// mydata[12] Low Byte Temperature 1Wire
// mydata[13] High Byte Temperature 1Wire
// mydata[14] Low Byte Longitude 1
// mydata[15] High Byte Longitude 1
// mydata[16] Low Byte Longitude 2
// mydata[17] High Byte Longitude 2
// mydata[18] Low Byte Latitude 1
// mydata[19] High Byte Latitude 1
// mydata[20] Low Byte Latitude 2
// mydata[21] High Byte Latitude 2
// mydata[22] Low Byte Tank Level 1
// mydata[23] High Byte Tank Level 1
// mydata[24] Low Byte Tank Level 2
// mydata[25] High Byte Tank Level 2
// mydata[26] Low Byte Alarm1 and Relay

// Copy the following code under PAYLOAD FORMATS in the TTN console

function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  
  var voffset = -1.1;  // Voltage offset
  var toffset = -10.10;// Temperature offset for BME280
  var poffset = 0      // pressure offset for altitude

  if (port === 1){
    var counter = ((bytes[1] << 8) | bytes[0]);
    var temperature = (((bytes[3] << 8) | bytes[2]) / 100) - 50 + toffset;
    temperature = Math.round(temperature * 10) / 10;
    var pressure = ((bytes[5] << 8) | bytes[4]) / 10 + poffset;
    var humidity = ((bytes[7] << 8) | bytes[6]) / 100;
    var dewpoint = (((bytes[9] << 8) | bytes[8]) / 100) - 50;
    dewpoint = Math.round(dewpoint * 10) / 10;
    var voltage = ((bytes[11] << 8) | bytes[10]) / 100 + voffset;
    voltage = Math.round(voltage * 100) / 100;
    var tempbattery = (((bytes[13] << 8) | bytes[12]) / 100) - 50;
    tempbattery = Math.round(tempbattery * 10) / 10;
    var longitude = ((bytes[15] << 8) | bytes[14]) / 100 + ((bytes[17] << 8) | bytes[16]) / 1000000;
    var latitude = ((bytes[19] << 8) | bytes[18]) / 100 + ((bytes[21] << 8) | bytes[20]) / 1000000;
    var level1 = ((bytes[23] << 8) | bytes[22]) / 100;
    var level2 = ((bytes[25] << 8) | bytes[24]) / 100;
    var alarm1 = bytes[26] & 0x01;
    var relay = (bytes[26] & 0x10) / 0x10;
  }
  
  return {
//    counter: counter,
//    temperature: temperature,
    pressure: pressure,
    humidity: humidity,
    dewpoint: dewpoint,
    voltage: voltage,
    tempbattery: tempbattery,
    position: {"value": 0, context:{"lat": latitude, "lng": longitude}},
    latitude: latitude,
    longitude: longitude,
    altitude: 1,
    hdop: 1.1,
//    level1: level1,
//    level2: level2,
    alarm1: alarm1,
//    relay: relay,
  };
}
