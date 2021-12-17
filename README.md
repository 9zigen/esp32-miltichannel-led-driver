## This is led controller firmware.

Device use ESP32 as main CPU and MCP7940 as RTC.
Communication between processor realised by I2C bus.

## Web UI Auth
http://192.168.4.1 or http://ledcontroller.local

Login: admin  
Password: 12345678

## Build firmware
1. Download sdk and toolchain and setup [instruction](https://github.com/espressif/esp-idf)
2. erase device `idf.py erase_flash`
2. build firmware and flash `idf.py flash`

## SSL Certs
openssl s_client -showcerts -connect fw.alab.cc:443 </dev/null 2>/dev/null|openssl x509 -outform PEM >fw_alab_cc.pem

## OTA
1. setup ota server url `idf.py menuconfig`
    - go 'component config' --> 'ota'
    - change the path to the real http server
2. build firmware and flash `idf.py flash`
3. reboot the device and connect to the access point
4. configure WiFi on [http://192.168.4.1](http://192.168.4.1) 
3. hold the "IO0" button after loading for 3 seconds. this will start the OTA sequence.