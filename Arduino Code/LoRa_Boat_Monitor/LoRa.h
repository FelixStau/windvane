#ifndef LoRa_h
#define LoRa_h

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

void do_send(osjob_t* j){
    // LoRa sending activ
    lora_activ = true;
    
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        DebugPrintln(3, F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.

        // Slot calculation
        slot = slotcounter % 12;   
        DebugPrint(3, "Slot = ");
        DebugPrint(3, slot);
        DebugPrintln(3, "");

        // Set different spreading factor for time slots
        setSF(slot, actconf.spreadf, actconf.dynsf);
        
        // Increment slot counter
        slotcounter++;
        
        // Read sensor values (BME280, DS18B20, ...)
        readValues();

        // int -> bytes
        byte counterLow = lowByte(counter16);
        byte counterHigh = highByte(counter16);
        // place the bytes into the payload
        mydata[0] = counterLow;
        mydata[1] = counterHigh;
        DebugPrint(3, F("Packet: "));
        DebugPrintln(3, counter16);

        // float -> int 48.234 -> 4823
        temperature16 = float2int(temperature + 50);
        // int -> bytes
        byte tempLow = lowByte(temperature16);
        byte tempHigh = highByte(temperature16);
        // place the bytes into the payload
        mydata[2] = tempLow;
        mydata[3] = tempHigh;
        DebugPrint(3, F("Temp: "));
        DebugPrintln(3, temperature16);

        // float -> int 48.234 -> 4823
        pressure16 = float2int(pressure / 10);
        byte pressLow = lowByte(pressure16);
        byte pressHigh = highByte(pressure16);
        // place the bytes into the payload
        mydata[4] = pressLow;
        mydata[5] = pressHigh;
        DebugPrint(3, F("Pressure: "));
        DebugPrintln(3, pressure16);

        // float -> int 48.234 -> 4823
        humidity16 = float2int(humidity);
        byte humLow = lowByte(humidity16);
        byte humHigh = highByte(humidity16);
        // place the bytes into the payload
        mydata[6] = humLow;
        mydata[7] = humHigh;
        DebugPrint(3, F("Humidity: "));
        DebugPrintln(3, humidity16);

        // float -> int 48.234 -> 4823
        dewp16 = float2int(dewp + 50);
        byte dewpLow = lowByte(dewp16);
        byte dewpHigh = highByte(dewp16);
        // place the bytes into the payload
        mydata[8] = dewpLow;
        mydata[9] = dewpHigh;
        DebugPrint(3, F("Dewpoint: "));
        DebugPrintln(3, dewp16);

        // float -> int 48.234 -> 4823
        voltage16 = float2int(voltage);
        byte voltageLow = lowByte(voltage16);
        byte voltageHigh = highByte(voltage16);
        // place the bytes into the payload
        mydata[10] = voltageLow;
        mydata[11] = voltageHigh;
        DebugPrint(3, F("Voltage: "));
        DebugPrintln(3, voltage16);

        // float -> int 48.234 -> 4823
        temp1wire16 = float2int(temp1wire + 50);
        // int -> bytes
        byte temp2Low = lowByte(temp1wire16);
        byte temp2High = highByte(temp1wire16);
        // place the bytes into the payload
        mydata[12] = temp2Low;
        mydata[13] = temp2High;
        DebugPrint(3, F("Temp1W: "));
        DebugPrintln(3, temp1wire16);

        // float 48.234567 -> int 4823.4567 -> 4823 4567
        float predot = int(longitude * 100);
        float postdot = (longitude * 100) - predot;
        longitude16_1 = uint16_t(predot);
        longitude16_2 = float4int(postdot);
        byte lon1Low = lowByte(longitude16_1);
        byte lon1High = highByte(longitude16_1);
        byte lon2Low = lowByte(longitude16_2);
        byte lon2High = highByte(longitude16_2);
        // place the bytes into the payload
        mydata[14] = lon1Low;
        mydata[15] = lon1High;
        mydata[16] = lon2Low;
        mydata[17] = lon2High;
        DebugPrint(3, F("Lon1: "));
        DebugPrintln(3, longitude16_1);
        DebugPrint(3, F("Lon2: "));
        DebugPrintln(3, longitude16_2);

        // float 48.234567 -> int 4823.4567 -> 4823 4567
        float predot2 = int(latitude * 100);
        float postdot2 = (latitude * 100) - predot2;
        latitude16_1 = uint16_t(predot2);
        latitude16_2 = float4int(postdot2);
        byte lat1Low = lowByte(latitude16_1);
        byte lat1High = highByte(latitude16_1);
        byte lat2Low = lowByte(latitude16_2);
        byte lat2High = highByte(latitude16_2);
        // place the bytes into the payload
        mydata[18] = lat1Low;
        mydata[19] = lat1High;
        mydata[20] = lat2Low;
        mydata[21] = lat2High;
        DebugPrint(3, F("Lat1: "));
        DebugPrintln(3, latitude16_1);
        DebugPrint(3, F("Lat2: "));
        DebugPrintln(3, latitude16_2);

        // float -> int 48.234 -> 4823
        tank1_16 = float2int(tank1p);
        byte tank1Low = lowByte(tank1_16);
        byte tank1High = highByte(tank1_16);
        // place the bytes into the payload
        mydata[22] = tank1Low;
        mydata[23] = tank1High;
        DebugPrint(3, F("Level1: "));
        DebugPrintln(3, tank1_16);
        
        // float -> int 48.234 -> 4823
        tank2_16 = float2int(tank2p);
        byte tank2Low = lowByte(tank2_16);
        byte tank2High = highByte(tank2_16);
        // place the bytes into the payload
        mydata[24] = tank2Low;
        mydata[25] = tank2High;
        DebugPrint(3, F("Level2: "));
        DebugPrintln(3, tank2_16);

        // int -> byte
        int alarmrelay = (actconf.relay * 16) + alarm1;
        byte alarmrelayLow = lowByte(alarmrelay);
        // place the bytes into the payload
        mydata[26] = alarmrelayLow;
        DebugPrint(3, F("Alarm: "));
        DebugPrintln(3, alarm1);

        // Relay
        DebugPrint(3, F("Relay: "));
        DebugPrintln(3, actconf.relay);

        String payload = "";
//        sprintf(payload,"%x",mydata);
        DebugPrint(3, F("Payload: "));
        DebugPrintln(3, payload);    
        
//        flashLED(100);  // Flash white LED on LoRa board
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        DebugPrintln(3, F("Packet queued"));

        // Refresh OLED data
        char cnt[10];
        dtostrf(int(counter16), 5, 0, cnt);
        char tmp[10];
        
        dtostrf(temperature, 5, 1, tmp);
        char pres[10];
        dtostrf(pressure, 5, 0, pres);
        char hum[10];
        dtostrf(humidity, 5, 1, hum);
        char dew[10];
        dtostrf(dewp, 5, 1, dew);
        char dt[10];
        dtostrf(float(TX_INTERVAL), 4, 0, dt);
        char vol[10];
        dtostrf(voltage, 5, 1, vol);
        char tmp2[10];
        dtostrf(temp1wire, 5, 1, tmp2);
        char tnk1[10];
        dtostrf(tank1p, 5, 1, tnk1);
        char tnk2[10];
        dtostrf(tank2p, 5, 1, tnk2);
        char alm[10];
        dtostrf(int(alarm1), 5, 0, alm);
        char rel[10];
        dtostrf(int(actconf.relay), 5, 0, rel);
        
        u8x8.setFont(u8x8_font_chroma48medium8_r);
        u8x8.drawString(0,0,"NoWa(C)OBP");
        u8x8.drawString(11,0,actconf.fversion);
        u8x8.drawString(0,1,"C:");
        u8x8.drawString(2,1,cnt);
        u8x8.drawString(0,2,"T:");
        u8x8.drawString(2,2,tmp);
        u8x8.drawString(0,3,"P:");
        u8x8.drawString(2,3,pres);
        u8x8.drawString(0,4,"H:");
        u8x8.drawString(2,4,hum);
        u8x8.drawString(0,5,"D:");
        u8x8.drawString(2,5, dew);
        u8x8.drawString(8,1,"dT:");
        u8x8.drawString(11,1, dt);
        u8x8.drawString(15,1, "s");
        u8x8.drawString(9,2,"V:");
        u8x8.drawString(11,2, vol);
        u8x8.drawString(8,3,"T2:");
        u8x8.drawString(11,3, tmp2);
        u8x8.drawString(0,6,"L:");
        u8x8.drawString(2,6, tnk1);
        u8x8.drawString(8,6,"L2:");
        u8x8.drawString(11,6, tnk2);
        u8x8.drawString(0,7,"A:");
        u8x8.drawString(2,7, alm);
        u8x8.drawString(8,7,"R:");
        u8x8.drawString(11,7, rel);
        u8x8.refreshDisplay();    // Only required for SSD1606/7  
        
        counter16++;
    }
    // Next TX is scheduled after TX_COMPLETE event.
    
    // LoRa sending end
    lora_activ = false;
}

