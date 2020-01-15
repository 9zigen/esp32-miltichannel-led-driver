## This is led controller firmware.

Device use ESP8266 as main CPU and stm32f051 as RTC, PWM generator, analog sensor reader.

Communication between processor realised by I2C bus, but possible to implement UART.

## Build firmware
1. Download sdk and toolchain and setup [instruction](https://github.com/espressif/ESP8266_RTOS_SDK)
2. erase device `make flash erase`
2. build firmware and flash `make app flash`

## OTA
1. setup ota server url `make menuconfig`
    - go 'component config' --> 'ota'
    - change the path to the real http server
2. build firmware and flash `make app flash`
3. reboot the device and connect to the access point
4. configure WiFi on [http://192.168.4.1](http://192.168.4.1)
3. hold the "IO0" button after loading for 3 seconds. this will start the OTA sequence.