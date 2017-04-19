
/*
 * Functions 
 * 
 * void init_sensors()        // function to initialise pins
 * void manage_brightness()   // adjust the brightness multipliers current_light_scale and target_light_day
 * void manage_fans()         // manage the speed of fans
 * int read_temp(int sensor)  // return value of temp sensor, input 1-3 corresponding to the sensor to report
 * float read_light()         // return avaraged light value and determine if one is covered
 * float read_current()       // return the total current draw of the matrices
 * 
 */

#define TEMP_RESISTOR 10000     //resistance used in series with thermistors

#define ROOM_BRIGHTNESS 800 //??? set this later 

// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 20000//3950
// temp for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 20  

#include <math.h>

//sensors and control 
bool temp_sensor_enable = true;       //define which snensors and control fans are used
bool current_sensor_enable = true;
bool light_sensor_enable = true;
bool fan1_enable = true;
bool fan2_enable = true;
bool fan3_enable = false;

//int temp1 = A0;     // all pins defined in main file
//int temp2 = A1;
//int temp3 = A2;
//
//int current1 = A3;
//int current2 = A4;
//
//int light1 = A5;
//int light2 = A6;
//
//int fan1 = 5;
//int fan2 = 6;
//int fan3 = 7;


void init_sensors(){
  Serial.println(F("Init sensors... "));
  if (temp_sensor_enable){    //enable temperaure sensors
    pinMode(temp1, INPUT);
    pinMode(temp2, INPUT);
    pinMode(temp3, INPUT);
  }

  if(current_sensor_enable){
    pinMode(current1, INPUT);
    pinMode(current2, INPUT);
    }

  if(light_sensor_enable){
    pinMode(light1, INPUT);
    pinMode(light2, INPUT);
    }

  if(fan1_enable)
    pinMode(fan1, OUTPUT);
  
  if(fan2_enable)
    pinMode(fan2, OUTPUT);
  
  if(fan3_enable)
    pinMode(fan3, OUTPUT);
}

void manage_brightness(){       //brightness is dependent on the current draw and the ambient light
  
  float avg_light  =read_light(); //average light in range 0-1023

  float tot_current = read_current();

  if (tot_current >= 30){           // do not allow current to exceed 30A to power screens. allow 5A for everything else with 5A margin
    if (debug)
    Serial.println(F("Total current greater than 30 amps, reduce brightness"));
    while(read_current()>30){
      
      current_light_scale = current_light_scale -0.1;   //continually reduce brightness on each iteration until stabilised
      update_brightness();
      delay(50);
    }            
  }

  else{                                       // otherwise the current is below 30A, use more intelligent method to set light scaling
    avg_light = map(avg_light, 0, 1023, 0.0, 7.0);   // change this depending on if using colour333 or colour444 in matrix code (ie 0-7 or 0-15)
    
    target_light_scale = avg_light/8;                   // also change this depending on if using colour333 or colour444 in matrix code (ie 8 or 16)

    Serial.println(F("Total current below 30 amps"));
    
    if (abs(target_light_scale-current_light_scale) < 0.3){   //only update display brightness if target significantly different from current value
      
      while(target_light_scale != current_light_scale && read_current()<30){   //while the current value isnt the target value and less than 28A being drawn
        
        if (abs(target_light_scale-current_light_scale) < 0.1){    //if difference between current displayed brightness and target is less than 0.1, set equal
          current_light_scale = target_light_scale;
          }
        else{                                             //otherwise decrease difference by 0.1;
          if (target_light_scale > current_light_scale)
            current_light_scale = current_light_scale+0.1;
          else
            current_light_scale = current_light_scale-0.1;
        }
        Serial.print(F("current light scale update sent, value:"));
        Serial.println(current_light_scale);
        
        update_brightness();        //function to send frame detailing brightness to all megas
        delay(100);                 //delay to allow megas to interpret and apply frame
      }
    }
  }
}


