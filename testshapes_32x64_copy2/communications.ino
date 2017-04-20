

/*
   Communication protocol for retrieving data from arduino due (or other master control board)

   Note that since a tweet is at most 140 characters long, the primary communication data,
   the max frame size will be slightly more than this 150 bytes. For annimations or bitmap pictures
   the data will need to be transmitted in multiple blocks. It can be specified in the header data
   what type of data it is and if there are multiple blcoks to be recieved

   Protocol based on a frame structure as such:   start byte, frame size byte, data identifier, data, ..., checksum byte, endbyte

   The start and end bytes are the characters [ and ] respectively (ascii codes 91 and 93)
   The frame size is the length of characters in the sring being transmitted
   The data identifier is the specific purpose the data is intended for, an attempt to future proof the design (text, annimations, debug instruction etc)
   The data then is the string, or other data to be transmitted
   The checksum is the difference of 256 and sum of the values of the data modulo 256, to find errors the summed data  and checksum at the reciever should equal zero
*/

void init_i2c() {

  Wire.begin(ADDRESS);
  Wire.onReceive(receiveEvent);
  pinMode(Request_text_pin, OUTPUT);

  str.reserve(max_frame_size - HEADER_LENGTH - TRAILER_LENGTH); // reserve space for string,works without but could override variables when extending string
  str = "No input string found - No input string found";  // setup string, need to recieve data from master first
  textMin = str.length() * (-text_width * text_size) - index1;
  delay(500);     // delay to allow all other arduinos to boot up, make sure bufferes are ready to recieve before starting comunications

  update_frame = true;        //request 1 frame on startup
}

void get_frame() {

  //request data
  if (Serial.available() > 0) {   //check for serial input from user
    
    if (Serial.peek() == 'a')
    {
      Serial.read();
      Serial.println(Wire.read());    //check for values stored in i2c buffer, useful for analysing interrupt

      Serial.println(str);            //print current string
      Serial.flush();
    }
    
    else if (Serial.peek() == 'r')
    {
      Serial.read();
      str = "No input string found, No input string found";   //setup string, need to recieve data from master first
    }
    
    else {
      flag2 = 1;        //set falg as 1 so as to request frame
      Serial.flush();
      while (Serial.available() > 0)    // <- loop possibly not needed?
      {
        char c = Serial.read();      // dump any extra characters eg '\n' so loop isn't run excess number of times
      }
    }
  }
  
  else if (update_frame) {    // if code elsewhere requires new frame, eg if animation has finished scrolling
    Serial.println(F("code triggered request"));
    flag2 = 1;
  }



  //make request
  if ( flag2 != 0 || errors != 0 || bad_frame == true) {               //if there has been serial input from user, or non zero errors from last frame, get new frame

    //Wire.flush();                   //flush anything in buffer, shouldnt be anything but to be safe
    digitalWrite(Request_text_pin, HIGH);          //request data from master
    Serial.println(micros());
    timeprev = millis();
    timeout = 0;

    while (flag1 == 0 && timeout < 5000) { //wait until interrupt changes flag1 to 1 or timeout
      delayMicroseconds(100);
      timeout = millis() - timeprev;
    }


    digitalWrite(Request_text_pin, LOW);  // Finished requesting data
    delay(10);
    if (flag1 == 0) {                       // if flag1 is 0 here, timeout occured,stop requesting data then do nothing

      new_frame = false;                    // code requests new frame until new_frame true, ie no errors
      Serial.println(F("Break"));
    }
    else                                    //otherwise data has arrived
    {
      error_check();
    }
  }
}




void error_check() {        // function to error check frame, common to requested frame and non requested frame.
  // on requested frame with errors should trigger get_frame() function

  int frame_length1 = x[FRAME_LENGTH_BYTE];             //the frame length, text plus header plus trailer

  if (debug) {                                      //display some details
    Serial.print(F("Frame length value (x[1]):"));
    Serial.println(frame_length1);

    Serial.print(F("Print frame: "));      //print entire frame for user
    for (int k = 0; k < frame_length1; k++)
    {
      Serial.print((char)x[k]);
    }
    Serial.println(F(""));

  }
  flag1 = 0;                            //reset flag1 for next itteration

  Serial.println (F("Check for Errors"));  //check for errors

  for (int error_counter = HEADER_LENGTH; error_counter < frame_length1 - 1; error_counter++)   // (<frame_length1 -1) as the end byte was not included in the checksum and zero addressing
  {
    errors = errors + x[error_counter];
    //Serial.print(x[error_counter]);
  }
  errors = errors % 256;              //should equal zero if arrived correctly

  if (debug) {
    Serial.print(F("errors present: "));
    Serial.println(errors);
    //      for (int j=0; j<=frame_length1; j++){
    //        Serial.print((char) x[j]);
    //        }
    Serial.println(F(""));
  }


  if (errors == 0) {      //if errors != 0 then get_frame triggered on next iteration (potiential probelms syncing...)
    if (debug)
      Serial.println(F("frame recieved correctly"));
    new_frame = true;                             //indicate new frame ready to unpack

  }
  bad_frame = false;
  flag2 = 0;                             //reset for next itteration
  flag1 = 0;
  Wire.flush();

}




