/* main.c

   This file written 2024 by Artur Podobas and Pedro Antunes

   For copyright and licensing, see file COPYING */


/* Below functions are external and found in other files. */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(char*);
extern void time2string(char*,int);
extern void tick(int*);
extern void delay(int);
extern int nextprime( int );

int mytime = 0x5957;
char textstring[] = "text, more text, and even more text!";
char btn_on_string[] = "value of button 2 == '1'";
char btn_off_string[] = "value of button 2 == '0'";

#define LEDS ((volatile int*) 0x04000000)
#define DISPLAY_BASE 0x04000050
#define DISPLAY_OFFSET 0x10
#define SWITCH_ADDRESS ((volatile int*) 0x04000010)
#define PUSH_BTN_ADDR ((volatile int*) 0x040000D0)

/* 0 = LED ON*/
static const unsigned char display_values[11] = {
  0b01000000, // 0
  0b01111001, // 1
  0b00100100, // 2
  0b00110000, // 3
  0b00011001, // 4 
  0b00010010, // 5 
  0b00000010, // 6 
  0b01111000, // 7
  0b00000000, // 8
  0b00010000, // 9
  0b01111111  // '.'

  };

//Sets the LEDs, MSB = left, LSB = right
void set_leds(int led_mask){
  led_mask = led_mask & 0x3FF; //0x3FF = 11 1111 1111
  *LEDS = led_mask;
}

//Sets the value of the 7-segment displays, display 0 == furthest right
void set_displays(int display_number, int value){
volatile unsigned char* display_address = (volatile unsigned char*)(DISPLAY_BASE + display_number * DISPLAY_OFFSET);
*display_address = display_values[value];
}

//Returns value of switches as a 10 bit value MSB = SW9, LSB = SW0
int get_sw(){
  return *SWITCH_ADDRESS & 0x3FF;

}

//Returns value of second push-button: Either 0/1
int get_btn(){
  return *PUSH_BTN_ADDR & 0x1;
}

void print_switches(){
  int switch_values = get_sw();
  char buffer[26] = "Switch values: ";

  buffer[15] = (switch_values & 0x200) ? '1' : '0';  // bit 9
  buffer[16] = (switch_values & 0x100) ? '1' : '0';  // bit 8
  buffer[17] = (switch_values & 0x080) ? '1' : '0';  // bit 7
  buffer[18] = (switch_values & 0x040) ? '1' : '0';  // bit 6
  buffer[19] = (switch_values & 0x020) ? '1' : '0';  // bit 5
  buffer[20] = (switch_values & 0x010) ? '1' : '0';  // bit 4
  buffer[21] = (switch_values & 0x008) ? '1' : '0';  // bit 3
  buffer[22] = (switch_values & 0x004) ? '1' : '0';  // bit 2
  buffer[23] = (switch_values & 0x002) ? '1' : '0';  // bit 1
  buffer[24] = (switch_values & 0x001) ? '1' : '0';  // bit 0
  buffer[25] = 0;

  display_string(buffer);
  
}
/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{}

/* Add your code here for initializing interrupts. */
void labinit(void)
{}

/*At start of program count in binary from 0 to 15, 1 second delay*/
void led_assignment(){
  int seconds = 0;
  set_leds(0);

  while (1) {
    time2string( textstring, mytime ); // Converts mytime to string
    display_string( textstring ); //Print out the string 'textstring'
    delay( 1000 );          // Delays 1 sec (adjust this value)
    tick( &mytime );     // Ticks the clock once

    seconds++;
    set_leds(seconds);

    if((seconds & 0xF)==0xF){ //0xF = 1111
      break;
    }    
  }
  
}

void btn_test(){
  if(get_btn() == 1){
    display_string(btn_on_string);
  }else{
  display_string(btn_off_string);
  }
}

/*Shows the time on the 7-Segment displays as HH:MM:SS
Inparameter: mytime (int): the time represented */
void time_to_displays(int mytime){
  int hours = (mytime >> 16) & 0xFF;
  int minutes = (mytime >> 8) & 0xFF;
  int seconds = mytime & 0xFF;

    set_displays(0, seconds & 0xF);   // ental
    set_displays(1, (seconds >> 4) & 0xF); // tiotal

    set_displays(2, minutes & 0xF);
    set_displays(3, (minutes >> 4) & 0xF);

    set_displays(4, hours & 0xF);
    set_displays(5, (hours >> 4) & 0xF);
}

// Hjälpfunktion: konverterar decimal till BCD
int dec_to_bcd(int val) {
    return ((val / 10) << 4) | (val % 10);
}

/*Function that modifies the time based on the switches position.
*/
void set_time(){
      int switches = get_sw();
      int mode = (switches >> 8) & 0x3; //Get SW9-SW8
      int value = switches & 0x3F;      //Get SW5-SW0

      if(mode == 0x1){
        int reset_seconds = mytime & 0xFFFF00;
        int new_seconds = dec_to_bcd(value);
        mytime = reset_seconds | new_seconds; //OR
      } 
      else if(mode == 0x2){
        int reset_minutes = mytime & 0xFF00FF;
        int new_minutes = dec_to_bcd(value) << 8; //Push value to bits 8-15
        mytime = reset_minutes | new_minutes;
      }
      else if(mode == 0x3){
        int reset_hours = mytime & 0x00FFFF;
        int new_hours = dec_to_bcd(value) << 16;
        mytime = reset_hours | new_hours;
      }
}


/*Infinite loop, time displayed on the 7-seg-displays as HH:MM:SS
When button is pressed, read switch values and update time as follows:
MSB 01: modify second counter
MSB 10: Modify minute counter
MSB 11: Modify hour counter
-based on the 6 LSB of the switch values
*/
void time_display_loop(){
  while (1)
  {
    time2string(textstring, mytime);
    display_string(textstring);
    time_to_displays(mytime);

    if(get_btn()==1){
      set_time();
    }

    delay(1000);
    tick(&mytime);
  }
}

/* Your code goes into main as well as any needed functions. */
int main() {
  // Call labinit()
  labinit();

  //led_assignment();
  btn_test();

  print_switches();

  time_display_loop();
  
  /*
  while (1) {
    time2string( textstring, mytime ); // Converts mytime to string
    display_string( textstring ); //Print out the string 'textstring'
    delay( 1000 );          // Delays 1 sec (adjust this value)
    tick( &mytime );     // Ticks the clock once
  }
    */
}



