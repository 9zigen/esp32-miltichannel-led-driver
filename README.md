# HTTPD Server Persistant Sockets Example

The Example consists of HTTPD server persistent sockets demo.
This sort of persistancy enables the server to have independent sessions/contexts per client.

* Configure the project using "make menuconfig" and goto :
    * Example Configuration ->
        1. WIFI SSID: WIFI network to which your PC is also connected to.
        2. WIFI Password: WIFI password

* In order to test the HTTPD server persistent sockets demo :
    1. compile and burn the firmware "make flash"
    2. run "make monitor" and note down the IP assigned to your ESP module. The default port is 80
    3. run the test script "python2 scripts/adder.py \<IP\> \<port\> \<N\>"
        * the provided test script sends (POST) numbers from 1 to N to the server which has a URI POST handler for adding these numbers into an accumulator that is valid throughout the lifetime of the connection socket, hence persistent
        * the script does a GET before closing and displays the final value of the accumulator

See the README.md file in the upper level 'examples' directory for more information about examples.

## Step 2: Run HTTP Server

Python has a built-in HTTP server that can be used for example purposes.

Open a new terminal to run the HTTP server, then run these commands to build the example and start the server, then build project:

Start http server at the directory of "build":

```
cd build
python -m SimpleHTTPServer 8070
```

# ESP8266 Toolchain & SDK
export PATH=$PATH:$HOME/dev/SDK/ESP8266_RTOS_SDK/xtensa-lx106-elf/bin
export IDF_PATH=$HOME/dev/SDK/ESP8266_RTOS_SDK


# Espressif ESP32 Partition Table
# Name, Type, SubType, Offset, Size, Flags
nvs,data,nvs,0x9000,16K,
otadata,data,ota,0xd000,8K,
phy_init,data,phy,0xf000,4K,
ota_0,app,ota_0,0x10000,960K,
ota_1,app,ota_1,0x110000,960K,