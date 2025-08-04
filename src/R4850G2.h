//Locate in following folder  '.\Arduino\libraries\R4850G2_V2_Shared'


//yeolde FlashStorage
#include <EEPROM.h>

//OLED
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

SSD1306AsciiWire oled;

#define menuFont font5x7 //X11fixed7x14
#define fontW 7  // 7
#define fontH 15   // 15

#define statusFont Adafruit5x7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


enum Status {startUp=0,comsErr=1,dcDiscon=2,acDisconn=3,rampUp=4,bulkCharge=5,absorption=6,Float=7,idle=8,acVoltRng=9,highTemp=10,hzoutRng=11,coolDown=12};

struct RectifierParameters
{
  //bool validData = false;

  float input_voltage =  0.00F;
  float input_frequency = 0.00F;
  float input_current = 0.00F;
  float input_power = 0.00F;
  float input_temp = 0.00F;
  float efficiency = 0.00F;
  float output_voltage = 0.00F;
  float output_current = 0.00F;
  float output_power = 0.00F;
  float output_temp = 0.00F;
  float amp_hour = 0.00F;

  Status status = startUp;
  float current = 0.0F;
  float currentSet = 0.0F;
  float voltageSet = 0.0F;
  unsigned long timeSinceLastUpdate = 0;
};


String getStatus(Status status) {

  switch(status){
    case 0:  return("Startup     ");
    case 1:  return("Can Error   ");
    case 2:  return("DC Discon   ");
    case 3:  return("AC Discon   ");
    case 4:  return("Ramp Up     ");
    case 5:  return("Bulk Charge ");
    case 6:  return("Absorption  ");
    case 7:  return("Float       ");
    case 8:  return("Idle        ");
    case 9:  return("AC Volt O/R ");
    case 10: return("High Temp   ");
    case 11: return("AC HZ Range ");
    case 12: return("Cooling Down");


    default:
             return("Unknown     ");
  }

}


void doDisplay(struct RectifierParameters rp, bool test) {
  Serial.print("STATUS: "); oled.setCursor(0,0); oled.print("STATUS:     ");
  Serial.println(getStatus(rp.status)); oled.setCursor(45,0); oled.print(getStatus(rp.status));

  Serial.print("SET: "); oled.setCursor(0,1); oled.print("SET:    ");
  Serial.print((int)rp.currentSet);  oled.setCursor(30,1); oled.print((int)rp.currentSet);
  Serial.print("A "); oled.print("A ");

  //if(test)
  //{
    Serial.print("RT: "); oled.setCursor(50,1); oled.print("RT:    ");
    Serial.print((int)rp.current);  oled.setCursor(70,1); oled.print((int)rp.current);
    Serial.print("A "); oled.print("A       ");
 // }  

  Serial.println("");
 
  Serial.print("T: "); oled.setCursor(0,2); oled.print("T: ");
  Serial.print(rp.input_temp); oled.print(rp.input_temp);
  Serial.print("C "); oled.print("C ");
  Serial.print(rp.output_temp); oled.setCursor(70,2); oled.print(rp.output_temp);
  Serial.println("C "); oled.print("C ");

  Serial.print("I: "); oled.setCursor(0,4); oled.print("I: ");
  Serial.print(rp.input_voltage); oled.print(rp.input_voltage);
  Serial.print("V "); oled.print("V ");
  Serial.print(rp.input_current); oled.setCursor(70,4); oled.print(rp.input_current);
  Serial.println("A "); oled.print("A ");
  Serial.print("   "); 
  Serial.print(rp.input_power); oled.setCursor(18,5); oled.print(rp.input_power);
  Serial.print("W "); oled.print("W ");
  Serial.print(rp.input_frequency); oled.setCursor(70,5); oled.print(rp.input_frequency);
  Serial.println("Hz"); oled.print("Hz  ");

  Serial.print("O: "); oled.setCursor(0,6); oled.print("O: ");
  Serial.print(rp.output_voltage); oled.print(rp.output_voltage);
  Serial.print("V "); oled.print("V ");
  Serial.print(rp.output_current);  oled.setCursor(70,6); oled.print(rp.output_current);
  Serial.println("A "); oled.println("A ");
  Serial.print("   "); 
  Serial.print(rp.output_power); oled.setCursor(18,7); oled.print(rp.output_power);
  Serial.print("W "); oled.print("W ");
  Serial.print(rp.efficiency); oled.setCursor(70,7); oled.print(rp.efficiency);
  Serial.println("%"); oled.print("%  ");
  Serial.println();
}

