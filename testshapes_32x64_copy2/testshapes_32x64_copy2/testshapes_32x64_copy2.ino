// testshapes demo for RGBmatrixPanel library.
// Demonstrates the drawing abilities of the RGBmatrixPanel library.
// For 32x64 RGB LED matrix.

// NOTE THIS CAN ONLY BE USED ON A MEGA! NOT ENOUGH RAM ON UNO!


#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library, modified for this project
#include <Wire.h>

//define functions here
void home_balls();
void bouncing_balls();
void scroll_text_horizontal();
void scroll_text_vertical();
void update_lines_array();

void check_serial();
void read_text();
void receiveEvent(int bytes);

/*  Define the screen number based on the layout below
    ________________ ________________ ________________ ________________
   |                |                |                |                |
   |       0        |       1        |       2        |       3        |
   |                |                |                |                |
   |________________|________________|________________|________________|

*/
//        \/ address now set in hardware
//#define ADDRESS 0                   // change for each arduino 1/2/3/4/RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);

#define SCREEN_NUM 1        // the position of this matrix in the screen, based on above, easier to index if zero
#define NUM_SCREENS  2              // the total number of matrices in setup
#define TOT_WIDTH  64*NUM_SCREENS   // the number of leds across the total screen width
#define TOT_HEIGHT 32               // height of the screen

#define HEADER_LENGTH 3             // start byte, frame length, data identifier/mode
#define TRAILER_LENGTH 2
#define DATA_IDENTIFIER_BYTE 2      // address in frame of data identifier 
#define FRAME_LENGTH_BYTE 1         // address of frame length byte, length of entire frame



// matrix pins
#define OE1  8
#define OE2  9
#define LAT 10
#define CLK 11
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE1, OE2, true, 64);

//syncing matrices
int ADDRESS = 0;                  // hardware configured address
int trig = 13;                    // pin to update display, essentially a clock from arduino due
int index1 = 64 * SCREEN_NUM;      // define offset of the left most pixel on this matrix from left most pixel of the overall screen
int flag = 0;                     // used to perform edge detection on trig signal
int Reset = 5;                    // reset all objects to initial positions, ground signal when connected, floating otherwise
int counter = 0;                  // helps for scrolling, counts the number fo pulses form the due
bool state = true;                // when state of trig flips, run main loop
//int mode =1;                    //define mode as scrolling, can be changed
bool debug = true;
int mode_change_time =0;


//text general variables
int dist_from_top =8;   // change by changing mode
int text_size = 2;       // also change through mode change
int text_height = 9;  // sizes uses for manual wrapping text
int text_width = 6;   // height = n*8 and wight = n*6 where n = text_size variable used in setTextSize method
uint16_t text_colour = 0x0000;    // colour zero, means use changing hue function, otherwise display data colour
const int max_lines = 140/(TOT_WIDTH/(6*8));       //(max tweet size)/(screen width / max char size)
int num_lines = max_lines;

//define matrix for scrolling horizontal
String   str = "No input string found - No input string found";       //string for horizontal scrolling, defualt message to allert no recieved string
int    textX   = matrix.width(),              //start pos, just off screen, need to include waiting conditions later
       textMin = str.length() * (-text_width*text_size) - index1,   //allow to scroll off left side by this much
       hue     = 0;


//define matrix for scrolling vertical
char   vert_str[] = "No input text found";
int    textY   = matrix.height(),              //start pos, just off screen, need to include waiting conditions later
       textMinY = -(text_size*text_height)*max_lines,                        //allow to scroll off top by this much, Change in function based on correct sizes
       hueY     = 0,
       new_string =0;
//       
//typedef struct {                                    // structure of individual lines for vertical scrolling, 
//                                                    // create array of these for overall string
//  int valid = 0;                                    // indicator for whether this line is active
//  char line[(TOT_WIDTH/6)+1] = "line not filled";   // create char array of max size plus one which will be used for endline char  
//} manual_wrap;
//
//static manual_wrap lines[max_lines];    //create array of lines
//




//Ball stuff

