
/*
 * Functions 
 * 
 * void init_sd_cards()     // initialisation process for sd cards
 * void remove_card_1()     // test and wait until sd1 removed
 * void extract_data()      // get copied data from sd2, network & password or default string
 * int in_header()          // return 1 if the colon is in the header, otherwise 0
 * int is_network()         // check if network stored in buffer
 * int is_password()        // check if password stored in buffer
 * int is_default()         // check if default string stored in buffer
 * int string_length()      // calculate the lenght of the relevent string in file
 * 
 * 
 */

 



SdFat sd1;
//const uint8_t SD1_CS = 40;  // chip select for sd1
SdFile file1;

SdFat sd2;
//const uint8_t SD2_CS = 4;   // chip select for sd2
SdFile file2;


const uint8_t BUF_DIM = 100;
uint8_t buf[BUF_DIM];

//const uint32_t FILE_SIZE = 1000000;
//const uint16_t NWRITE = FILE_SIZE/BUF_DIM;


char read_buffer[67];       //buffer to read some data, dont need ot read whole file at once, and doing so could be problematic if file large, 
                            //read 15 bytes to recognise id word (eg Network) and have 50 bytes for string (default could be long)

//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt_P(PSTR(msg))
#define initError(msg) initErrorHalt_P(PSTR(msg))
//------------------------------------------------------------------------------


    //declared in master_controler_cf
                             
//String str_sd ="123456789 123456789 123456789 123456789 123456789 ";   //sample string, needed to set the length correctly
//String Network = "init network";    //store current network
//String Password = "init password";   //store current password


//
//void setup(){
//  str_sd.reserve(150);
//  str_sd = "No input string found, can't connect to a network, ensure network information stored in memory";
//  Serial.begin(9600);
//  while (!Serial) {}  // wait for Leonardo
// 
//  init_sd_cards();
//  remove_card_1();
//
//  Serial.println("Card 1 removed, extract data...");
//  
//  extract_data();
// 
//  Serial.print("Network: ");    //print extracted data
//  Serial.println (Network);
//  Serial.print("Password: ");
//  Serial.println (Password);
//  Serial.print("Str: ");
//  Serial.println (str_sd);
//
//  input_str = str_sd;
//  
//  Serial.print(input_str);
//}
//void loop() {}
//






