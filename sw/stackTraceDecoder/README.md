# EspStackTraceDecoder

This project uses the [ESPStackTraceDecoder](https://github.com/littleyoda/EspStackTraceDecoder) tool for decoding debug stack trace information from ESP8266 and ESP32 devices. It takes stack trace output from your serial monitor and translates it into a more readable and useful format by mapping addresses to functions in your code.

## How to Use
2. **Collect the Stack Trace:**
   - Enable debug output on your ESP device.
   - Copy the raw stack trace from your serial monitor.

3. **Run the Decoder (decode.sh):**
   - Use the tool to input the stack trace.  
   - The decoder translates the addresses to function names and file locations.

4. **Interpret Results:**
   - Use the decoded information to locate and fix bugs.
