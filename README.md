# greenhouse_MakerSpace
Simple project for managing DIY greenhouse via ESP8266 and RaspberryPi

# Table of contents
- [📜 CODE](#code)
  - [Description of a code](#description)
  - [Prerequisities](#prerequisities)
  - [To-Do](#to-do)
- [🛠️ HARDWARE](#hardware)
  - [❌What's wrong? ✅What's good?](#whats-wrong-whats-good)
    - [Humidifier](#humidifier-whats-good-whats-wrong)
    - [Soil sensor](#soil-sensor-whats-good-whats-wrong)
    - [Temperature and humidity sensor](#dht-whats-good-whats-wrong)
    - [Board ESP8266](#board-whats-good-whats-wrong)
  - [🛒 Shoplist](#shoplist)
  
# Code
## Description
There are two versions of this program:
### 🐣 ESP8266_grennhouse.ino 
It is a simple program to run on greenhouse, but all the sensors are hardcoded and any addition, change or subtraction of sensors is limited and requires rewriting whole code. Code successfuly posts all sensors values on MQTT topics.
### 🚀 extended_ESP8266_greenhouse.ino 
It has similar functionality as above program, but it futures: 
- config file that allows to manage amount and type of sensors/devices without rewriting the main code
- It also can receive MQTT messages to change on/off in fans or humidifier.
- As you can see in config.json file, it also features the division of greenhouse into sections. Greenhouse consists of zones and zones consist of flower pots. Therefore it is possible to define desired parameters of temperature, humidity etc. per flower pot, zone or whole greenhouse. For now it will be only possible to toggle or read sensors for whole greenhouse or individual flower pots, as there is no physical barier in current Greenhouse project to divide whole greenhouse into zones.

**There is no need for SD card in board to implement config.json file,** but to upload config file to the board you need to install a tool in ArduinoIDE to upload files - [tutorial](https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/) 
*This extended code has been tested with config.json file and it works. It may take a few seconds to boot.*
### 📌 Other comments
- For now it has not been tested with server side being prepared by Milosz on Raspberry Pi.
- For now it also doesn't support idea proposed by Miłosz, it is maintenance of deisred parameters' values (temperature/humidity). I was thinking on writing this part on server side, so board will only execute instructions from server and post the values of sensors via MQTT. Downside of this solution is that if the board will loose connection to server it will stop adjusting desired parameters. It is hard for me at this point to further enhance the code, since I know little about programming boards in C++.


## Prerequisities
In Adriuino IDE install in Sketch->Include Library->Manage Librares following libraries: "Adafruit Unified Sensor", "DHT sensor library". 
From tutorial https://randomnerdtutorials.com/esp8266-nodemcu-mqtt-publish-dht11-dht22-arduino/ for MQTT we need to also install two libraries:
- https://github.com/me-no-dev/ESPAsyncTCP/archive/master.zip
- https://github.com/marvinroger/async-mqtt-client/archive/master.zip
after downloading .zip include it in sketch->Include Library->Add .ZIP library


## TO DO
- [ ] Add calibration of soil sensor to the code and convert it into percents.
- [ ] Add function to control on/off state of power PINs based on subscribed MQTT topic (in config.json defined as type:toggle)
- [ ] Design a way to maintain given parameters (ex. temperature) at the same level by switching on/off fans and humidifier
- [ ] Update READEME.md and add section on prerquisites

---

# Hardware

## What's wrong, what's good?
<a name="humidifier-whats-good-whats-wrong"></a>
1. Humidifier
   - ❌ What's wrong: Piezoelectric humidifier doesn't work. Powering it via USB cable 5V doesn't even turn on LED on it. Tried multiple power sources, it never lit up for even a second. There is also no effect of humidification after plugging it in.
   - ✅ What's good: It has big USB-C port but also two smaller holes for 5V VCC and GND that will allow it to be powered via board and thus toggled on/off via the board itself with no need for external power source and switches.

<a name="soil-sensor-whats-good-whats-wrong"></a>
2. Soil sensor
   - ❌ What's wrong: Calibration and Corosion
     * **Calibration** It is not calibrated and outputs int in range 0-1024 (already added to [TO-DO](#to-do)).
     After my tests I have obtained results as:
        - ~100 - salted tap water
        - ~150 - pure water
        - =1024 - air
      * **Corosion** It is cheap non-waterproof sensor. That rises follwing issues: to water plants you need to be careful not to water the sensitive parts of electronics, it will get used up after a while due to corosion. An alternative is to buy already waterproofed with isolated 1.5 meters long cable sensor. Unfortunately it is expensive. See [shoplist section](#shoplist).

   - ✅ What's good: It works now and the need to update it can be postponed for future

<a name="dht-whats-good-whats-wrong"></a>
3. DHT - humidity and temperature sensor
   - ❌ What's wrong: There is a need for [longer cables](#shoplist) to connect it.
   - ✅ What's good: It works

<a name="board-whats-good-whats-wrong"></a>
4. ESP8266 Board (now I use NodeMCU v3 board with ESP8266)
   - ❌ What's wrong: It has limited power 3.3V outputs and limited PINs for data read, therefore probably more than one board will be needed to satisfy demand for multiple analog soil sensors. (With MQTT broker on Raspberry Pi, there will be no trouble with adding new ESP boards - slight changes in config lines in code will do the trick)
   - ✅ What's good: It works

<a name="fans-whats-good-whats-wrong"></a>
4. Fans
   - ❌ What's wrong: The fan we have is powered with 12V. We need to connect fans' cables to [12V power supply](#shoplist) and board. I need help with that.
   - ✅ What's good: Nothing as for now.


## Shoplist
1. Humidifier - probably the one we received is malfunctioned, maybe let's buy one more and see if the problem persists
2. Soil sensor (to discuss which or if needed):
   - [The expensive sensor that is fully waterproof and corosionproof for 67 PLN](https://botland.com.pl/gravity-czujniki-pogodowe/17377-gravity-wodoodporny-analogowy-czujnik-wilgotnosci-gleby-v20-dfrobot-sen0308-6959420917068.html)
   - [Cheaper non-fully-waterproof but corosionproof for 28 PLN](https://botland.com.pl/gravity-czujniki-pogodowe/10305-dfrobot-gravity-analogowy-czujnik-wilgotnosci-gleby-odporny-na-korozje-sen0193-6959420910434.html)

    And for future use - their manulas/tutorials respectively: [SEN0308](https://wiki.dfrobot.com/Waterproof_Capacitive_Soil_Moisture_Sensor_SKU_SEN0308) and [SEN0193](https://wiki.dfrobot.com/Capacitive_Soil_Moisture_Sensor_SKU_SEN0193)

3. [Power Supply 12V](https://botland.com.pl/szukaj?s=12v%20zasilacz) (to discuss)
4. Long cables to connect sensors to the board (are they available in makerspace?)
5. More boards with ESP8266 possibly (not sure if needed until we connect all sensors required for flower pots, so we need to know how many flower pots we gonna have)

