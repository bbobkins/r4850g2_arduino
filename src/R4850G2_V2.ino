/*  r4850g2 arduino - Huawai R4850G2 rectifier canbus control - Generator friendly
//  Forked from
//  https://github.com/haklein/r4850g2_arduino 
//  Repository 
//  https://github.com/bbobkins/r4850g2_arduino
*/


//CAN
#include <CAN.h>

//yeolde FlashStorage
#include <EEPROM.h>

//Shared libraries
#include "R4850G2.h" //<R4850G2.h> 

struct RectifierParameters rp;

#define DEBUG false

#define MAX_CURRENT_MULTIPLIER        20.0

#define R48xx_DATA_INPUT_POWER        0x70
#define R48xx_DATA_INPUT_FREQ         0x71
#define R48xx_DATA_INPUT_CURRENT      0x72
#define R48xx_DATA_OUTPUT_POWER       0x73
#define R48xx_DATA_EFFICIENCY         0x74
#define R48xx_DATA_OUTPUT_VOLTAGE     0x75
#define R48xx_DATA_OUTPUT_CURRENT_MAX 0x76
#define R48xx_DATA_INPUT_VOLTAGE      0x78
#define R48xx_DATA_OUTPUT_TEMPERATURE 0x7F
#define R48xx_DATA_INPUT_TEMPERATURE  0x80
#define R48xx_DATA_OUTPUT_CURRENT     0x81
#define R48xx_DATA_OUTPUT_CURRENT1    0x82


#define R48xx_VOLTAGE         0x00
#define R48xx_DEFAULT_VOLTAGE 0x01
#define R48xx_OV_PROTECTION   0x02
#define R48xx_CURRENT         0x03
#define R48xx_DEFAULT_CURRENT 0x04


const uint32_t RESET_INTERFACE = 10000;  // 10 seconds

const int BUTTON_PIN_UP = 3;    //PIN D3
const int BUTTON_PIN_DOWN = 4;  //PIN D4

//float Current = 1.0f;
//float maxCurrent = 5.0f;

//Voltage is hard coded for now, TODO: make a menu to all this to change
//Set the voltage for your battery chemistry then, 
//float maxVoltage = 50.0f;          //Maximum appears to be 57.3, any higher and the unit becomes flakey
//float defaultVoltage = 57.0f;



void setup() {
  // put your setup code here, to run once:


  //Input button setup
  pinMode(BUTTON_PIN_UP, INPUT_PULLUP);
  pinMode(BUTTON_PIN_DOWN, INPUT_PULLUP);


  //Serial port setup
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();
  // Wire.setClock(400000L); # might be required on some boards
  oled.begin(&Adafruit128x64, SCREEN_ADDRESS);
  //  oled.begin(&SH1106_128x64, SCREEN_ADDRESS); # use this instead on AZDelivery v3 nano board due to space constraints
  oled.setFont(menuFont);

 //read CURRENT setting from flash memory
  float currentSet = 5.0f;
  EEPROM.get(0, currentSet);

  //Set default current in flash memory
  if(isnan(currentSet) ||  currentSet < 0.0f || currentSet > 55.0f)
  {
    currentSet = 5.0f;
    EEPROM.put(0, currentSet);
  }
  rp.currentSet = currentSet;

  //read VOLTAGE setting from flash memory
  float voltageSet = 50.0f;
  EEPROM.get(4, voltageSet);

  //Set default current in flash memory
  if(isnan(voltageSet) ||  voltageSet < 42.0f || voltageSet > 57.3f)
  {
    voltageSet = 50.0f;
    EEPROM.put(4, voltageSet);
  }
  rp.voltageSet = voltageSet;

  Serial.println();
  Serial.println("###############################################################");
  Serial.println("## R4850G2 Inverter");
  Serial.println("###############################################################");

  oled.clear();
  oled.println("R4850G2 Inverter");
  oled.println("");
  oled.println("Version: 0.2");
  oled.println("");

  delay(3000); //Show startup screen


  CAN.setClockFrequency(8E6); // 8 MHz for Adafruit MCP2515 CAN Module

  bool canOk = false;
  if (CAN.begin(125E3)) // 125kbit
  {    
    CAN.onReceive(onCANReceive);
    canOk = true;
  }


  if(canOk){
    Serial.println("Starting...");
    oled.println("Starting...");
  } else {
    Serial.println("ERROR: CAN receiver offline! Aborting.");
    oled.println("ERROR: CAN receiver");
    oled.println("   offline! Aborting.");
    while (1);
  }

  //persistant defaults 
  r4850_set(rp.voltageSet, R48xx_DEFAULT_VOLTAGE);
  delay(250);
   r4850_set(rp.currentSet,  R48xx_DEFAULT_CURRENT);  //Offline values
  delay(250);

  r4850_request_data();
  delay(250);

  //Looks like a reboot, so continue on where we left off
  if(rp.output_current > 0) 
  {
    rp.current = (int)rp.output_current;
    Serial.print("Reboot detected, current set = "); 
    Serial.println(rp.current);  
  }


  //#if(DEBUG) 
      Serial.println("INFO: CAN setup done."); 
      Serial.print("currentSet = ");
      Serial.print(rp.currentSet);  
      Serial.println("A");
  //#endif 

  oled.clear();
}

