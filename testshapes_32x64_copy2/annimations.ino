//void bouncing_balls();
//void home_balls();
//void scroll_text_horizontal();
//void scroll_text_vertical();
//void update_lines_array()


void bouncing_balls() {
  // Bounce three balls around
  int radius = 5;
  if (digitalRead(Reset) == LOW) {       //only draw circles once reset enabled, otherwise reset all positions
    for (byte i = 0; i < 3; i++) {
      // Draw 'ball'

      if (ball[i][0] > index1 - radius && ball[i][0] < index1 + 64 + radius)                                 //if ball should be visable on screen
        matrix.fillCircle(ball[i][0] - index1, ball[i][1], radius, (uint16_t)current_light_scale * pgm_read_word(&ballcolor[i])); //draw ball, correcting for ralative x position

      // Update X, Y position
      ball[i][0] += ball[i][2];
      ball[i][1] += ball[i][3];

      // Bounce off edges
      if ((ball[i][0] == radius) || (ball[i][0] == (TOT_WIDTH - radius))) //left and right extremes
        ball[i][2] *= -1;
      if ((ball[i][1] == 0) || (ball[i][1] == (matrix.height() - 1)))     //top and bottom extremes
        ball[i][3] *= -1;
    }
  }

  else {                                //reset all balls to know positions
    home_balls();

  }
}


void scroll_text_horizontal() {
  // Draw big scrolly text on top
  matrix.setTextWrap(false);
  matrix.setTextSize(text_size);     // size 1 == 8 pixels high

  if (text_colour == 0)
    matrix.setTextColor((uint16_t)current_light_scale * matrix.ColorHSV(hue, 255, 255, true));
  else
    matrix.setTextColor(pgm_read_word(&text_colour));
    
  matrix.setCursor(textX, dist_from_top);
  matrix.print(str);

  counter++;          //count trig pulses from initial set
  hue += 7;           // keep hues in sync

  //dont decrement cursor position until text position is left of the right most column this matrix
  if (counter >= TOT_WIDTH - 64 - index1) {//if num clock pulses is greater than columns to the right of current screen
    textX--;            //move text
    if (debug) {
      Serial.print(F("textX: "));
      Serial.println(textX);
    }
  }

  // Move text left (w/wrap), increase hue
  if (textX <= textMin || (digitalRead(Reset) == HIGH && reset_flag == false)) {
    textX = matrix.width() + 1;
    counter = 0;
    reset_flag = true;
    if (debug)
      Serial.println(F("reset positions"));
  }
  else if (reset_flag == true && digitalRead(Reset) == LOW) {
    reset_flag == false;
  }

  if (hue >= 1536) hue -= 1536;

  if (digitalRead(Reset) == HIGH) hue = 0;
}


void scroll_text_vertical() {
  // Draw big scrolly text on top
  /*  if (new_string){
      update_lines_array();
    }


    matrix.setTextWrap(true);
    matrix.setTextSize(text_size);     // size 1 == 8 pixels high

    if (text_colour == 0)
    matrix.setTextColor(matrix.ColorHSV(hueY, 255, 255, true));
    else
    matrix.setTextColor(pgm_read_word(&text_colour));



    counter++;          //count trig pulses from initial set
    hueY += 7;           // keep hues in sync

    num_lines = sizeof(vert_str)/(TOT_WIDTH/(6*text_size));
    textMinY = -(text_size*text_height)*num_lines;
    int line_spacing = text_height + (text_size/4);    //spacing for seperation of lines and cursor positions

    for (int disp_line=0;disp_line < max_lines; disp_line++){   //loop through array of structures and display the content is valid and should be on screen

      if(lines[disp_line].valid == 1){                                                    // if valid
        if (textY < textMinY + 32 + ((num_lines-disp_line)*line_spacing)){                 // check if should be on screen
          if (textY >= textMinY + ((num_lines-disp_line)*line_spacing) - text_height){

          int cursor_pos = textY + (disp_line*line_spacing);    //find cursor y pos for this line
          matrix.setCursor(-index+2, cursor_pos);                 //x pos always left most pixel+2
          matrix.print(lines[disp_line].line);
          }
        }
      }
    }
    if (counter >= textMinY) // decrement textY while counter is greater than textMinY value
      textY--;            //move text


    // reset text pos
    else if ((textY) < textMinY || digitalRead(Reset) == HIGH) {
      textX = matrix.height();    //reset position to just off bottom of screen
      counter = 0;                //reset counter
    }

    if (hueY >= 1536) hueY -= 1536;

    if (digitalRead(Reset) == HIGH) hueY = 0;  */
}