void manage_fans(){

  int reading1 = read_temp(1);    //read temperature sensors 1-3, note temp1 is on psu case
  int reading2 = read_temp(2);
  int reading3 = read_temp(3);
  

  if (reading1== 0 || reading2== 0|| reading3== 0|| reading1== 1023|| reading2== 1023|| reading3== 1023){   // temperature sensor not responding, turn all fans on for safety, max airflow
    Serial.println(F("Temperature sensor not responding, all fans on high power"));
    
    digitalWrite(fan1, HIGH);
    digitalWrite(fan2, HIGH);
    //digitalWrite(fan3, HIGH);
    
  }
  else{
    int reading_avg = (reading2+reading3)/2;

  Serial.print(F("Sensors ok: "));

    //***  Worth applying (low pass) filter to these values? Reduce fan ramping in case of short term burst of air temperature/ bad reading

    
    if (reading1 > 40){      //temp 1 is on/in the case of the psu, if it reads 30 degrees, likely components of the psu are significantly hotter
      digitalWrite(fan1, HIGH);
      Serial.println(F("\t High temperature measured at psu, fan on high"));
    }

    else if (reading1> 20&& reading1 < 40){
      
      analogWrite(fan1, map(reading1, 20,40,100,200));
      Serial.println(F("\t psu warm, fan on medium speed"));
    }
    else{
      digitalWrite(fan1, LOW);
      Serial.println(F("\t Psu cool, no through flow required"));
    
    }
  
    if (reading_avg >25){   //if internal temp greater than 25 degrees, turn fans on
      
      if (reading_avg>30) {         //if greater than 35 degrees turn on full power
        digitalWrite(fan2, HIGH);
        //digitalWrite(fan3, HIGH);
        Serial.println(F("\t High internal temperature, full power air through flow"));
      }
      else{
        byte Speed = map(reading_avg, 26,30,100, 200);     // map the reading over the range 25 to 35 degree to a fan speed
                                                           // note that pwm value 100 is about as slow as a fan can turn and usefully move air
        analogWrite(fan2, Speed);
        //analogWrite(fan3, Speed);
        Serial.println(F("\t moderate internal temperature, medium air through flow"));
      }
    }

    else{                                                //otherwise turn fans off, temp below 25 degrees
      analogWrite(fan2, LOW);
      //analogWrite(fan3, LOW);
      Serial.println(F("\t Internal air cool, no through flow required"));
    }
  }
}

int read_temp(int sensor){      //read temperature of specifified sensor and conver to celcius. https://learn.adafruit.com/thermistor/using-a-thermistor , http://playground.arduino.cc/ComponentLib/Thermistor2
  float raw = 0;
  float resistance=0;
  int num_samples =3;
  int alpha=0;
  float temp;

  for (int sample=0;sample<num_samples;sample++){   //loop to get average of several samples, reduces noise
    delayMicroseconds(100);   //short delay
    if (sensor==1){
      alpha = analogRead(temp1);                    //read sensor
      if (alpha == 0 || alpha == 1023)              //if sensor equals rail voltage return as error
      return alpha;
      Serial.print(alpha);
      Serial.print("\t");
      raw = raw + alpha;     //otherwise calculate raw input of thermistor
    }
    if (sensor==2){
      alpha = analogRead(temp2);
      if (alpha ==0 || alpha == 1023)
      return alpha;
      Serial.print(alpha);
      Serial.print("\t");
      raw = raw + alpha;  
    }
    if (sensor==3){
      alpha = analogRead(temp3);
      if (alpha ==0 || alpha == 1023)
      return alpha;
      Serial.print(alpha);
      Serial.print("\t");
      raw = raw + alpha; 
    }
  }
  if(debug){
  Serial.print(F("Raw total: "));
  Serial.print(raw);
  Serial.print(F("\t"));
  }
  raw = raw / num_samples;

  if (debug){
  Serial.print(F("Raw average: "));
  Serial.println(raw);
  Serial.print(F("\t"));
  }
  //resistance = TEMP_RESISTOR*(1024/raw-1);
  resistance = 10000*(1024/raw-1);
  if (debug){
  Serial.print(F("Resistance: "));
  Serial.println(resistance);
  }

//method 1

  temp = log(resistance); 
//         =log(10000.0/(1024.0/RawADC-1)) // for pull-up configuration
 temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp ))* temp );
 temp = temp - 273.15;            // Convert Kelvin to Celcius
 //temp = (temp * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit
 //return temp;

//method 2  <- not working
  
