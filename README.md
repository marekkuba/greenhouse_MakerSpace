# greenhouse_MakerSpace
Simple project for managing DIY greenhouse via ESP8266 and RaspberryPi

# Description
There are two versions of this program:
## ESP8266_grennhouse.ino 
It is a simple program to run on greenhouse, but all the sensors are hardcoded and any addition, change or subtraction of sensors is limited and requires rewriting whole code.
## extended_ESP8266_greenhouse.ino 
It has the same functionality as above program, but it futures config file that allows to manage amount and type of sensors/devices without rewriting the main code
**There is no need for SD card in board to implement config.json file,** but to upload config file to the board you need to install a tool in ArduinoIDE to upload files - [tutorial](https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/) 
*This extended code has been tested with config.json file and it works. It may take a few seconds to boot.*

---Move to prerequisities section---
Also (for future prerequisites section): In Adriuino IDE install in Sketch->Include Library->Manage Librares following libraries "Adafruit Unified Sensor", "DHT sensor library". From tutorial https://randomnerdtutorials.com/esp8266-nodemcu-mqtt-publish-dht11-dht22-arduino/ for MQTT we need to also install two libraries:
- https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip
- https://github.com/marvinroger/async-mqtt-client/archive/master.zip
after downloading .zip include it in sketch->Include Library->Add .ZIP library
--------------------------------------

# TO DO
- [ ] Add calibration of soil sensor to the code and convert it into percents.
- [ ] Add function to control on/off state of power PINs based on subscribed MQTT topic (in config.json defined as type:toggle)
- [ ] Update READEME.md and add section on prerquisites


# What's wrong, what's good? (about sensors, not code)
## Humidifier
### What's wrong
Piezoelectric humidifier doesn't work. Powering it via USB cable 5V doesn't even turn on LED on it. Tried multiple power sources, it never lit up for even a second. There is also no effect of humidification after plugging it in. 
### What's good
It has big USB-C port but also two smaller holes for 5V VCC and GND that will allow it to be powered via board and thus toggled on/off via the board itself with no need for external power source and switches.

## Soil sensor
### What's wrong
#### Calibration
It is not calibrated and outputs int in range 0-1024 (where 1024 means high resistance - dry enviroment and 0 means zero resistance).
After my tests I have obtained results as:
- ~100 - salted tap water
- ~150 - pure water
- =1024 - air.
It therefore requires additional calibrating and conversion into percents.
#### Corosion
It is cheap non-waterproof sensor. That rises follwing issues: to water plants you need to be careful not to water the sensitive parts of electronics, it will get used up after a while due to corosion.
An alternative is to buy already waterproofed with isolated 1.5 meters long cable sensor. Unfortunately it is expensive.

[The expensive sensor that is fully waterproof and corosionproof for 67 PLN](https://botland.com.pl/gravity-czujniki-pogodowe/17377-gravity-wodoodporny-analogowy-czujnik-wilgotnosci-gleby-v20-dfrobot-sen0308-6959420917068.html)

[Cheaper non-fully-waterproof but corosionproof for 28 PLN](https://botland.com.pl/gravity-czujniki-pogodowe/10305-dfrobot-gravity-analogowy-czujnik-wilgotnosci-gleby-odporny-na-korozje-sen0193-6959420910434.html)

And for future use - their manulas/tutorials respectively: [SEN0308](https://wiki.dfrobot.com/Waterproof_Capacitive_Soil_Moisture_Sensor_SKU_SEN0308) and [SEN0193](https://wiki.dfrobot.com/Capacitive_Soil_Moisture_Sensor_SKU_SEN0193)

### What's good
It works now and the need to update it can be postponed for future

## DHT - humidity and temperature sensor
### What's wrong
There is a need for longer cables to connect it.

## ESP8266 Board
Now I use NodeMCU v3 board with ESP8266
### What's wrong
1. It has limited power 3.3V outputs and limited PINs for data read, therefore probably more than one board will be needed to satisfy demand for multiple analog soil sensors. (With MQTT broker on Raspberry Pi, there will be no trouble with adding new ESP boards - slight changes in config lines in code will do the trick)
### What's good
It works.