void onEvent (ev_t ev) {
    // LoRa sending activ
    lora_activ = true;
  
    DebugPrint(3, os_getTime());
    DebugPrint(3, ": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            DebugPrintln(3, F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            DebugPrintln(3, F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            DebugPrintln(3, F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            DebugPrintln(3, F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            DebugPrintln(3, F("EV_JOINING"));
            break;
        case EV_JOINED:
            DebugPrintln(3, F("EV_JOINED"));
            break;
        case EV_RFU1:
            DebugPrintln(3, F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            DebugPrintln(3, F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            DebugPrintln(3, F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            DebugPrintln(3, F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              DebugPrintln(3, F("Received ack"));
            if (LMIC.dataLen) {
              // data received in rx slot after tx
              DebugPrint(3, F("Received "));
              DebugPrint(3, LMIC.dataLen);
              DebugPrintln(3, F(" bytes of payload"));
              for (int i = 0; i < LMIC.dataLen; i++) {
                DebugPrint(3, F("0x"));
                if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
                    DebugPrint(3, F("0"));
                }
                DebugPrint(3, String(LMIC.frame[LMIC.dataBeg + i], HEX));
                
                //*********************************************
                // Download Byte 0
                //################
                // Decoding download message for relais
                if(i == 0){
                  relaytimer = LMIC.frame[LMIC.dataBeg + i]; // Set relay time (Relay on)
                  relayTimer(); // Activate relay timer
                  actconf.relay = 1;
                  DebugPrint(3, F(" Download Massage Relay: "));
                  DebugPrint(3, relaytimer);
                  DebugPrintln(3, F(" x 5min"));
                }

                // Download Byte 1
                //################
                // Decoding spreading factor
                if(i == 1){
                  if(LMIC.frame[LMIC.dataBeg + i] >= 7 && LMIC.frame[LMIC.dataBeg + i] <= 10){
                    actconf.spreadf = LMIC.frame[LMIC.dataBeg + i];
                    DebugPrint(3, F(" Download Massage SF: "));
                    DebugPrintln(3, actconf.spreadf);
                  }
                  else{
                    DebugPrintln(1, F("Download Massage SF: Error"));
                  }
                }

                // Download Byte 2
                //################
                // Decoding LoRa send interval
                if(i == 2){
                  if(LMIC.frame[LMIC.dataBeg + i] > 0){
                    actconf.tinterval = LMIC.frame[LMIC.dataBeg + i];
                    TX_INTERVAL = actconf.tinterval * 30;
                    DebugPrint(3, F(" Download Massage LoRa send interval: "));
                    DebugPrint(3, actconf.tinterval);
                    DebugPrintln(3, F(" x 30s"));
                  }
                  else{
                    DebugPrintln(1, F("Download Massage Send Interval: Error"));
                  }
                }
                //*********************************************
              }
              DebugPrintln(3, "New download settings saved");
              saveEEPROMConfig(actconf);
              DebugPrintln(3, "");
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            DebugPrintln(3, F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            DebugPrintln(3, F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            DebugPrintln(3, F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            DebugPrintln(3, F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            DebugPrintln(3, F("EV_LINK_ALIVE"));
            break;
         default:
            DebugPrintln(3, F("Unknown event"));
            break;
    }
    // LoRa sending end
    lora_activ = false;
}

#endif