bool doVoltageInterface = false;

void loop() {

  if(digitalRead(BUTTON_PIN_UP) == 0 || digitalRead(BUTTON_PIN_DOWN) == 0)
  {
    doInterface(&rp); 
    delay(50);   //Interface speed, increase delay for slower
  } 
  else
  {
    resetInterface();

    //Serial.println(F("req data..."));
    r4850_request_data();

    rp.status = updateStatus(rp);
    
    if(updateCurrentValues(&rp))
    {
        Serial.println("### MAKE A CHANGE ###");

        //Update 
        r4850_set(rp.voltageSet, R48xx_VOLTAGE); //57.3 max?
        delay(250);
        r4850_set(rp.current, R48xx_CURRENT);
        delay(250);

    }
    

    delay(500);  //Normal delay
    delay(1000); // for serial debugging


    doDisplay(rp, true);

  }

}



void doInterface(struct RectifierParameters *rp) {


  if(digitalRead(BUTTON_PIN_DOWN) == 0 && digitalRead(BUTTON_PIN_UP) == 0)
  {
    doVoltageInterface = true;
  }

  if(doVoltageInterface)
  {
    if(digitalRead(BUTTON_PIN_DOWN) == 0 )
    {
      rp->voltageSet = rp->voltageSet - 0.1f;
      if(rp->voltageSet < 42.0f) rp->voltageSet = 42.0f;
    }

    if(digitalRead(BUTTON_PIN_UP) == 0)
    {
        rp->voltageSet = rp->voltageSet + 0.1f;
        if(rp->voltageSet > 57.3f) rp->voltageSet = 57.3f; //57.3 max?
    }
    //oled.clear();
    oled.setCursor(0,0);

    Serial.print("Max Voltage: "); oled.print("Max Voltage: ");       
    Serial.print(rp->voltageSet); oled.print(rp->voltageSet);      
    Serial.println("V"); oled.println("V      ");   
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");

    //Write the setting to the EEPROM for next startup
    EEPROM.put(4, rp->voltageSet);
    
  }
  else {


    if(digitalRead(BUTTON_PIN_DOWN) == 0 )
    {
      rp->currentSet = rp->currentSet - 1.0f;
      if(rp->currentSet < 0) rp->currentSet = 0;
    }

    if(digitalRead(BUTTON_PIN_UP) == 0)
    {
        rp->currentSet = rp->currentSet + 1.0f;
        if(rp->currentSet > 55) rp->currentSet = 55;
    }
    //oled.clear(); 
    oled.setCursor(0,0);

    Serial.print("Set Current: "); oled.print("Set Current: ");       
    Serial.print(rp->currentSet); oled.print(rp->currentSet);      
    Serial.println("A"); oled.println("A      ");   
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");

    //Write the setting to the EEPROM for next startup
    EEPROM.put(0, rp->currentSet);

  }


}


void resetInterface()
{
    //Reset the interface every 10sec
    static uint32_t previousMillis;
    if (millis() - previousMillis >= RESET_INTERFACE) {
        doVoltageInterface = false;
        previousMillis += RESET_INTERFACE;
    }

}





int r4850_set(float val, uint8_t command)
{
  uint16_t value = val * 1024;

  if(command == R48xx_CURRENT || command == R48xx_DEFAULT_CURRENT)
    value = val * MAX_CURRENT_MULTIPLIER;


  #if(DEBUG)      
    Serial.print(F("Command: "));
    Serial.println(command);
    Serial.print(F("Value: "));
    Serial.println(val);
  #endif   

  uint8_t data[8];
  data[0] = 0x01;
  data[1] = command;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = (value & 0xFF00) >> 8;
  data[7] = value & 0xFF;

  #if(DEBUG)      
    //Serial.println("data = %" PRIu32 "\n", data);
    Serial.print(F("Data: "));
    print_r4850_data(data);
  #endif   


  return sendCAN(0x108180FE , data, 8, false);

  Serial.println("sendCAN complete.");
}


char print_r4850_data(uint8_t* data)
{
  for(int x = 0; x < 8; x++)
  {
    Serial.print(" ");
    Serial.print(data[x]);
  }
  Serial.println();
} 

int r4850_request_data()
{
  uint8_t data[8];
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;

  return sendCAN( 0x108040FE , data, 8, true);
}

