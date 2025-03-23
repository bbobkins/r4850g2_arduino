//CAN
#include <CAN.h>

//OLED
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

//yeolde FlashStorage
#include <EEPROM.h>

//STATUS
//
//CAN ERR
//DC DISCONNECT
//AC DISCONNECT
//RAMP UP
//CHARGE
//RAMP DOWN
//IDLE

enum Status {startup=0,comsErr=1,dcDiscon=2,acDisconn=3,rampUp=4,bulkCharge=5,topOff=6,rampDown=7,idle=8};

Status status = startup;

struct RectifierParameters
{
  //bool validData = false;

  float input_voltage; 
  float input_frequency;
  float input_current;
  float input_power;
  float input_temp;
  float efficiency;
  float output_voltage;
  float output_current;
  float max_output_current;
  float output_power;
  float output_temp;
  float amp_hour;

  //float temp;
};

struct RectifierParameters rp;


#define DEBUG false

#define MAX_CURRENT_MULTIPLIER    20.0

#define R48xx_DATA_INPUT_POWER    0x70
#define R48xx_DATA_INPUT_FREQ   0x71
#define R48xx_DATA_INPUT_CURRENT  0x72
#define R48xx_DATA_OUTPUT_POWER   0x73
#define R48xx_DATA_EFFICIENCY   0x74
#define R48xx_DATA_OUTPUT_VOLTAGE 0x75
#define R48xx_DATA_OUTPUT_CURRENT_MAX 0x76
#define R48xx_DATA_INPUT_VOLTAGE  0x78
#define R48xx_DATA_OUTPUT_TEMPERATURE 0x7F
#define R48xx_DATA_INPUT_TEMPERATURE  0x80
#define R48xx_DATA_OUTPUT_CURRENT 0x81
#define R48xx_DATA_OUTPUT_CURRENT1  0x82


#define R48xx_VOLTAGE 0x00
#define R48xx_DEFAULT_VOLTAGE 0x01
#define R48xx_OV_PROTECTION 0x02
#define R48xx_CURRENT 0x03
#define R48xx_DEFAULT_CURRENT 0x04


SSD1306AsciiWire oled;

#define menuFont font5x7 //X11fixed7x14
#define fontW 7  // 7
#define fontH 15   // 15

#define statusFont Adafruit5x7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

const int BUTTON_PIN_UP = 3;    //PIN D3
const int BUTTON_PIN_DOWN = 4;  //PIN D4

float Current = 1.0f;
float maxCurrent = 5.0f;

//Voltage is hard coded for now, TODO: make a menu to all this to change
//Set the voltage for your battery chemistry then, 
//UNCOMMENT the lines below to allow the code to compile
float voltage = //57.3f;          //Maximum appears to be 57.3, any higher and the unit becomes flakey
float defaultVoltage = //57.0f;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN_UP, INPUT_PULLUP);
  pinMode(BUTTON_PIN_DOWN, INPUT_PULLUP);


  Serial.begin(115200);
  while (!Serial);

  //read setting from flash memory
  EEPROM.get(0, maxCurrent);

  //Set default current in flash memory
  if(isnan(maxCurrent) ||  maxCurrent < 0.0f || maxCurrent > 55.0f)
  {
    maxCurrent = 5.0f;
    EEPROM.put(0, maxCurrent);
  }



  Wire.begin();
  // Wire.setClock(400000L); # might be required on some boards
  oled.begin(&Adafruit128x64, SCREEN_ADDRESS);
  //  oled.begin(&SH1106_128x64, SCREEN_ADDRESS); # use this instead on AZDelivery v3 nano board due to space constraints
  oled.setFont(menuFont);
  oled.clear();
  oled.println("R4850G2 Inverter");
  oled.println("");

  delay(5000); //Show startup screen


  CAN.setClockFrequency(8E6); // 8 MHz for Adafruit MCP2515 CAN Module

  bool canOk = false;
  if (CAN.begin(125E3)) // 125kbit
  {    
    CAN.onReceive(onCANReceive);
    canOk = true;
  }


  if(canOk){
    oled.println("Starting..");
    delay(1000); //Show startup screen
  } else {
    Serial.println("ERROR: CAN receiver offline! Aborting.");
    oled.println("ERROR: CAN receiver");
    oled.println("   offline! Aborting.");
    while (1);
  }


  //persistant defaults
  r4850_set(defaultVoltage, R48xx_DEFAULT_VOLTAGE);
  delay(250);
   r4850_set(10.0f,  R48xx_DEFAULT_CURRENT);
  delay(250);


  //#if(DEBUG) 
      Serial.println("###############################################################");
      Serial.println("INFO: CAN setup done."); 
      Serial.print("maxCurrent = ");
      Serial.print(maxCurrent);  
      Serial.println("A");

  //#endif 


}




