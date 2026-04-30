Compile and upload firmware directly to the connected ESP32 via USB.

1. Run `arduino-cli board list` to find the port
2. Compile: `arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc --output-dir build/ firmware/`
3. Upload to the detected port
4. Open serial monitor after upload