void init_sd_cards() {
  Serial.println(F("Sd init setup..."));
  int card1_init=0;
  int card2_init=0;
  PgmPrint("FreeRam: ");

  Serial.println(FreeRam());
  
  // fill buffer with known data
  for (int i = 0; i < sizeof(buf); i++) buf[i] = i;
  
//  PgmPrintln("type any character to start");
//  while (Serial.read() <= 0) {}
//  delay(400);  // catch Due reset problem

  // disable sd2 while initializing sd1
  pinMode(SD2_CS, OUTPUT);
  digitalWrite(SD2_CS, HIGH);



  
  // initialize the first card
  int beta =millis();
  int theta = beta;
  int alpha = 0;
  while (millis()<beta+40000 && card1_init ==0){
    if (sd1.begin(SD1_CS)) {
       card1_init = 1;              //loop exit conditions if card detected
    }
    else{
      if (alpha% 50==0)
      Serial.println(F("Insert sd card 1"));
      //send_frame... //display error on screen
      delay(20);
      alpha++;

      if (millis()>= theta+ 3000){   //flash built in led while waiting as notification if serial not connected
        digitalWrite(notifier, HIGH);
        delay(200);
        digitalWrite(notifier, LOW);
        delay(200);
        digitalWrite(notifier, HIGH);
        delay(200);
        digitalWrite(notifier, LOW);
        theta=millis();
      }
    }
  }

if (card1_init==0){
  Serial.println(F("No SD card detected"));
  sd1.initError("sd1:");    //stops code when called
}

  
  // create SIGN1 on sd1 if it does not exist
  // Use this to test if card is inserted, if directory can't be made, functional card not inserted (or wiring problem..., presume card)
  beta = millis();
  card1_init=0;
  alpha=0;
  
  while (millis()<beta+20000 && card1_init==0){
    
    if (!sd1.exists("/SIGN1")) {
      if (debug&& alpha%50 == 0)   //fast loop, do not print all itterations
      Serial.println(F("DIR 1 does not exist"));
      
      if (!sd1.mkdir("/SIGN1")) {
        if (debug && alpha%50 == 0)
        Serial.println(F("Can't creat DIR 1"));

        alpha++;
        if (alpha==10)    //print once, after short delay
        Serial.println(F("Please insure card inserted"));
        delay(20);
        
       }
       
    }
    else 
       card1_init=1;      //folder exists, exit
  }
  if (card1_init==0){
  sd1.errorExit("sd1.mkdir");   //while(1)...
  }

beta= millis();
alpha=0;

  while (millis()<beta+20000 && card2_init ==0){
    if (sd2.begin(SD2_CS)) {
       card2_init = 1;              //loop exit conditions if card detected
    }
    else{
      if (alpha% 100==0)
      Serial.println(F("Insert sd card 2"));   //promt for user
      //send_frame... //display error on screen
      delay(20);
      alpha++;
    }
  }

if (card2_init==0)
  sd1.initError("sd2:");    //stops code when called
     
 // create SIGN2 on sd2 if it does not exist
  if (!sd2.exists("/SIGN2")) {
    if (!sd2.mkdir("/SIGN2")) sd2.errorExit("sd2.mkdir");
  }
  if(debug){
  // list root directory on both cards
  PgmPrintln("------sd1 root-------");
  sd1.ls();
  PgmPrintln("------sd2 root-------");
  sd2.ls();
  }

  // make /SIGN1 the default directory for sd1
  if (!sd1.chdir("/SIGN1")) sd1.errorExit("sd1.chdir");
  
  // make /SIGN2 the default directory for sd2
  if (!sd2.chdir("/SIGN2")) sd2.errorExit("sd2.chdir");
  
  // list current directory on both cards
  if (debug){
  PgmPrintln("------sd1 SIGN1-------");
  sd1.ls();
  PgmPrintln("------sd2 SIGN2-------");
  sd2.ls();
  PgmPrintln("---------------------");
  }


  // set the current working directory for open() to sd1
  sd1.chvol();
  
  // create or open /SIGN1/TEST.BIN and truncate it to zero length

  if (!file1.open("TEST1.BIN", O_RDWR | O_CREAT )){
    sd1.errorExit("file1");
  }

  // set the current working directory for open() to sd2
  sd2.chvol();
  
  // create or open /SIGN2/TEST2.BIN and truncate it to zero length
  
  if (!file2.open("TEST2.BIN", O_WRITE | O_CREAT | O_TRUNC)) {
    sd2.errorExit("file2");
  }
  PgmPrintln("Copying TEST1.BIN to TEST2.BIN");
  
  // copy file1 to file2
  file1.rewind();
  uint32_t t = millis();

  while (1) {
    int n = file1.read(buf, sizeof(buf));
    if (n < 0) sd1.errorExit("read1");
    if (n == 0) break;
    if (file2.write(buf, n) != n) sd2.errorExit("write2");
    digitalWrite(notifier, !digitalRead(notifier));
  }
  
  if (debug){
  t = millis() - t;
  PgmPrint("File size: ");
  Serial.println(file2.fileSize());
  PgmPrint("Copy time: ");
  Serial.print(t);
  PgmPrintln(" millis");
  }
  
  // close files
  file1.close();
  
  file2.close();

  PgmPrintln("Done Transfer\n");
}
//------------------------------------------------------------------------------


void remove_card_1(){
  int theta = millis();
  int card1_init=0;
  int alpha=0;
  while (card1_init ==0){
    if (!sd1.begin(SD1_CS)) {
       card1_init = 1;              //loop exit conditions if card detected
    }
    else{
      if (alpha ==10){              //prompt
      Serial.println(F("Remove sd card 1"));
      //send frame to matrix...
      }
      
    delay(50);
    alpha++;
    }

    if (millis()>= theta+ 3000){   //flash built in led while waiting as notification if serial not connected
        digitalWrite(notifier, HIGH);
        delay(200);
        digitalWrite(notifier, LOW);
        delay(200);
        digitalWrite(notifier, HIGH);
        delay(200);
        digitalWrite(notifier, LOW);
        theta=millis();
      }
  }
}



