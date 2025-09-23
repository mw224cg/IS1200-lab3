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

#define LEDS ((volatile int*) 0x04000000) //Address to on board LEDs 10bits
#define DISPLAY_BASE 0x04000050 //Display 1 address
#define DISPLAY_OFFSET 0x10 //Display address offset
#define SWITCH_ADDRESS ((volatile int*) 0x04000010) //Address to on board switches 10 bits
#define PUSH_BTN_ADDR ((volatile int*) 0x040000D0) //PUSH button nr2 address

// Timer addresses, each register is 16 bits.
#define TIMER_STATUS_ADDR ((volatile int*) 0x04000020) //Timer status address Bit 0: TO (1 when clock == 0), Bit 1: RUN == 1 if timer is running
#define TIMER_CONTROL_ADDR ((volatile int*) 0x04000022) /*Control: Bit 0: ITO, Bit 1: CONT, Bit 2: START, Bit 3: STOP*/
#define TIMER_PERIOD_LOW_ADDR ((volatile int*) 0x04000024) //Timer period low address [0-15]
#define TIMER_PERIOD_HIGH_ADDR ((volatile int*) 0x04000026) //Timer period high address [16-31]

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

//Sets the LEDs, MSB = left, LSB = right. Mask to 10 bits.
void set_leds(int led_mask){
  led_mask = led_mask & 0x3FF; //0x3FF = 11 1111 1111
  *LEDS = led_mask;
}

/*Sets the value of the 7-segment displays. 
Inparameter: The display number (0-5) and the value to show (0-9 or 10 for dot).
Outparameter: void*/
void set_displays(int display_number, int value){
volatile unsigned char* display_address = (volatile unsigned char*)(DISPLAY_BASE + display_number * DISPLAY_OFFSET);
*display_address = display_values[value];
}

/*Returns value of switches as a 10 bit value MSB = SW9, LSB = SW0
Inparameter: void
Outparameter: int (0-1023)
*/
int get_sw(){
  return *SWITCH_ADDRESS & 0x3FF;

}

//Returns value of second push-button: Either 0/1
int get_btn(){
  return *PUSH_BTN_ADDR & 0x1; //Mask to get LSB only
}

/*This function reads the switch values and prints them to the display.*/
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

/*Initialize the timer to generate an interrupt every second (assuming a 30 MHz clock).
*/
void labinit(void)
{
    int period = 3000000; // 300k clock cycles = 0,1 seconds at 30 MHz
    *TIMER_PERIOD_LOW_ADDR = (period & 0xFFFF); // Set the low 16 bits of the period
    *TIMER_PERIOD_HIGH_ADDR = (period >> 16) & 0xFFFF; // Set the high 16 bits of the period
    *TIMER_CONTROL_ADDR = 0b0111; // ITO (IRQ ON) = 1, CONT (Restart) = 1, START = 1, STOP = 0
}



/*This function takes the current time in BCD format and updates the 7-seg-displays
Inparameter: int mytime (in BCD format HH:MM:SS, 4 bits per value).
Outparameter: void
*/
void time_to_displays(int mytime){
  int hours = (mytime >> 16) & 0xFF; // Extract bits 16-23 by shifting right 16 and masking with 0xFF
  int minutes = (mytime >> 8) & 0xFF; // Extract bits 8-15 by shifting right 8 and masking with 0xFF
  int seconds = mytime & 0xFF; // Extract bits 0-7 by masking with 0xFF

    set_displays(0, seconds & 0xF);   //Bits 0-3, single seconds
    set_displays(1, (seconds >> 4) & 0xF); //Bits 4-7, ten seconds

    set_displays(2, minutes & 0xF);  //Bits 0-3, single minutes
    set_displays(3, (minutes >> 4) & 0xF); //Bits 4-7, ten minutes

    set_displays(4, hours & 0xF); //Bits 0-3, single hours
    set_displays(5, (hours >> 4) & 0xF); //Bits 4-7, ten hours
}

/*Help function: converts decimal values to BCD, e.g 45 -> 0x45 -> 0100 0101 
Inparameter: int val, the decimal value to convert
Outparameter: int, the BCD representation of the value
*/
int dec_to_bcd(int val) {
    return ((val / 10) << 4) | (val % 10); //Shift the tens to the left 4 bits and OR with the units
}

/*Function that modifies the time based on the switches position.
SW9-SW8: Mode (00 = no change, 01 = change seconds, 10 = change minutes, 11 = change hours)
SW5-SW0: Value to set the selected mode to (0-63 decimal)
*/
void set_time(){
      int switches = get_sw();
      int mode = (switches >> 8) & 0x3; //Get SW9-SW8, 0x3 = 0000 0011
      int value = switches & 0x3F;      //Get SW5-SW0, 0x3f = 0011 1111

      if(mode == 0x1){ 
        int reset_seconds = mytime & 0xFFFF00; //Mask to keep hours and minutes, reset seconds to 0
        int new_seconds = dec_to_bcd(value); //Convert the decimal value of switches to BCD
        mytime = reset_seconds | new_seconds; //OR to set the new seconds value
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
  
  while (1)
  {
    int timeoutcounter = 0;
    if(*TIMER_STATUS_ADDR == 1) //check if TO bit is == 1
    {
        *TIMER_STATUS_ADDR = 0; //reset TO, RUN bit not changed by writing to register
        timeoutcounter++;

        if(timeoutcounter >= 10)
        {
            tick(&mytime); //update time
            timeoutcounter = 0; //reset counter
            time2string(textstring, mytime);
            display_string(textstring);
            time_to_displays(mytime);
        }
    }
}
}