void loop() {



  if(digitalRead(BUTTON_PIN_UP) == 0 || digitalRead(BUTTON_PIN_DOWN) == 0)
  {
    doInterface(); 
    delay(500); 
  } 
  else
  {
    doDisplay();
    delay(5000);  //5sec

    // put your main code here, to run repeatedly:
    //r4850_set(Current, R48xx_CURRENT);
    //delay(250);

    //r4850_set(58.3f, R48xx_OV_PROTECTION);
    //delay(250);

    if(Current < maxCurrent && status == startup)
    {    
      r4850_set(voltage, R48xx_VOLTAGE); //57.3 max?
      delay(250);
    }


    if(Current < maxCurrent && status == rampUp)
    {
      Current = Current + 5.0f;
      if(Current > maxCurrent) Current = maxCurrent;

        r4850_set(Current, R48xx_CURRENT);
        delay(250);
    }
    else if(status == rampUp)
    {
      status = bulkCharge;
    }


  }





}







void doDisplay() {



  if(r4850_request_data()) {
      oled.clear();



      if(rp.input_voltage < 220)
      {
        status = acDisconn;
        Current = 1.0f;  //Start over again if conms come back up
      }
      else if(rp.input_voltage >= 220)
        status = rampUp;

      Serial.print("STATUS: "); oled.print("STATUS: ");
      Serial.println(getStatus()); oled.println(getStatus());

      Serial.print(F("SET=")); oled.print(F("SET="));
      Serial.print(rp.max_output_current); oled.print(rp.max_output_current);
      Serial.println(F("A")); oled.println(F("A"));

      Serial.print(F("T: ")); oled.print(F("T: "));
      Serial.print(rp.input_temp); oled.print(rp.input_temp);
      Serial.print(F("C ")); oled.print(F("C "));
      Serial.print(rp.output_temp); oled.print(rp.output_temp);
      Serial.println(F("C ")); oled.println(F("C "));

      Serial.print(F("I: ")); oled.print(F("I: "));
      Serial.print(rp.input_voltage); oled.print(rp.input_voltage);
      Serial.print(F("V ")); oled.print(F("V "));
      Serial.print(rp.input_current); oled.print(rp.input_current);
      Serial.println(F("A ")); oled.println(F("A "));
      Serial.print(rp.input_power); oled.print(rp.input_power);
      Serial.print(F("W ")); oled.print(F("W "));
      Serial.print(rp.input_frequency); oled.print(rp.input_frequency);
      Serial.println(F("hz")); oled.println(F("hz"));

      Serial.print(F("O: ")); oled.print(F("O: "));
      Serial.print(rp.output_voltage); oled.print(rp.output_voltage);
      Serial.print(F("V ")); oled.print(F("V "));
      Serial.print(rp.output_current); oled.print(rp.output_current);
      Serial.println(F("A ")); oled.println(F("A "));
      Serial.print(rp.output_power); oled.print(rp.output_power);
      Serial.print(F("W ")); oled.print(F("W "));
      Serial.print(rp.efficiency); oled.print(rp.efficiency);
      Serial.println(F("%")); oled.println(F("%"));
      Serial.println();

  } else
  {
    status = comsErr;
    Current = 1.0f;  //Start over again if conms come back up
  }

}

void doInterface() {

  Serial.print(" maxCurrent: ");
  Serial.println(maxCurrent);

  if(digitalRead(BUTTON_PIN_DOWN) == 0)
  {
    maxCurrent = maxCurrent - 1;
    if(maxCurrent < 0) maxCurrent = 0;
  }

  if(digitalRead(BUTTON_PIN_UP) == 0){
      maxCurrent = maxCurrent + 1;
      if(maxCurrent > 55) maxCurrent = 55;
  }

  oled.clear();   
  oled.setCursor(40,30);     
  oled.print(maxCurrent);      
  oled.println("A");

  //Write the setting to the EEPROM for next startup
  EEPROM.put(0, maxCurrent);
}




String getStatus(){

  switch(status){
    case 0: return("Startup.");
    case 1: return("Can Error.");
    case 2: return("DC Discon.");
    case 3: return("AC Discon.");
    case 4: return("Ramp Up.");
    case 5: return("Bulk Charge.");
    case 6: return("Top Up.");
    case 7: return("Ramp Down.");
    case 8: return("Idle.");

    return("Unknown.");
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

      oled.setFont(statusFont);
      oled.clear();
      oled.println("CAN Error:");
      oled.println("  Send packet failed.");

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
      rp->max_output_current = value / MAX_CURRENT_MULTIPLIER;
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
        Serial.println("R48xx_DATA_OUTPUT_CURRENT1");
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
        printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
      #endif
      break;

  }
  //rp->validData = true;
}