void receiveEvent(int bytes)                   //interrupt to read in new string
{
  int timeOut2 = millis();
  frame_length = 150;
  if (debug) {
    Serial.println(F("data recieved, interupt triggered"));
    Serial.println(F("data intended for this arduino"));
  }
  i = 0;
  while (flag1 == 0)
  {
    if (Wire.available() > 0)
    {

      x[i] = Wire.read();                         // read one character from the I2C
      if (debug)
        Serial.print((char) x[i]);

      //extract info from frame
      if (i == FRAME_LENGTH_BYTE)                                //frame size byte, entire frame including heading and trailer bytes
      {
        frame_length = x[i];
        //Serial.println((byte)frame_length);
      }

      if (i == DATA_IDENTIFIER_BYTE)                                //frame type (mode) byte, used to decied what to do with recieved data
      {
        frame_type = x[i];
      }

      // exit conditions when full frame recieved

      if (i == frame_length - 1)                    //end of frame reached, set flag high, allow wait loop to exit
      {
        flag1 = 1;                                // use to exit waitng loop
        i = 0;                                    // reset i
        frame_length = max_frame_size;            // set frame length to default length for next iteration
        if (debug) {
          //          Serial.println(F(""));
          //          Serial.println(F("Frame recieved"));
        }
        Wire.flush();
      }
      else                        //otherwise increment i
      {
        i++;
      }
    }

    else flag1 = 1;   //break while loop

    //    else
    //    {
    //      Serial.print("Did not Receive full frame (available = ");
    //      Serial.print(Wire.available());
    //      Serial.println(")");
    //      bad_frame = true;             // set as bad frame
    //      flag1=1;                      // break from while loop
    //
    //      if (millis() - timeOut2 > 1000)
    //      {
    //        Serial.println("time out");
    //        break;
    //      }
    //    }

    if (flag1 == 1 && digitalRead(Request_text_pin) != HIGH && !bad_frame) { // full frame recieved but not requested, call function

      flag1 = 0;    //set back to zero to avoid confusion

      error_check();                                            // should do error checking and if errors present, get_frame() will be called
      break;                                                          // if no errors frame will be unpacked as usual
    }
  }

  //Serial.println("exit interrupt");
  Wire.flush();
}


void unpack_frame() {   //if new frame arrived sucessfully extract relevent information form it

  if (new_frame) {    //only unpack frame if new frame is recieved with no errors
    Serial.println(F("new frame arrived, unpack frame"));
    int temp1 = (x[DATA_IDENTIFIER_BYTE]) - 48; //define frame type, could be mode of operation or nature of frame data

    Serial.print(F("frame type: "));
    Serial.println(temp1);

    if (temp1 < 20) {       // change mode to new mode (0-19 define valid modes of operation, ie scrolling, static text etc)
      if (mode != temp1) {            // if current mode does not equal temp1
        mode_change_time = millis();  // log time of mode change, used to display current mode on right most screen
      }
      mode = temp1;   //change mode
      matrix.Mode(mode2matrixMode());   //update screen operating mode

      int e = 0; // x[FRAME_LENGTH_BYTE-2];

      Serial.print(F("New string: "));
      for (e = HEADER_LENGTH; e < x[FRAME_LENGTH_BYTE] - TRAILER_LENGTH; e++) {
        str[e - HEADER_LENGTH] = x[e];                //copy frame into string, offset by frame header length

        Serial.print((char)x[e]);
      }
      Serial.println(F(""));
      //      Serial.print("E = ");
      //      Serial.println(e-HEADER_LENGTH);
      //str[e-HEADER_LENGTH] = NULL;
      str.remove(x[FRAME_LENGTH_BYTE] - HEADER_LENGTH - TRAILER_LENGTH);    //remove any data left after the current string length
      Serial.println(x[FRAME_LENGTH_BYTE] - HEADER_LENGTH - TRAILER_LENGTH);
      textMin = (x[FRAME_LENGTH_BYTE] - HEADER_LENGTH - TRAILER_LENGTH) * (-text_width * text_size) - index1;
      Serial.println(textMin);
      Serial.println(str);
    }
    else if (temp1 == 20) {      // 20 is brightness scaling value*100 (to make into int for transmission)
      current_light_scale = (float)x[HEADER_LENGTH] / 100; //first data element, offset by header length
      Serial.println(F("brightness frame arrived"));
    }

    else if (temp1 > 20 && temp1 < 40) {  // this mode indicates to display mode data for a couple seconds befroe normal operation

    }



  }
  update_frame = false;   //new frame arrived, stop requesting update for now
  new_frame = false;      // do not attempt to unpack more than once
  textMin = str.length() * (-text_width * text_size) - index1;
}














