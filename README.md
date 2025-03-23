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
    - Setting are saved in EEprom memory and persist
    - Simple implimentation

  ## Todo:
    - Add menu item to change voltage also, hard coded for now
  
  ## Requirements 
   
   - this has been tested with an Arduino Nano Only
   - libraries used:
      * Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN



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
