//Shared libraries
#include "R4850G2.h"

#define DEBUG true

const int BUTTON_PIN = 3;    //PIN D3


enum TestStatus {NominalChargeCycle=0,InputVoltage=1,InputFreq=2,InputCurrent=3,InputTemp=4,OutputVoltage=5,OutputCurrent=6,OutputTemp=7};
TestStatus teststatus = NominalChargeCycle; //InputVoltage;


struct RectifierParameters rp;

//float current = 1.0f;
//float maxCurrent = 5.0f;

//Status status = startup;

int doLoadTest = 0;




void setup() {
  // put your setup code here, to run once:

  //Set defaults to resionable values
  EEPROM.put(0, 55.00f); //maxCurrent
  EEPROM.put(4, 57.3f); //maxVoltage


  //Setup the button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //Serial port setup
  Serial.begin(115200);
  while (!Serial);

  Wire.begin();
  // Wire.setClock(400000L); # might be required on some boards
  oled.begin(&Adafruit128x64, SCREEN_ADDRESS);
  //  oled.begin(&SH1106_128x64, SCREEN_ADDRESS); # use this instead on AZDelivery v3 nano board due to space constraints
  oled.setFont(menuFont);

  Serial.println();
  Serial.println("###############################################################");
  Serial.println("## R4850G2 Unit Test");
  Serial.println("###############################################################");

  oled.clear();
  oled.println("R4850G2 Unit Test");
  oled.println("");

  delay(1000); //Show startup screen

  oled.clear();

  rp.input_voltage = 240.10F; 
  rp.input_frequency = 50.10F;
  rp.input_current = 0.00F;
  rp.input_power = 0.00F;
  rp.input_temp = 20.00F;
  rp.efficiency = 95.00F;
  rp.output_voltage = 44.10F;
  rp.output_current = 0.00F;
  //rp.max_output_current = 0.00F;
  rp.output_power = 0.00F;
  rp.output_temp = 20.00F;
  rp.amp_hour = 0.00F;
  rp.status = startUp;
  
  Serial.println("###############################");
  Serial.println(rp.status); 
  Serial.println("###############################");
  delay(1000);

  //Load defatlt values from EEPROM or set sensible defaults
  getDefaultValues(&rp);

/*
  Serial.println("_________________________");
  Serial.println(millis() / 1000); // Seconds?

  delay(5000);

  Serial.println("_________________________");
  Serial.println(millis() / 1000); // Seconds?
  delay(5000);
*/

  //randomSeed(analogRead(0));
}


void loop() {
  // put your main code here, to run repeatedly:


  teststatus = setupTest(teststatus, &rp);



  if(digitalRead(BUTTON_PIN) == 0)
  {
    doInterface(); 
    
  } 
  else
  {
    rp.status = updateStatus(rp);

    if(updateCurrentValues(&rp))
    {
        Serial.println("### MAKE A CHANGE ###");
        //delay(1000);
    }
    
    
    doDisplay(rp, true);
  }

  //delay(250); 
}







TestStatus setupTest(TestStatus ts, struct RectifierParameters *rp){
  if(ts == NominalChargeCycle)
  {
    //ts = InputVoltage; //SKIP THIS TEST

    if( rp->status == absorption || rp->status == Float )  //Ramp down
    {
      rp->output_current = rp->output_current - 0.2f;
      if(rp->output_current < 0) rp->output_current = 0;

      if(doLoadTest == 0 && rp->output_current >= 19.00f && rp->output_current <=  20.00f)  //Add heavy load (battery voltage drops) during rampdown in absorption
      {
        rp->output_voltage = 50.00f; 
        doLoadTest++;
      }

      if(doLoadTest == 1 && rp->output_current >= 0.00f && rp->output_current <=  1.00f)  //Add heavy load (battery voltage drops) during rampdown in Float
      {
        delay(1000);
        rp->output_voltage = 50.00f; 
        doLoadTest++;
      }
      if(doLoadTest == 2 && rp->output_current <= 0.00f)  //Continue on with range tests
      {
         delay(1000);
         ts = InputVoltage;
      }
    }
    else if(rp->output_current < rp->current) //Ramp Up
    {
      rp->output_current = rp->output_current + 0.2f;
      if(rp->output_current > rp->current) rp->output_current = rp->current;
    }
    else if( rp->current == rp->currentSet && rp->output_voltage < rp->currentSet)
    {
      rp->output_voltage = rp->output_voltage + 0.2f;
      if(rp->output_voltage > rp->voltageSet) rp->output_voltage = rp->voltageSet;
    }
    




  }


//Range tests
 if(ts == InputVoltage)
  {
    if(rp->input_voltage == 240.10F) 
    {
      rp->input_voltage = 70.00F;
      rp->output_current = 0.00F;
      rp->current = 20.00F;
    }

    if(rp->input_voltage < 90) {  rp->input_voltage == 89; }

    if(rp->input_voltage != 240.20F)
    {
     rp->input_voltage = rp->input_voltage + 1;
     rp->input_power = rp->input_voltage * rp->input_current;
    }

     if(rp->input_voltage > 260)
     {
      rp->input_voltage = 240.20F;
      //ts = InputFreq;
     }

     if(rp->input_voltage == 240.20F  && rp->output_current > 20)  //Wait for a ramp up to happen
      ts = InputFreq;

    if(rp->output_current < rp->current) //Ramp Up
    {
      rp->output_current = rp->output_current + 0.2f;
      if(rp->output_current > rp->current) rp->output_current = rp->current;
    }



  }








  if(ts == InputFreq)
  {

    if(rp->input_frequency == 50.10F) rp->input_frequency = 44.00F;

    rp->input_frequency = rp->input_frequency + 1;

    if(rp->input_frequency > 65)
     {
      rp->input_frequency = 50.10F;
      ts = InputCurrent;
     }

  }
  if(ts == InputCurrent)
  {
    rp->input_current = rp->input_current + 0.1F;
    rp->input_power = rp->input_voltage * rp->input_current;

    if(rp->input_current > 12)
     {
      rp->input_current = 10.00F;
      ts = InputTemp;
     }


  }


  if(ts == InputTemp)
  {
    rp->input_temp = rp->input_temp + 1;

    if(rp->input_temp > 100)
     {
      rp->input_temp = 50.00F;
      ts = OutputVoltage;
     }

  }

  if(ts == OutputVoltage)
  {
    if(rp->output_voltage == 44.10F) rp->output_voltage = 0.00F;

    rp->output_voltage = rp->output_voltage + 1;
    rp->output_power = rp->output_voltage * rp->output_current;

    if(rp->output_voltage > 60)
     {
      rp->output_voltage = 44.10F;
      ts = OutputCurrent;
     }

  }
  if(ts == OutputCurrent)
  {
    rp->output_current = rp->output_current + 1;
    rp->output_power = rp->output_voltage * rp->output_current;

    if(rp->output_current > 60)
     {
      rp->output_current = 30.00F;
      ts = OutputTemp;
     }

  }


  if(ts == OutputTemp)
  {
    rp->output_temp = rp->output_temp + 1;

    if(rp->output_temp > 100)
     {
      rp->output_temp = 50.00F;
      ts = NominalChargeCycle;  //STARTOVER????
     }

  }

    return ts;
}








void doInterface() {
    oled.setCursor(0,0);
    Serial.print("Max Voltage: "); oled.print("Max Voltage: ");       
    Serial.print(random(10)); oled.print(random(10));      
    Serial.println("V"); oled.println("V      ");   
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
    oled.println("                     ");
}