// _____________________________________________________
// ______________________  old code ____________________
// _____________________________________________________


/*
  void read_text() {

  int flag2 =0;   //keep requesting newest frame until one arrives with no errors
  int frame_counter=0;
  bool arrived = false;
  bool mem_allocated = false;
  byte *temp_array;
  int temp_array_length=0;


  do{
    digitalWrite(Request_text_pin, HIGH);   //request data
    flag1=0;    //indicator when frame arrived, changes in interupt

    if (debug)
    Serial.println("requested data, waiting for response");

    while (flag1 == 0) {
      //wait until new frame read in
    }

    if (debug)
    Serial.println("frame recieved");

    //if frame indicates multiple frames to come, create storage array and loop, do error checking later

    if (frame_type > 100 && frame_type < 256 && frame_counter != frame_type-100){ // 100 indicated multiple frames, value from frame_type -100 indicates the number of related frames to arrive
      if (debug){
          Serial.println("indicates multiple frames");
          Serial.print("frames remaining: ");
          Serial.println(frame_type-100);
      }

      if( frame_counter ==0 ){    //first frame, allocate memory
        int Size1 = (frame_type-100)*(max_frame_size); //allocate memory equal to the size of total data to be reciever
        temp_array =  (byte *) malloc(Size1);
        mem_allocated = true;
      }

      //copy data into temp_array;
      for (int w=0; w<frame_length-4;w++){
      temp_array[frame_counter*145+w] = x[w+3];   //copy data data elements and checksums into temp array
      }
      temp_array_length = temp_array_length + (frame_length-5); //counter to track how long the data is for error checking
      frame_counter++;    //increment to next frame

      if (frame_type-100 == frame_counter){  //all frames arrived
      arrived == true;
      frame_counter=0;    //reset in case errors have occured and need to loop again
      }


    }
    else {          //else if standard frame type set arrived to true after one frame
    arrived =true;
    temp_array = &x[0];   //set temp array as address of x[0] for error checking
    if (debug)
    Serial.println("all data recieved, start error checking");
    }

    //if arrived do error checking, else loop without error checking for speed, check later
    if (arrived==true){
      //acknowledge recieved frame, disconnect comm
      digitalWrite(Request_text_pin, LOW);
      delay(1);
      //check_for errors

      //if no errors present, the sum of the data and checksum modulo 256 should equal 0

      int errors =0;
      for (int error_counter =3; error_counter<frame_length-1; error_counter++){
        errors = errors + x[error_counter];
      }
      errors = errors%256;

      if (debug){
      Serial.print("errors present: ");
      Serial.println(errors);
      for (int j=0; j<frame_length; j++){
        Serial.print((char) x[j]);
        }
        Serial.println("");
      }

      //if errors present, repeat entire process, request again
      if (errors != 0){
        flag2=0;    //confirm it will loop
      }
      else if (errors ==0){
        flag2=1;   //otherwise exit
      }
    }
  }
  while(flag2==0);   //loop until entire data recieved with no errors

  if (debug)
  Serial.println("frame recieved with no errors");


  //if no errors, copy data to relevent location

    //case of scrolling text string
    if (frame_type == 1){
    int k = 3;      //start at initial data element
    while (k <= frame_length-2) { //copy all data up to frame length
      str[k] = (uint8_t)x[k];     //cast required
      k++;
      }
    }

    //else if (frame_type ==2)    //vertical scroll
    //new_string =1   //include this to identify if lines array needs updating
    //...


    if (mem_allocated ==true){    //if memory was allocated in recieving this frame, deallocate
       free(temp_array);
    }
  }




  void receiveEvent(int bytes) {    //interrupt to read in new string
  if (debug)
  Serial.println("data recieved, interupt triggered");

  //if (digitalRead(Enable_com)== HIGH){    //confirm the character is for this board
    if (debug)
    Serial.println("data for this arduino");

    x[i] = Wire.read();    // read one character from the I2C

    //extract info from frame
    if (i == 1) { //frame size byte, entire frame including heading and trailer bytes
      frame_length = x[i];
    }

    if (i == 2) { //frame type byte, used to decied what to do with recieved data
      frame_type = x[i];
    }

   //exit conditions when full frame recieved

   if (i == frame_length) {    //end of frame reached, det flag high, allow wait loop to exit
      flag1 = 1;
      i = 0;          //reset i
      frame_length = max_frame_size; // set frame length to default length for next iteration
    }
    else {                       //otherwise increment i
      i++;
    }
  //}
  }



  void check_serial(){            //check for serial data input
  String input_str = "";

  if (Serial.available()>0){
    int i =0;
      while(Serial.available()>0){
        input_str = Serial.readString();
        i++;

      }
      if (input_str == "debug"){
        debug = !debug;
      }

      if (debug){
        Serial.println(input_str);

      }

  }
  }
*/
