


int Blue[3] = {0,0,7};    //define colour combinations
int Red[3] = {7,0,0};
int Green[3] = {0,7,0};
int White[3] = {7,7,7};


void sign_colour_calculator(String target_colour){

   if (target_colour == "blue"){
    colour.r = Blue[0];
    colour.g = Blue[1];
    colour.b = Blue[2];
   }
   else if (target_colour == "red"){
    colour.r = Red[0];
    colour.g = Red[1];
    colour.b = Red[2];
   }
   else if (target_colour == "green"){
    colour.r = Green[0];
    colour.g = Green[1];
    colour.b = Green[2];
   }

   //else if...
    colour.r = (int)(current_light_scale*colour.r)+0.5;   //round result to nearest integer
    colour.g = (int)(current_light_scale*colour.g)+0.5; 
    colour.b = (int)(current_light_scale*colour.b)+0.5; 
}

