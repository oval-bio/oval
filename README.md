# oval
open source hardware from oval.bio

current_ranger.ino is source code for the stock firmware.

current_ranger_bt_force.ino has datalogging enabled via the usb and serial ports.  

oled_init_attiny13 is the firmware for the oled initilization board.

Oled_init_stripboard_schematic.jpg contains a schematic for the Oled init board.  Only required if using the 
larger 2.4" DiyMore oled.

meter_telemetry contains an Adafruit MQTT example modified to parse & upload data from the Current Ranger 3v serial port.