//initialise ball positions
int8_t ball[3][4] = {
  {  3,  0,  1,  1 }, // Initial X,Y pos & velocity for 3 bouncy balls
  { 17, 15,  1, -1 }, //start x,y off screen based on index
  { 27,  4, 1,  1 }
};
//set ball colours
static const uint16_t PROGMEM ballcolor[3] = {
  0x0080, // Green=1
  0x0002, // Blue=1
  0x1000  // Red=1
};


//communications
// i2c pins on mega: sda = 20   scl = 21
int Request_text_pin = 7;
//int Enable_com = 23; 

int flag1 = 0;      // do error checking once frame arrived, flag set high when arrives
byte flag2 = 0;
int i = 0;          // counter for frame size

//byte frame_length;  // frame length as extracted form incoming signal
byte frame_type;    //frame type defines what is to be done with info, ie where string should go or other operation
byte frame_length = 150;    //length of current frame
int max_frame_size = 150;    //max frame size 150, ie max length of a tweet plus some frame info
byte x[150];

int errors = 0;             //number of errors found
bool bad_frame = false; 
bool new_frame = false;     //new frame arrived marker for unpack_frame function
bool reset_flag = false;
int mode=1;                  //current mode of operation
bool update_frame = false;  //request variable to allow code to request updates
//int mode_change_time =0;
int timeout = 0;
int timeprev = 0;
int prev_loop=0;

float current_light_scale = 1.0;

void setup() {
  Serial.begin(115200);

  set_address();    //function to set the address of the mega based on hardware config
  Serial.print("Address: ");
  Serial.println(ADDRESS);
  
  //matrix initialisation
  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge
  matrix.setTextSize(text_size);
  matrix.Mode(mode2matrixMode()); //set operating mode of matrix
  //home_balls();               //set ball positions

  // timing clock pins
  pinMode(trig, INPUT);   //clock signal to increment step
  pinMode(Reset, INPUT);  //start scrolling at same time


  init_i2c();
  // communications
  
  Serial.flush();
  prev_loop= millis();
}

void loop() {
  //delay(1000);

  if (state != digitalRead(trig)){ // || millis()>prev_loop+50) {  //edge detection of trig pin, trig on every edge
    state = digitalRead(trig);
    if (debug){
//      Serial.print("Mode: ");
//      Serial.println(mode);
    }
   
    //prev_loop= millis();
    //if (NUM_SCREENS == SCREEN_NUM-1 && millis() < mode_change_time+2000){   //if this is the right most screen and there was a mode change less than 2 seconds ago
      //mode_change_display();            //display mode change details
    //}
    //else{                               //otherwise do normal procedure
      // Clear background
      matrix.fillScreen(0);
  
      flag = 1;
      //bouncing_balls();   //update ball positions
      if (mode == 1)
      scroll_text_horizontal();      //update text positions
  
     // else if (mode==2)
     // scroll_text_vertical();
  
      
      flag = 0;
      // Update display
      matrix.swapBuffers(false);    //swap buffers, write to one display the other
      //Serial.println(str);
    //}
  }

  //no need to do comms on trig, might aswell use the dead time
  get_frame();
  unpack_frame();

}

int mode2matrixMode(){   // fucntion to take the operatin mode and return the mode for the matrices, ie two screens on, one on one off, or both off
  Serial.println(F("Change screen mode"));
  if (mode == 1 || mode ==21)
      return 2;           //both on, is mode 2 in matrix.mode() mehtod
  else if (mode == 2 || mode ==22)
      return 1;
  else if (mode==3 || mode==23)
      return 3;
}


void set_address(){
  pinMode(44, INPUT);
  pinMode(45, INPUT);
  
  int temp1 = digitalRead(44);
  int temp2 = digitalRead(45);


  if (temp1 == LOW && temp2 == LOW)
  ADDRESS = 1;

  
  if (temp1 == LOW && temp2 == HIGH)
  ADDRESS = 2;

  
  if (temp1 == HIGH && temp2 == LOW)
  ADDRESS = 3;

  
  if (temp1 == HIGH && temp2 == HIGH)
  ADDRESS = 4;
}


