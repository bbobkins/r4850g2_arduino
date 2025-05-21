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
    - Add rampdown as current reduces went batteries approach charge, a large load in that state can bog down the generator
  
  ## Requirements:
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
    -6x 6.35mm Adhesive PCB Standoffs - (Jaycar HP0760)
    -(OPTIONAL) DC-DC Buck Step Down Adjustable Converter 4.5-60V input to 3-32v out

  ## Build Instructions:
    - PCB Prep:
      * Drill 6x 4mm holes for PCB standoffs (2mm for accuracy)
      * repair the power rails with bodge wires if damaged by drilling
    - Nano:
      * Wire GND to ground (Inner) rail of PCB 
      * Wire 5V to power (outer) rail of PCB 
    - Switches:
      * Place switches on PCB under the oled with half the legs on the ground (Inner) rail
      * Wire the red switch to Nano D4
      * Wire the green switch to Nano D3
    - Display:
      * Wire GND to ground rail
      * Wire VCC to power rail
      * Wire SCL to Nano A5
      * Wire SDA to Nano A4
    - Can Module:
      * Install jumper on J1 of can bus module
      * Wire GND to ground rail
      * Wire VCC to power rail
      * Wire CS to Nano D10
      * Wire S0 to Nano D12
      * Wire SI to Nano D11
      * Wire SCK to Nano D13
      * Wire INT to Nano D2
    - Optional Power module:
      * Set output voltage of DC buck converter module to around 10v (7-12v)
      * Wire out- of DC buck converter module to ground (Inner) rail
      * Wire out+ of DC buck converter module to VIN pin of Arduino Nano 


  ## Usage:
    - Set current using red or green buttons
    - Set Max Voltage by holding both buttons until menu is displayed then only hold 1 of the buttons to set value
    - Settings are stored in EEPROM and will survive power cycles
    - Max Voltage appears to be 57.3 with the R4850G2 becoming unstable above that value

    

   ## Troubleshooting:
    - No CAN input detected: check Jumper J1 is installed on CAN Module, check wireing to R4850G2

      
  ## PICTURES:
  PCB Example:
    [r4850g2_pcb.jpg](https://github.com/bbobkins/r4850g2_arduino/blob/main/r4850g2_pcb.jpg)


  ## PINOUT:
  See:
  -[r4850g2_arduino.png](https://github.com/bbobkins/r4850g2_arduino/blob/main/r4850g2_arduino.png)