//function to extract data from the sd card on the ethernet shield, passwords should be copied when this is called
void extract_data(){

    char temp_password[25];   //temp arrays to copy data into if found, makes converting from array of lots of data to string of only selected data easier
    char temp_network[25];
    char temp_default [50];
    
    int str_len =0;
    bool Connected =false;      //value to define if ethernet/wifi connected with current network and password, if true stop looping through file
    int n=1;                    //define n as non zero
    int alpha=0;
    int beta=0;

    sd2.chvol();

    if (!file2.open("TEST2.BIN", O_READ)){        //open file for reading
       if (debug)
       Serial.println(F("Can't open file to extract data"));
       return;
    }
 
    while(!Connected && n != 0){    //while Connected not set to 1 and file end no reached
      
    file2.seekSet(alpha);       //set cursor position, and increment by one on each loop (probably better way to do this)
    alpha++;
    n = file2.read(read_buffer, sizeof(read_buffer));   //returns 0 if file finished, negative if error
    if (n < 0) sd2.errorExit("read2");

//    Serial.print("alpha: ");
//    Serial.println(alpha);
//    Serial.println(read_buffer);
//    Serial.println("");
    if (read_buffer[14] == ':'){    // if it equals a colon it could be a password or network
          if (debug)
          Serial.println(F("colon found"));
          
          if (in_header()){ // check if colon is from header, such as examples
            //do nothing
            if (debug)
            Serial.println(F("colon in header"));
          }
          
          else {            // if not, presume to be valid
            if(is_network()){
              str_len = string_length();    //call function to find length of string
              
              for(beta=0; beta<str_len; beta++){    //loop through network name
                Network[beta] = read_buffer[beta+16]; // offset to where string starts
                
              }
              Network.remove(beta);                
              //Network = temp_network;
            }

            else if(is_password()){
              str_len = string_length();    //call function to find length of string
              
              for(beta=0; beta<str_len; beta++){    //loop through network name
                 Password[beta] = read_buffer[beta+16]; // offset to where string starts
               
              }
              //Serial.println(beta);
              Password.remove(beta);
                             
              //Password = temp_password;
              // attempt to connect to network using network and password...

              
              if (!debug)   //if debug, read through entire file
              Connected=1;    //just assume to have connected for now once network and password detected
              
            }
            
            else if(is_default()){
              str_len = string_length();    //call function to find length of string
              if (debug){
              Serial.print(F("string length: "));
              Serial.println(str_len);
              }

              beta=0;
              while(beta!=str_len){    //loop through network name
                str_sd[beta] = read_buffer[beta+16]; // offset to where string starts
                if (debug)
                Serial.print(str_sd[beta]);
                beta++;
              }
              Serial.println("");
//
//              
//              Serial.println("");
//              Serial.print("beta: ");
//              Serial.println(beta);
              str_sd.remove(str_len);         
              //str_sd = temp_default;
              
            }
          }
      }
    }
    file2.close(); 
    if (Connected){
      str_sd = "Connected to network";    //set string as something for clarity
      str_sd.remove(str_sd.length());      //remove excess characters of string
    }
}

int in_header(){          //return 1 if the colon is in the header, otherwise 0

  if (read_buffer[15] == '"')
    return 1;

  if (read_buffer[16] == 'E' && read_buffer[17] == 'x' && read_buffer[18] == 'a' && read_buffer[19] == 'm' && read_buffer[20] == 'p' && read_buffer[21] == 'l' && read_buffer[22] == 'e' )//&& (read_buffer[23] == '\n' || read_buffer[23] == '\t'))
    return 1;

  return 0;
}


int is_network(){ //check if network stored in buffer
  if (read_buffer[7] == 'N' && read_buffer[8] == 'e' && read_buffer[9] == 't' && read_buffer[10] == 'w' && read_buffer[11] == 'o' && read_buffer[12] == 'r' && read_buffer[13] == 'k')
    return 1;
  return 0;
  
}

int is_password(){    //check if password stored in buffer
  if (read_buffer[6] == 'P' && read_buffer[7] == 'a' && read_buffer[8] == 's' && read_buffer[9] == 's' && read_buffer[10] == 'w' && read_buffer[11] == 'o' && read_buffer[12] == 'r' && read_buffer[13] == 'd')
    return 1;
  return 0;
}

int is_default(){     //check if default string stored in buffer
  if (read_buffer[7] == 'D' && read_buffer[8] == 'e' && read_buffer[9] == 'f' && read_buffer[10] == 'a' && read_buffer[11] == 'u' && read_buffer[12] == 'l' && read_buffer[13] == 't')
    return 1;
  return 0;
}

int string_length(){
  int alpha=16;
  while (read_buffer[alpha] != '\n' && read_buffer[alpha] != '\t'){   //if the current index is a carriage return exit
  alpha++;    //increment alpha
  }
  return alpha-16-1;    //alpha-offset-1
}



