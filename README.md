# mbed-os-example-http-server

This application demonstrates how to run an HTTP server on an mbed OS 5 device.

Request parsing is done through [nodejs/http-parser](https://github.com/nodejs/http-parser).

## To build
Disclaimer: On Windows, You'll have to install python (Python 3.7.9), git (git version 2.28.0.windows.1), gcc (gcc.exe (GCC) 7.0.1 20170205 (experimental)), g++ (g++.exe (GCC) 7.0.1 20170205 (experimental)),  mbed-cli( using pip install mbed-cli, don't use mbed-cli-installer.exe, it doesn't work.)...  before moving ahead.

1. Delete ``mbed_app.json``. (Believe me, its going to save you a ton of time. Atleast, if you are using K64F.)
2. Build the project  using mbed CLI.
2.a. run "mbed new .'' in the directory of the cloned repo.
2.b. Set the Target using "mbed target K64F"
2.c. Set the Toolchain using "mbed toolchain GCC_ARM"
2.d. Export to MCUXpresso IDE using "mbed export -i mcuxpresso"
2.d.1 MCUXpresso on Windows never compiles mbed projects successfully, use mbed cli only for compilation.
3. Add the mbed-http library using "mbed add http://os.mbed.com/teams/sandbox/code/mbed-http/"
4. Compile using "mbed compile"
5. Flash the project to your development board.
5.a. Can Copmile and Flash using "mbed compile -f"
6. Attach a serial monitor to your board to see the debug messages.
6.a. Can see the serial messages using "mbed sterm -b <Baud>"

## Tested on

* K64F with Ethernet.
* NUCLEO_F411RE with ESP8266.
    * For ESP8266, you need [this patch](https://github.com/ARMmbed/esp8266-driver/pull/41).

But every networking stack that supports the [mbed OS 5 NetworkInterface API](https://docs.mbed.com/docs/mbed-os-api-reference/en/latest/APIs/communication/network_sockets/) should work.
