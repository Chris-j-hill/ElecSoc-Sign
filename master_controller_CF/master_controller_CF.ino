

  void trig();                    // trig signal to indicate increment of matrix display
  void check_serial();            // check for serial input,  use to change input_str directly
  void mode_change();             // force change the mode of the device through serial

  
  void pulse_reset();             // pulse the reset line, syncronise megas
  void pulse_reset2();            // fixed version of pulse reset, doesnt require new_flag2 high to run, good for asyncronise use
  void mode_change_interrupt();   // if button pressed, call interrupt to change mode
  void increment_counter();       // keep track of where text should be on screen
 
  void init_i2c();                // initialise the i2c link
  void send_frame_1();            // interrupts caused due to slave requesting data
  void send_frame_2();
  void send_frame_3();
  void send_frame_4();
  void send_frame(int address);   // send frame to the specified address
  void make_frame(int mode1);     // make the frame
  byte calculate_checksum();      // calculate the checksum of the 
  void std_text_frame();          // generate standard text frame using current input_str
  void update_brightness_frame(); // generate update brightness frame to send new multiplier
  void update_brightness();       // function to update the brightness on all megas
 
  void init_sd_cards();     // initialisation process for sd cards
  void remove_card_1();     // test and wait until sd1 removed
  void extract_data();      // get copied data from sd2, network & password or default string
  int in_header();         // return 1 if the colon is in the header, otherwise 0
  int is_network();         // check if network stored in buffer
  int is_password();        // check if password stored in buffer
  int is_default();        // check if default string stored in buffer
  int string_length();      // calculate the lenght of the relevent string in file

  void init_sensors();        // function to initialise pins
  void manage_brightness();   // adjust the brightness multipliers current_light_scale and target_light_day
  void manage_fans();         // manage the speed of fans
  int read_temp(int sensor);  // return value of temp sensor, input 1-3 corresponding to the sensor to report
  float read_light();         // return avaraged light value and determine if one is covered
  float read_current();       // return the total current draw of the matrices
  
 


#include <Wire.h>

#include <SdFat.h>
#include <SdFatUtil.h>
#if !USE_MULTIPLE_CARDS
#error You must set USE_MULTIPLE_CARDS nonzero in SdFatConfig.h
#endif



#define NO_ADDRESSES 4

#define HEADER_LENGTH 3      // start byte, frame length, data identifier/mode
#define TRAILER_LENGTH 2      //length of trailer, checksum and endbyte
#define DATA_IDENTIFIER_BYTE 2    // address in frame of data identifier 
#define FRAME_LENGTH_BYTE 1       // address of frame length byte, length of entire frame

#define TOT_WIDTH  64*NO_ADDRESSES   // the number of leds across the total screen width
#define TOT_HEIGHT 32               // height of the screen



//Pins used on due
int trig_pin = 13;    // update pin for screens
int Reset = 15;        //used to sync screens
int button = 22;
int arduino[NO_ADDRESSES] = {16,17,18,19};        //interrupt pins on mega (can use any digital pins on Due)

const uint8_t SD2_CS = 4;   // chip select for sd2
const uint8_t SD1_CS = 40;  // chip select for sd1

int temp1 = A0;
int temp2 = A1;
int temp3 = A2;

int current1 = A3;
int current2 = A4;

int light1 = A5;
int light2 = A6;

int fan1 = 5;
int fan2 = 6;
int fan3 = 7;

int notifier = 13;    //pin to use to flash led to prompt user, useful if not connected to serial

//+ i2c link -> sda(20) & scl(21)
//+ spi 





String input_str = "1:Testing 1 2 3 4 5 6 7 8 9";
int text_size = 2;
//int matrix_index = 64 * (NO_ADDRESSES);    // define width of screen in pixels, ie start position of scroll
int textMin = input_str.length() * -(text_size*6) - TOT_WIDTH;   //allow to scroll off left side by this much
       


int trig_freq = 8;   //update rate of screens, number of times characters are moved, not refersh rate
int time_now;
int trig_time_prev;
bool debug = true;       // mode to enable debug
bool force_checksum_error = false; //force the checksum to be wrong if true
int mode = 1;

int new_flag = 0;     //flag to indicate new string from serial input
int new_flag2 = 0;    //flag to indicate new string from mode change
volatile int addressToSend = 0x00;

bool data_request1 = false;   //flags in interrupts
bool data_request2 = false;
bool data_request3 = false;
bool data_request4 = false;
bool button_pushed = false; 

int sensor_read_delay = 4000;
int prev_sensor_time =0;

