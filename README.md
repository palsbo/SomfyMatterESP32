# SomfyMatter
### ESP32 based controller for Somfy RTS blinds.
Based on Matter and can be paired with all major home automation systems.

Hardware used: 
- LILYGO Lora32

Software:
- based on the greate work: SomfyMQTT by Rudi Radmaker (https://github.com/ruedli/SomfyMQTT)

When using Arduino IDE:
- Use ESP32 WROOM Module as selected board as the TOGO Lora do not provide the partition option.
- Select Huge "partition"
#### ✨ Features
The sample software provides a Matter node with 3 endpoints for 3 blinds. This can easely be changed.

When paired with Google Home, you can talk to your blinds!!
- Hey Googlle - "Open blind one"
- Hey Googlle - "Close blind one"
- Hey Googlle - "Open blind one to 30%"

<img src="SomfyMatterESP32.png" width="480"/>
  