void home_balls() {

  ball[0][0] = 13;
  ball[0][1] = 10;
  ball[0][2] = 1;
  ball[0][3] = 1;

  ball[1][0] = 17;
  ball[1][1] = 15;
  ball[1][2] = 1;
  ball[1][3] = -1;

  ball[2][0] = 30;
  ball[2][1] = 30;
  ball[2][2] = -1;
  ball[2][3] = 1;
}

/*
  void update_lines_array(){    //function to take the new string and move it into the lines array
  int alpha;
  int beta;
  for(alpha =0; alpha<max_lines; alpha++){    //set all elements of array as invalid
    lines[alpha].valid=0;
  }

  int char_per_line = (TOT_WIDTH-4)/text_width;   //(total width - margin)/character width
  int current_line =1;
  int chars_copied = 0;    //offset for loop, number of characters copied defines start point of scanning for space
  for(alpha = chars_copied +(char_per_line*current_line); alpha >= chars_copied + (char_per_line*current_line) - char_per_line; alpha--){   //go to end of current line and go backwards until space found

     if (vert_str[alpha] == char(32)){   //character is space
          lines[current_line-1].valid=1;    //set line as valid

          for(beta= chars_copied; beta<alpha; beta++){
            lines[current_line-1].line[beta-chars_copied] = vert_str[beta];  //copy part of string into array element
          }

         chars_copied = chars_copied + alpha; //move chars_copied offset
         current_line++;    //increment current line

     }


      else if(alpha == chars_copied+1){                    // case of word longer than width of screen or end of string, simply split word

          for(beta= chars_copied; beta<(char_per_line+chars_copied); beta++){
            lines[current_line-1].line[beta-chars_copied] = vert_str[beta];  //copy part of string into array element
          }

         chars_copied = chars_copied + alpha; //move chars_copied offset
         current_line++;    //increment current line

      }
  }
  new_string=0;   //set new string variable to zero so as this does not execute again until a new string has arrived
  }*/


void mode_change_display() {      //display details of current mode

  if (debug) {
    Serial.print(F("mode change just occured mode is now: "));
    Serial.println(mode);
  }

  matrix.fillScreen(0);
  matrix.setTextWrap(false);

  if (debug) {          //two modes, debug displays more details

  }

  matrix.setTextSize(2);     // size 1 == 8 pixels high
  matrix.setTextColor((uint16_t) current_light_scale * pgm_read_word(&text_colour));
  matrix.setCursor(5, 10);
  matrix.print("Mode: ");
  matrix.print(mode);


  matrix.swapBuffers(false);    //swap buffers, write to one display the other
  counter ++;                   //keep track of counter, assume there wont be issues with wrapping as function only called for
}


void disp_static_text() {

  int offset = (TOT_WIDTH - SCREEN_NUM * 64) - TOT_WIDTH; //offset of screen from left hand side, write text from here, should be negative for all but left screen
  dist_from_top = (TOT_HEIGHT-(text_size*text_height))/2;    //center text on screen
  if (debug) {
    Serial.print(F("Offset: "));
    Serial.print(offset);
    Serial.print(F(", \tfrom top: "));
    Serial.println(dist_from_top);
  }
  matrix.setTextSize(2);     // size 1 == 8 pixels high
  matrix.setTextWrap(false);
  //sign_colour_calculator("blue");
  
  matrix.setCursor(0, 0);
  matrix.setTextColor(matrix.Color333(7,0,0));
  matrix.print("test");
}



