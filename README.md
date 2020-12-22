# mbed-os-example-http-server

This application demonstrates how to run an HTTP server on an mbed OS 6 device.(updated for better reliability, efficiency and scalability.)

Request parsing is done through [nodejs/http-parser](https://github.com/nodejs/http-parser).

## To build
#### Disclaimer:
On Windows, You'll have to install:
* python (Python 3.7.9)
* git (git version 2.28.0.windows.1)
* gcc (gcc.exe (GCC) 7.0.1 20170205 (experimental))
* g++ (g++.exe (GCC) 7.0.1 20170205 (experimental))
* mbed-cli( using pip install mbed-cli, don't use mbed-cli-installer.exe, it doesn't work.)...  before moving ahead.

#### Build Steps:
(Done After forking from [here](https://github.com/janjongboom/mbed-os-example-http-server).)
1. Deleted ``mbed_app.json``. (Believe me, its going to save you a ton of time. Atleast, if you are using K64F.)
2. Build the project  using mbed CLI.
   * run "mbed new .'' in the directory of the cloned repo.
   * Set the Target using "mbed target K64F"
   * Set the Toolchain using "mbed toolchain GCC_ARM"
   * Export to MCUXpresso IDE using "mbed export -i mcuxpresso"
     * MCUXpresso on Windows never compiles mbed projects successfully, use it only for editing/developing.
3. Add the mbed-http library using "mbed add http://os.mbed.com/teams/sandbox/code/mbed-http/"
4. Compile using "mbed compile"
5. Flash the project to your development board.
   * Can Compile and Flash using "mbed compile -f"
6. Attach a serial monitor to your board to see the debug messages.
   * Can see the serial messages using "mbed sterm -b <\baud>"

## Tested on

* K64F with Ethernet.

But every networking stack that supports the [mbed OS 5 NetworkInterface API](https://docs.mbed.com/docs/mbed-os-api-reference/en/latest/APIs/communication/network_sockets/) should work.

ps: Readme.md edited using [This Useful Online Tool](https://jbt.github.io/markdown-editor/)