//comms

//int arduino_enable[NO_ADDRESSES] = {A4,A5,A6,A7};
byte frame[150];
byte checksum;

int counter = NO_ADDRESSES*64;

float target_light_scale = 1.0;         //target scaling multiplier for colour applied to screen
float current_light_scale = 1.0;  //actual scaling multiplier currently applied

String temp_input_str;


// sd cards
                            
String str_sd ="123456789 123456789 123456789 123456789 123456789 ";   //sample string, needed to set the length correctly
String Network = "init network";    //store current network
String Password = "init password";   //store current password


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
  Serial.begin(115200);
  while (!Serial) {}  // wait for Leonardo
  Serial.flush();
  Serial.println(F("Serial good"));
  pinMode(trig_pin, OUTPUT);
  pinMode(Reset, OUTPUT);
  //pinMode(Reset, LOW);
  pinMode(notifier, OUTPUT);
//  while(1){
//    pulseReset2();
//    delay(100); 
//  }
  init_i2c();
  Serial.println(F("i2c init ok"));
  init_sensors();
  Serial.println(F("sensor init ok"));
  delay(100);
  //noInterrupts();
  #if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)          //define interrupts differently for mega and due, this code can run on either
  
  //attachInterrupt(digitalPinToInterrupt(arduino[0]), send_frame_1, RISING);
  //attachInterrupt(digitalPinToInterrupt(arduino[1]), send_frame_2, RISING);
  attachInterrupt(digitalPinToInterrupt(arduino[2]), send_frame_3, RISING);
  attachInterrupt(digitalPinToInterrupt(arduino[3]), send_frame_4, RISING);
  attachInterrupt(digitalPinToInterrupt(button), button_interrupt, RISING);
  
  #else
  
    attachInterrupt(arduino[0], send_frame_1, RISING);
    attachInterrupt(arduino[1], send_frame_2, RISING);
    attachInterrupt(arduino[2], send_frame_3, RISING);
    attachInterrupt(arduino[3], send_frame_4, RISING);
    attachInterrupt(button, button_interrupt, RISING);
    
  #endif

  
  interrupts();
  delay(100);
  Serial.println(F("attach interrupts ok"));
  delay(1000);


  //initialise sd cards and extract data
  str_sd.reserve(150);
  init_sd_cards();
  remove_card_1();

  Serial.println(F("Card 1 removed, extract data..."));
  
  extract_data();
  input_str = str_sd;
  Serial.print(F("Network: "));    //print extracted data
  Serial.println (Network);
  Serial.print(F("Password: "));
  Serial.println (Password);
  Serial.print(F("Str: "));
  Serial.println (str_sd);
  Serial.print(F("input String: "));
  Serial.println(input_str);


  trig_time_prev = millis(); //used for trig function
  prev_sensor_time = millis();
  
      
  pulse_reset2();          //pulse the reset line, due ready
//
//       while(1){
//    pulse_reset2();
//    delay(100); 
//  }
}

// the loop function runs over and over again forever
void loop() {

  check_serial();                       // check for new data
  mode_change();                        // if data recieved, change mode
  pulse_reset();                        // if mode change requires reset, do so
  trig();                               // state change trig pin
//  if (button_pushed)
//  mode_change_interrupt();              // check if button pushed
  frame_requested();                    // check if frame has been requested
  
//  if (millis() - prev_sensor_time > sensor_read_delay){     // do this every couple loops, no need to do all the time
//    
//    manage_fans();                      // manage fans based on internal temperature
//    manage_brightness();                // manage brightness based on current draw and ambient light
//    prev_sensor_time = millis();
//    Serial.println("-");
//    Serial.println("");
//  }
  
//  if (addressToSend != 0)               // if any slaves request a frame
//  {
//    send_frame(addressToSend);          // if arduino needs new frame, send it on         (handled by interrupts)
//    addressToSend = 0;
//    interrupts();
//  }
  //delay(500);
//  Serial.println("-");
//  Serial.println("");
}




void trig() { //flip the state of the pin

  time_now = millis();
  //                                        \ / why 2 ?
  if (time_now >= (trig_time_prev + ((1000 / 2) / trig_freq))) { //flip state at defined intervals
    trig_time_prev = time_now;   //mark time
    digitalWrite(trig_pin, !digitalRead(trig_pin));

    if (debug)
      Serial.println("state change");

    increment_counter();
  }

}

