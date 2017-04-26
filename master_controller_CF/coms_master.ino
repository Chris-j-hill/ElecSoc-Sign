

/*
   Functions

   void init_i2c()                // initialise the i2c link
   void send_frame_1()            // interrupts caused due to slave requesting data
   void send_frame_2()
   void send_frame_3()
   void send_frame_4()
   void send_frame(int address)   // send frame to the specified address
   void make_frame(int mode1)     // make the frame
   byte calculate_checksum()      // calculate the checksum of the
   void std_text_frame()          // generate standard text frame using current input_str
   void update_brightness_frame() // generate update brightness frame to send new multiplier
   void update_brightness()       // function to update the brightness on all megas

*/



void init_i2c()      //initialise the pins for comms
{
  Serial.println(F("Init i2c..."));
  Wire1.begin();

  for (int ref = 0; ref < NO_ADDRESSES; ref++)
  {
    pinMode(arduino[ref], INPUT);
   
  }
}

void send_frame_1()
{
  //noInterrupts();         // disable interrupts until the request is dealt with
  addressToSend = 0x01;
  data_request1 = true;
}

void send_frame_2()
{
  //noInterrupts();
  addressToSend = 0x02;
  data_request2 = true;
}

void send_frame_3()
{
  //noInterrupts();
  addressToSend = 0x03;
  data_request3 = true;

}

void send_frame_4()
{
  //noInterrupts();
  addressToSend = 0x04;
  data_request4 = true;
}


void frame_requested() {
  if (data_request1 == true) {  //check is interrupt has been triggered since last loop
    send_frame(1);              //send frame
    data_request1 = false;      //set interrupt flag false
  }

  if (data_request2 == true) {
    send_frame(2);
    data_request2 = false;
  }

  if (data_request3 == true) {
    send_frame(3);
    data_request3 = false;
  }

  if (data_request4 == true) {
    send_frame(4);
    data_request4 = false;
  }

}


void send_frame(int address)        //send frame if arduino mega is requesting it
{
  if (debug) {
    Serial.print(F("send to arduino "));
    Serial.println(address);
  }
  make_frame(mode);
  Wire1.beginTransmission(address); // transmit to device with address 1,2,3 and 4
  Wire1.write(frame, frame[FRAME_LENGTH_BYTE+5]);              // send frame
 
  Wire1.endTransmission();
  Wire1.flush();

  pulse_reset2();    //reset all positions


}

void make_frame(int mode1) {           //generate the new frame
  if (debug)
  {
    Serial.println(F("making frame"));
  }


  frame[0] = 91;            //first byte always startbyte

  if (mode1 == 1 || mode1 == 2 || mode1 == 3)       //frame type for displaying tweet in either horizontal or vertical scrolling or static display
  {
    if (debug)
      Serial.println(F("standard text frame"));
    std_text_frame();
  }

  else if (mode1 == 20) {
    if (debug)
      Serial.println(F("update brightness frame"));
    update_brightness_frame();
  }


  //else if...{             //add other frame generating protocols here

  //}




  //Serial.flush();


  if (debug) {                              //debug to display finished frame and test if checksum is correct before sending
    int check = 0;
    Serial.println(F("frame generated: "));    //print again
    for (int k = 0; k < frame[FRAME_LENGTH_BYTE]; k++) {
      Serial.print((char)frame[k]);
      if (k >= HEADER_LENGTH && k < frame[FRAME_LENGTH_BYTE] - 1)            //check checksum
        check = check + frame[k];
    }

    if (force_checksum_error) {             //Force checksum error by enabling this
      check = check + 10;
      frame[HEADER_LENGTH] = frame[HEADER_LENGTH] + 10;                //change arbitrary byte, in this case first data element
      Serial.println(F("Forced checksum error"));
    }

    check = 256 - (check % 256);
    //frame[k] = frame[k]+1;
    Serial.println("");
    if (check == 256)
      Serial.println(F("checksum correct"));   //if check counter =0 checksum correctly generated

    else
      Serial.print(F("checksum incorrect, value is: "));
    Serial.println(check);
  }
  for (int i = frame[FRAME_LENGTH_BYTE+1]; i<frame[FRAME_LENGTH_BYTE+6];i++){
    frame[i] = '#';
  }
  frame[2] = frame[2]+5;

  //Serial.flush();
}




byte calculate_checksum() {                     //function to calculate the checksum of the data string
  int temp = 0;
  if (debug)
  Serial.println("calculate checksum function..."):
  
  for (int k = HEADER_LENGTH; k < frame[FRAME_LENGTH_BYTE] - 2; k++) {
    temp = temp + frame[k];
    if (debug)
      Serial.print((char)frame[k]);
  }
  checksum = (byte) (256 - (temp % 256));
  if (debug) {                                  //print checksum if debug
    Serial.print(F("calculate checksum as:"));
    Serial.println(checksum);
  }
  return checksum;

}


void std_text_frame() {

   if (debug)
  Serial.println("std_text_frame function..."):
  
  frame[FRAME_LENGTH_BYTE] = input_str.length() + HEADER_LENGTH + TRAILER_LENGTH; //frame length  (+5 for extra 5 bytes we add just here)
  if (debug) {
    Serial.print(F("frame length: "));
    Serial.println(frame[FRAME_LENGTH_BYTE]);
  }

  frame[DATA_IDENTIFIER_BYTE] = mode + 48;                         //character conversion for mode value, for readability...
  int k = 0;
  for (k = HEADER_LENGTH; k < frame[FRAME_LENGTH_BYTE] - 2; k++) {          //copy text string
    frame[k] = (byte) input_str[k - HEADER_LENGTH];                 //input_str is mode:data
  }

  frame[k] = calculate_checksum();          //checksum value
  frame[k + 1] = 93;                          //end byte

  if (debug) {
    for (k = 0; k < frame[FRAME_LENGTH_BYTE]; k++)              //reprint entire frame
    {
      Serial.print((char)frame[k]);
    }
    Serial.println();
  }
}

void update_brightness_frame() {  //short frame, just generate everything directly, no need for loops or other functions

  if (debug)
    Serial.println(current_light_scale);

  int scale = round(current_light_scale * 100);

  frame[FRAME_LENGTH_BYTE] = 6;   //1 data element and 5 header and trailer elements
  frame[DATA_IDENTIFIER_BYTE] = 20 + 48;

  frame[HEADER_LENGTH] = scale;

  if (scale > 256)  //hard limit on light scaling is 2.56 times, shouldnt need to be this bright, but also cant transimit larger value as this format
    frame[HEADER_LENGTH] = 256;

  frame[HEADER_LENGTH + 1] = 256 - frame[HEADER_LENGTH];
  frame[HEADER_LENGTH + 2] = 93;

}

void update_brightness() {
  int alpha = 0;
  make_frame(20);                  //make a frame of mode 20, update brightness frame identifier

  for (alpha = 0; alpha < frame[FRAME_LENGTH_BYTE]; alpha++) {
    Serial.print((byte) frame[alpha]);
    Serial.print(" ");
  }
  Serial.println("");

  for (int alpha = 1; alpha <= NO_ADDRESSES; alpha++) { //send on to all arduinos
    Serial.print(F("send to arduino "));
    Serial.println(alpha);

    Wire1.beginTransmission(alpha); // transmit to device with address 1,2,3 and 4
    Wire1.write(frame, frame[FRAME_LENGTH_BYTE]);              // send frame
    Wire1.endTransmission();
    Wire1.flush();
    delay(1);     //pause, not sure if needed
  }
  Serial.println(F("Update sent to all arduinos"));
}

