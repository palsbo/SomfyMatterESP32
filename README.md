# SomfyMatter
### ESP32 based controller for Somfy RTS blinds.
Based on Matter and can be paired with all major home automation systems.
#### ✨ Features
The sample software provides a Matter node with 3 endpoints for 3 blinds. This can easely be changed.

When paired with Google Home, you can talk to your blinds!!
- Hey Googlle - "Open blind one"
- Hey Googlle - "Close blind one"
- Hey Googlle - "Open blind one to 30%"

#### Hardware used: 
LILYGO Lora32
When using Arduino IDE:
- Use TTGO LoRa32-OLED as selected board.
- <B>Important!</B> Change the file C:\Users\<USER>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.14\boards.txt:
  - change ttgo-lora32.upload.maximum_size=1310720 to ttgo-lora32.upload.maximum_size=1810720
  - change ttgo-lora32.build.partitions=default to ttgo-lora32.build.partitions=huge_app
- This changes the Partition scheme to "Huge"
#### Software:
- based on the greate work: SomfyMQTT by Rudi Radmaker (https://github.com/ruedli/SomfyMQTT)

<img src="public/SomfyMatterESP32.png" width="480"/>
  