//  temp = resistance / TEMP_RESISTOR ;     // (R/Ro)
//  temp = log(temp);                  // ln(R/Ro)
//  temp /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
//  temp += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
//  temp = 1.0 / temp;                 // Invert
//  temp -= 273.15;  

  if (debug){    
    Serial.print(F("Average thermistor resistance: "));
    Serial.print(resistance);  
    Serial.print(F("\t Caluclated temperature: "));
    Serial.println(temp);
  }  
  return temp; 
}






float read_light(){
  int reading1 = analogRead(light1);
  int reading2 = analogRead(light2);
  bool light1_usable = true;
  bool light2_usable = true;

  if (reading1 ==0 || reading1 ==1023){     //check if reading is reliable, is potiential divider not connected correctly reading not usable
    Serial.println(F("Problem reading light sensor 1"));
    light1_usable = false;
    
  }

   if (reading2 ==0 || reading2 ==1023){
    Serial.println(F("Problem reading light sensor 2"));
    light2_usable = false;
  }

  if (light1_usable == false && light2_usable == true){   //if one sensor is usable just return the value
    if (debug)
    Serial.println(F("return sensor 2 reading"));
    return reading2;
  }

  if (light2_usable == false && light1_usable == true){
    if (debug)
    Serial.println(F("return sensor 1 reading"));
    return reading1;
  }

  if (light1_usable == true && light2_usable == true){    //if obth usable, compare readings
    int avg;

    if (abs(reading1-reading2) >=(1023/2)){    //if large disparity between sensor readings, return the higher, possibly one covered
      if (debug){
        Serial.println(F("large disparity in sensor readings, return higher"));
        Serial.print(F("light sensor 1: "));
        Serial.print(reading1);
        Serial.print(F("\t light sensor 2: "));
        Serial.println(reading2);
      }
      if (reading1>reading2)
        return reading1;
      
      else 
        return reading2;
    }
    else{                                       //if values close assume both valid, return average
      avg = (reading1+reading2)/2;
      if (debug){
        Serial.println(F("both readings good, return average"));
        Serial.print(F("Average reading: "));
        Serial.println(avg);
      }
      return avg;
    }
  }

  if (light1_usable == false && light2_usable == false){    //if neither usable return 0.5, safe ish value
    return 0.5;
  }  
}






float read_current(){
   /*
  Measuring Current Using ACS712
  */
  
  int mVperAmp = 66; // use 185 for 5A Module, 100 for 20A Module and 66 for 30A Module
  int ACSoffset = 2500; 
  
  int RawValue1 = 0;
  int RawValue2 = 0;
  
  double Voltage1 = 0;
  double Amps1 = 0;
  
  double Voltage2 = 0;
  double Amps2 = 0;
  
   RawValue1 = analogRead(current1);
   Voltage1 = (RawValue1 / 1023.0) * 5000; // Gets you mV
   Amps1 = ((Voltage1 - ACSoffset) / mVperAmp);
  
   RawValue2 = analogRead(current2);
   Voltage2 = (RawValue2 / 1023.0) * 5000; // Gets you mV
   Amps2 = ((Voltage2 - ACSoffset) / mVperAmp);
   
   if (debug){
     Serial.println(F("Current sensor 1:"));
     Serial.print(F("Raw Value = " )); // shows pre-scaled value 
     Serial.print(RawValue1); 
     Serial.print(F("\t mV = ")); // shows the voltage measured 
     Serial.print(Voltage1,3); // the '3' after voltage allows you to display 3 digits after decimal point
     Serial.print(F("\t Amps = ")); // shows the voltage measured 
     Serial.println(Amps1,3); // the '3' after voltage allows you to display 3 digits after decimal point
     
     Serial.println("");
    
     Serial.println(F("Current sensor 2: "));
     
     Serial.print(F("Raw Value = " )); // shows pre-scaled value 
     Serial.print(RawValue2); 
     Serial.print(F("\t mV = ")); // shows the voltage measured 
     Serial.print(Voltage2,3); // the '3' after voltage allows you to display 3 digits after decimal point
     Serial.print(F("\t Amps = ")); // shows the voltage measured 
     Serial.println(Amps2,3); // the '3' after voltage allows you to display 3 digits after decimal point
   }
  
   float total_current = Amps1 + Amps2;
  
   return total_current;
   
 }

