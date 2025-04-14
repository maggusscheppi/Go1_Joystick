# Go1_Joystick
remote control for the unitree go1 based on esp32, wifi, mqtt

Hardware:
ESP32 lolin wemos d1 e.g. from
https://www.az-delivery.de/products/esp32-d1-mini-nodemcu-wifi-modul-esp-32-bluetooth-internet-entwicklungsboard-mit-usb-c-anschluss-kompatibel-mit-arduino-und-kompatibel-mit-wemos-d1-mini

joystick, e.g. from
https://eckstein-shop.de/JoyStickBreakoutModulPS2SpielsteuerungGameControllerfC3BCrArduino

Analog i2c converter, e.g. from
https://de.aliexpress.com/item/32761481134.html

Download ino file and compile for your controller.
Take care of values when driving with 5V while developing and 3.3V when using. Values form adc differ.

You may use the mqtt topic debug/xy to send debug strings and watch them with a mqtt client, e.g. MQTTX
https://mqttx.app/downloads

housing:
https://www.thingiverse.com/thing:3384973

<img src="https://github.com/maggusscheppi/Go1_Joystick/blob/main/housing_closed.JPEG" width=300px;/>

<img src="https://github.com/maggusscheppi/Go1_Joystick/blob/main/housing_open.JPEG" width=300px;/>