void check_serial()            //check for serial data input, use to change input_str directly
{
  if (Serial.available() > 0)
  {
    
    
    while (Serial.available() > 0)
    {
     temp_input_str = Serial.readString();
    }
    
    if (debug)
    {
      Serial.println(temp_input_str);
    }

    if (temp_input_str[0] == 'M' && temp_input_str[1] == 'o' && temp_input_str[2] == 'd' && temp_input_str[3] == 'e') 
    {
      mode_change();
    }
    else if (temp_input_str[0] == 'L' && temp_input_str[1] == 'i' && temp_input_str[2] == 'g' && temp_input_str[3] == 'h' && temp_input_str[4] == 't')
    {
      change_brightness();
    }
    else if(temp_input_str == "Reset"){
      
      pulse_reset2();
      
    }
    else
    {
      input_str = temp_input_str;
      new_flag = 1;     //indicate new string
      new_flag2 =1;
      
    }
    temp_input_str = "invalid";
  }
}


void mode_change() {  //change mode when new string recieved in format "Mode x"
  if (temp_input_str[0] == 'M' && temp_input_str[1] == 'o' && temp_input_str[2] == 'd' && temp_input_str[3] == 'e') 
  {
    mode = temp_input_str[5] - 48; //set mode to value of input "Mode x"

    Serial.print(F("Mode set to:  "));
    Serial.println(mode);
    new_flag = 0;
    if (mode == 40)
    {
      debug = !debug;
    }
    if (mode<10){ //mode<20)    //if it is a display text mode
      bool alpha = false;       //dont want to increment mode in mode_change_interrupt, so store button pushed temporarily 
      
      if (button_pushed)        //if button pushed is true, save it in alpha and set false
      alpha = button_pushed;
      
      button_pushed = false;
      mode_change_interrupt();    
      button_pushed = alpha;    //in case that button was also pushed at same time (unlikely), mode change will be called again and mode incremented
    }
  }
}

void change_brightness(){   //manually adjust the light scaling, can only work is sensors not initialised / read, otherwise will be set back on next loop
  Serial.println(temp_input_str);
  current_light_scale = (float) temp_input_str[6]/100;    // adjust target and current light scale factor
  target_light_scale = (float) temp_input_str[6]/100;

  Serial.print(F("Current light scaling adjusted to: "));
  Serial.println(current_light_scale);
  update_brightness();
}

void pulse_reset()        //reset pulse to megas to sync displays
{
  if (new_flag2 == 1)
  {
    
    digitalWrite(Reset, HIGH);
    delay(100);
    digitalWrite(Reset, LOW);
    new_flag = 0;
    if (debug)
    {
      Serial.println(F("Reset pulse sent"));
    }
  }
  

}

void pulse_reset2(){
   
    digitalWrite(Reset, HIGH);
    delay(100);
    digitalWrite(Reset, LOW);
    new_flag = 0;
    if (debug)
    {
      Serial.println(F("Reset pulse sent"));
    }
  
}


void mode_change_interrupt(){   //mode change interrupt mode to be incremented and frames to be passed to all arduino slaves
  if (debug)
  Serial.println(F("mode change interrupt called"));
  //noInterrupts();      //disable interrupts while sending

  //only increment mode if button pushed
  if (mode<10 && button_pushed){//if (mode<20){        // cycle through valid text display modes
    mode++;            //increment with button push
  }
  else if (button_pushed){  //if button pushed loop mode, otherwise leave unchanged, updated through serial elsewhere
    mode=1;
  }
  
  
  //always send new frames out specifying new mode
  for (int alpha =1; alpha<= NO_ADDRESSES; alpha++){   //send the current string to all arduinos along with updated mode
    send_frame(alpha);    //send standard text frame
  }

  //updated all frames, pulse reset
  
  pulse_reset2();

  button_pushed=false;    //set false after action taken, until next interrupt 
  counter = TOT_WIDTH;
  //interrupts();         //enable interrupts again
}



void increment_counter(){

  if (mode == 1 || mode == 4 || mode == 7){    //scrolling both sides, left or right
    Serial.print("Mode: ");
    Serial.println(mode);
    counter--;
    if (counter<= textMin){
      counter=TOT_WIDTH;
       
      pulse_reset2();          //pulse the reset line

    }
  }
  else {
    Serial.print("Mode: ");
    Serial.println(mode);
    counter++;
    if (counter>= -textMin){
      counter=TOT_WIDTH;
      
      pulse_reset2();
      
    }
  }
  Serial.println(counter);
}

void button_interrupt(){    //call mode_change_interrupt, but disable the comments because serial print interrupt problems
 button_pushed = true;
}