int sendCAN(uint32_t msgid, uint8_t *data, uint8_t len, bool rtr)
{
  CAN.beginExtendedPacket(msgid, len, rtr);
  CAN.write(data, len);

  if (!CAN.endPacket()) {
    //#if(DEBUG) 
      Serial.println("CAN Error: Send packet failed.");
    //#else

      //oled.setFont(statusFont);
      //oled.clear();
      oled.setCursor(0,0);

      oled.println("CAN Error:           ");
      oled.println("  Send packet failed.");
      oled.println("                     ");
      oled.println("                     ");
      oled.println("                     ");
      oled.println("                     ");
      oled.println("                     ");
      oled.println("                     ");
      oled.println("                     ");

    //#endif
    return 0;
  }
  return 1;
}

int r4850_ack(uint8_t *frame)
{
  bool error = frame[0] & 0x20;
  uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

  switch (frame[1]) {
    case 0x00:
      #if(DEBUG)
        Serial.print(F("Setting on-line voltage: "));
      #endif
      break;
    case 0x01:
      #if(DEBUG)
        Serial.print(F("Setting off-line voltage: "));
      #endif
      break;
    case 0x02:
      #if(DEBUG)   
        Serial.print(F("Setting OVP: "));
      #endif
      break;
    case 0x03:
      #if(DEBUG) 
        Serial.print(F("Setting on-line current: "));        
      #endif
      break;
    case 0x04:
      #if(DEBUG) 
        Serial.print(F("Setting off-line current: "));  
      #endif
      break;
    default:
      #if(DEBUG)
        Serial.print(F("Setting unknown parameter: "));  
      #endif

      break;
  }
  #if(DEBUG)
    Serial.println(error ? F("ERROR") : F("SUCCESS"));
  #endif
}

void onCANReceive(int packetSize)
{
  if (!CAN.packetExtended())
    return;
  if (CAN.packetRtr())
    return;

  uint32_t msgid = CAN.packetId();
  uint8_t data[packetSize];

  CAN.readBytes(data, sizeof(data));


  switch (msgid & 0x1FFFFFFF) {

    case 0x1081407F:
      #if(DEBUG)   
       Serial.print("Data frame: ");
      #endif

       
      r4850_data((uint8_t *)&data, &rp);
      #if(DEBUG)   
       Serial.println();
      #endif

      break;
    case 0x1081407E:
      /* Acknowledgment */
      #if(DEBUG)         
       Serial.println("Ack frame - 0x1081407E");
      #endif       
      break;
    case 0x1081D27F:
      #if(DEBUG)        
       Serial.println("Description - 0x1081D27F");
      #endif           
       //r4850_description((uint8_t *)&data); <-- old version?
      break;
    case 0x1081807E:
      #if(DEBUG)        
       Serial.println("Ack frame - 0x1081807E");
      #endif         
      r4850_ack((uint8_t *)&data);
      break;
    case 0x1001117E:
      #if(DEBUG)         
       Serial.println("Amp hour frame - 0x1001117E");
      #endif     
      break;
    case 0x100011FE:
      #if(DEBUG)       
       Serial.println("unknown frame - 0x100011FE - Normally  00 02 00 00 00 00 00 00 ");
      #endif        
      break;
    case 0x108111FE:
      #if(DEBUG)    
       Serial.println("unknown frame - 0x108111FE - Normally 00 03 00 00 00 0s 00 00, s=1 when output enabled ");
      #endif        
      break;
    case 0x108081FE:
      #if(DEBUG)      
       Serial.println("unknown frame - 0x108081FE - Normally 01 13 00 01 00 00 00 00 ");
      #endif         
      break;

    default:
      #if(DEBUG)      
        Serial.println(F("ERROR: Unknown Frame!"));
      #endif 
      break;
  }
}


int r4850_data(uint8_t *frame, struct RectifierParameters *rp)
{
  uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

  switch (frame[1]) {
    case R48xx_DATA_INPUT_POWER:
      rp->input_power = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_FREQ:
      rp->input_frequency = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_CURRENT:
      rp->input_current = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_POWER:
      rp->output_power = value / 1024.0;
      break;

    case R48xx_DATA_EFFICIENCY:
      rp->efficiency = (value / 1024.0) * 100;
      break;

    case R48xx_DATA_OUTPUT_VOLTAGE:
      rp->output_voltage = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT_MAX:
      //rp->max_output_current = value / MAX_CURRENT_MULTIPLIER;
      rp->output_current = value / MAX_CURRENT_MULTIPLIER;
      break;

    case R48xx_DATA_INPUT_VOLTAGE:
      rp->input_voltage = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_TEMPERATURE:
      rp->output_temp = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_TEMPERATURE:
      rp->input_temp = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT1:
      #if(DEBUG)
        Serial.println("Output Current(1) %.02fA", value / 1024.0);
        printf("Output Current(1) %.02fA\r\n", value / 1024.0);
      #endif
      rp->output_current = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT:
      rp->output_current = value / 1024.0;

      /* This is normally the last parameter received. Print */
      //doPrint = true;
      break;

    default:
      #if(DEBUG)
        Serial.println("Unknown parameter 0x%02X, 0x%04X",frame[1], value);
        printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
      #endif
      break;

  }
  //rp->validData = true;
}


