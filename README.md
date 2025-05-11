# Huawai R4850G2 rectifier canbus control - Generator friendly

  ## Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

   https://github.com/haklein/r4850g2_arduino

  ## Goals:
    - Generator friendly with slow current ramp up & ramp down!!
    - Allows generators to remain on ECO mode and prevents bogdown and over current trips
    - Still works well with grid power
    - Removed stuipd click encoder and menu libraries freeing up much memory!!
    - Settings are saved in EEprom memory and persist
    - CAN side of things mostly unchanged
    - Simplified implimentation and use

  ## Todo:
    - Add menu item to change voltage also, hard coded for now 
  
  ## Requirements 
   
   - this has been tested with an Arduino Nano Only
   - libraries used:
      * Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN
      * EEPROM.h - FlashStorage library by Various:
        https://github.com/cmaglie/FlashStorage
      * SSD1306Ascii.h - SSD1306Ascii by Bill Geriman
         https://github.com/greiman/SSD1306Ascii
  ## Parts List:
    
    -1x Compatible Arduino Nano V3 16Mhz With Bootloader
    -1x MCP2515 CAN Controller Bus Module TJA1050 Receiver SPI for Arduino
    -1x 2.54mm Jumper for Pin Header/Circuit Board 
    -1x OLED Display 0.96" 128x64 I2C IIC SSD1306 Arduino 
    -1x Large Breadboard Layout Prototyping Board (Jaycar HP9572)
    -1x Green Snap Action Keyboard Switch - PCB Mount (Jaycar SP0724)
    -1x Red Snap Action Keyboard Switch - PCB Mount (Jaycar SP0720)

  ## Usage

    - To compile code you must comment in the voltage lines {right before void setup()} and set the voltage to your batteries chemistry
    - Max Voltage appears to be 57.3 with the R4850G2 becoming unstable above that value

TODO
    - complete this

      
  ## PICTURES:

TODO


  ## PINOUT
  See:
  - r4850g2_arduino.png