bool updateCurrentValues(struct RectifierParameters *rp)
{
  //Emergency STOP, something not right
  if(rp->current > 0.0f && (rp->status < 0 || rp->status == startUp ||  rp->status == dcDiscon || rp->status == hzoutRng || rp->status == acDisconn || rp->status == acVoltRng ))
  {
    rp->current = 0.0f;
    rp->timeSinceLastUpdate = millis() / 1000;
    return true;
  }

  //TODO ~~~
  //rp->status == highTemp ||


  //Lets wait 1sec to allow the gen to catch up before doing any further updates
  if((millis() / 1000) - rp->timeSinceLastUpdate  < 5)
    return false;

  //Ramp up the current if we are in rampup mode
  //and we still have some way to get to the set current
  //and the output current of the module is catching up
  if(rp->status == rampUp && rp->current < rp->currentSet && rp->output_current >  rp->current - 2.0f)  
  {
    rp->current = rp->current + 1.0f;
    if(rp->current > rp->currentSet) rp->current = rp->currentSet;
    if(rp->current < 0.0f) rp->current = 0.0f;
    rp->timeSinceLastUpdate = millis() / 1000;
    return true;
  }

  //Ramp down the real time current as the battery charges and current into the battery reduces
  //This is to prevent a heavy load switching on and instantly bogging down the generator when in this state
  //If a heavy load is detected then we need to ramp up slowly again
  //Or Ramp down due to user input change
  if(((rp->status == absorption || rp->status == Float) && (rp->output_current + 2 < rp->current)) || rp->current > rp->currentSet) 
  {
    rp->current =  rp->output_current + 1.0f;
    if(rp->current > rp->currentSet) rp->current = rp->currentSet;
    if(rp->current < 0.0f) rp->current = 0.0f;
    rp->timeSinceLastUpdate = millis() / 1000;
    return true;
  }

  return false;
}

//Status updateStatus(enum Status status, struct RectifierParameters rp, float current, float maxCurrent)
//updateStatus(struct RectifierParameters *rp)
Status updateStatus(struct RectifierParameters rp)
{
    //TEMP TOO HIGH
  if(rp.input_temp > 90 || rp.output_temp > 90) 
    return highTemp;

  //DC Voltage to low/disconnected
  if(rp.output_voltage < 40.00F) 
    return dcDiscon;


  if(rp.status == dcDiscon && rp.output_voltage > 40.00F) 
    return startUp;
  else if(rp.status == dcDiscon)
    return dcDiscon;


 //AC Voltage works with 110v and 240v
  if(rp.input_voltage < 80.00F)  
    return acDisconn;



  if(!((rp.input_frequency > 48.00F && rp.input_frequency < 52.00F) || (rp.input_frequency > 58.00F && rp.input_frequency < 62.00F)) )  
    return hzoutRng;



  if(rp.input_voltage > 140 && rp.input_voltage < 220)  //240v low voltage
    return acVoltRng;



  if(rp.input_voltage > 80  && rp.input_voltage < 105)  //110v low voltage  
    return acVoltRng;
 
  if(rp.status == startUp || rp.status == acVoltRng || rp.status == acDisconn || rp.status == hzoutRng || rp.status == highTemp)
    return rampUp;


  if(rp.output_voltage >= rp.voltageSet)
  {
    if(rp.output_current < 2.0F)
      return Float;

    if(rp.output_current > (rp.current - 2.0f) || (rp.status == absorption) )
      return absorption;
  } 
  else if(rp.status == absorption || rp.status == Float)  //Heavy load has come on voltage gas dropped below maxVoltage
    return rampUp;


  if(rp.current == 0 && rp.currentSet == 0)
    return idle;

  if(rp.current >= rp.currentSet && (rp.output_current > rp.current - 2.00f) && (rp.status == rampUp || rp.status == bulkCharge ))
    return bulkCharge;



  if(rp.status == rampUp || rp.status == Float || rp.status == absorption || rp.status == bulkCharge || rp.status == idle) //I THINK !!!!
    return rampUp;


  Serial.println("###############################");
  Serial.print("# Should not get here, status is "); 
  Serial.println(rp.status); 
  Serial.println("###############################");
  delay(1000);

  //Should not get here!!!
    return startUp;
 

}



void getDefaultValues(struct RectifierParameters *rp) {
  float value;

 //read CURRENT setting from flash memory
  EEPROM.get(0, value);

  //Set default current in flash memory
  if(isnan(value) ||  value < 0.0f || value > 55.0f)
  {
    rp->currentSet = 5.0f;
    EEPROM.put(0, rp->currentSet);
  } else {
    rp->currentSet = value;
  }


  //read VOLTAGE setting from flash memory
  EEPROM.get(4, value);

  //Set default current in flash memory
  if(isnan(value) ||  value < 42.0f || value > 57.3f)
  {
    rp->voltageSet = 50.0f;
    EEPROM.put(4, rp->voltageSet);
  }else {
    rp->voltageSet = value;
  }

  //rp->status = startup;
}

