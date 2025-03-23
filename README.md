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

 TODO: NEEDS UPDATING encoder removed and 2 buttons added, rest is the same

  ![316427349-3ad49603-3def-4ec2-9e40-f8289db90cfa](https://github.com/haklein/r4850g2_arduino/assets/4569994/0a200d5f-f5de-4887-b59d-5bd5942bd7a0)

  !(https://github.com/bbobkins/r4850g2_arduino/blob/main/r4850g2_arduino.png)
