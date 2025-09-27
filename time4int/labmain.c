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
extern void enable_interrupt(void);

int mytime = 0x5957;
char textstring[] = "text, more text, and even more text!";
int timeoutcount = 0;
int prime = 1234567;

#define LEDS ((volatile int*) 0x04000000)
#define DISPLAY_BASE 0x04000050
#define DISPLAY_OFFSET 0x10
#define SWITCH_ADDRESS ((volatile int*) 0x04000010)
#define PUSH_BTN_ADDR ((volatile int*) 0x040000D0)

// Timer addresses, each register is 16 bits, pointers to 2byte (short).
#define TIMER_STATUS_ADDR ((volatile unsigned short*) 0x04000020) //Timer status address Bit 0: TO, Bit 1: RUN
#define TIMER_CONTROL_ADDR ((volatile unsigned short*) 0x04000024) /*Control: Bit 0: ITO, Bit 1: CONT, Bit 2: START, Bit 3: STOP*/
#define TIMER_PERIOD_LOW_ADDR ((volatile unsigned short*) 0x04000028) //Timer period low address [0-15]
#define TIMER_PERIOD_HIGH_ADDR ((volatile unsigned short*) 0x0400002C) //Timer period high address [16-31]

//Lista över värden för 7-segmentsdisplayen. 0 = LED ON
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

/* Sätter värdet på 7-segmentsdisplayen.
Inparameter: Display nummer (0-5) och värdet som ska visas (0-9 eller 10 för punkt).
Utparameter: void
*/
void set_displays(int display_number, int value){
volatile unsigned char* display_address = (volatile unsigned char*)(DISPLAY_BASE + display_number * DISPLAY_OFFSET); //8bit ptr to the display address
*display_address = display_values[value];
}

/* Sätter värdet på LEDsen, MSB = vänster, LSB = höger. Maskar till 10 bitar.
*/
void set_leds(int led_mask){
  led_mask = led_mask & 0x3FF; //0x3FF = 11 1111 1111
  *LEDS = led_mask;
}

/* Returnerar värdet på switcharna som ett 10 bitars värde MSB = SW9, LSB = SW0
Inparameter: void
Utparameter: int (0-1023)
*/
int get_sw(){
  return *SWITCH_ADDRESS & 0x3FF;

}

/* Returnerar värdet på den andra knappen: Antingen 0/1
Inparameter: void
Utparameter: int (0/1)
*/
int get_btn(){
  return *PUSH_BTN_ADDR & 0x1; //Maskar för att få endast LSB
}

/*Denna funktion läser av switcharna och skriver ut deras värde till terminalen.
Inparameter: void
Utparameter: void
*/
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

/*Denna funktion visar tiden på 7-segmentsdisplayen som HH:MM:SS
Inparameter: mytime (int): Tiden representerad som BCD i formatet 0xHHMMSS
Utparameter: void
*/
void time_to_displays(int mytime){
  int hours = (mytime >> 16) & 0xFF; // Extraherar bits 16-23 (HH)
  int minutes = (mytime >> 8) & 0xFF; // Extraherar bits 8-15 (MM)
  int seconds = mytime & 0xFF; // Extraherar bits 0-7 (SS)

    set_displays(0, seconds & 0xF);   //Bits 0-3, sekunder ental
    set_displays(1, (seconds >> 4) & 0xF); //Bits 4-7, sekunder tiotal

    set_displays(2, minutes & 0xF);  //Bits 0-3, minuter ental
    set_displays(3, (minutes >> 4) & 0xF); //Bits 4-7, minuter tiotal

    set_displays(4, hours & 0xF); //Bits 0-3, timmar ental
    set_displays(5, (hours >> 4) & 0xF); //Bits 4-7, timmar tiotal
}

/* Hjälpfunktion: konverterar decimal till BCD. Exempel: 45 decimal -> 0x45 BCD == 0100 0101
Inparameter: val (int): Decimalvärdet som ska konverteras till BCD
Utparameter: int: BCD-värde
*/
int dec_to_bcd(int val) {
    return ((val / 10) << 4) | (val % 10); //Dela med 10 för tiotal, modulo 10 för ental
}

/* Denna funktion modifierar tiden baserat på switcharnas position.
Inparameter: void
Utparameter: void
*/
void set_time(){
      int switches = get_sw();
      int mode = (switches >> 8) & 0x3; //Get SW9-SW8, 0x3 = 0000 0011
      int value = switches & 0x3F;      //Get SW5-SW0, 0x3f = 0011 1111

      if(mode == 0x1){ 
        int reset_seconds = mytime & 0xFFFF00; //Behåll HHMM, nollställ SS
        int new_seconds = dec_to_bcd(value); //Konvertera value till BCD
        mytime = reset_seconds | new_seconds; // HHMMSS = HHMM00 OR 00SS
      } 
      else if(mode == 0x2){
        int reset_minutes = mytime & 0xFF00FF;
        int new_minutes = dec_to_bcd(value) << 8; //Flytta value till bits 8-15
        mytime = reset_minutes | new_minutes;
      }
      else if(mode == 0x3){
        int reset_hours = mytime & 0x00FFFF;
        int new_hours = dec_to_bcd(value) << 16;//Flytta value till bits 16-23
        mytime = reset_hours | new_hours;
      }
}

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{
    *TIMER_STATUS_ADDR = 0; //Reset TO

    timeoutcount++;

    set_leds(0x3FF); 

    if (timeoutcount >= 10) {
        timeoutcount = 0;             // Reset counter
        tick(&mytime);                 // uppdatera tiden
        time_to_displays(mytime);      // visa på 7-seg-display
        }

}
/*Initialize the timer to generate an interrupt every second (assuming a 30 MHz clock).
*/
void labinit(void)
{
    int period = 3000000 - 1; //3MHz clock -> 0.1s period = 3,000,000 cykler
    *TIMER_PERIOD_LOW_ADDR = period & 0xFFFF; //Bit 0-15 av perioden
    *TIMER_PERIOD_HIGH_ADDR = (period >> 16) & 0xFFFF; //Bit 16-31 av perioden
    
    *TIMER_CONTROL_ADDR = 0b0111;
    // Bit 0: ITO=1 (interrupt enable)
    // Bit 1: CONT=1 (continuous mode)
    // Bit 2: START=1 (start timer)
    // Bit 3: STOP=0

    enable_interrupt();
}
void print_bin(unsigned int value) {
    char buffer[33]; // 32 bitar + nullterminator
    for (int i = 31; i >= 0; i--) {
        buffer[31 - i] = (value & (1u << i)) ? '1' : '0';
    }
    buffer[32] = '\0';
    print(buffer);
    print("\n");
}
void print_bitstatus(const char* name, int set) {
    print(" - ");
    print(name);
    print(" = ");
    print(set ? "1\n" : "0\n");
}

unsigned int read_mie() {
    unsigned int value;
    asm volatile ("csrr %0, mie" : "=r"(value));

    print("mie = ");
    print_bin(value);

    print_bitstatus("MSIE (Machine Software Interrupt Enable)", (value >> 3) & 1);
    print_bitstatus("MTIE (Machine Timer Interrupt Enable)",   (value >> 7) & 1);
    print_bitstatus("MEIE (Machine External Interrupt Enable)",(value >> 11) & 1);

    return value;
}

unsigned int read_mstatus() {
    unsigned int value;
    asm volatile ("csrr %0, mstatus" : "=r"(value));

    print("mstatus = ");
    print_bin(value);

    print_bitstatus("MIE  (Global Machine Interrupt Enable)",   (value >> 3) & 1);
    print_bitstatus("MPIE (Previous MIE)",                     (value >> 7) & 1);
    print_bitstatus("MPRV (Modify Privilege)",                  (value >> 17) & 1);
    print_bitstatus("MPP  (Machine Previous Privilege) bit0",  (value >> 11) & 1);
    print_bitstatus("MPP  (Machine Previous Privilege) bit1",  (value >> 12) & 1);

    return value;
}

int main(void) {
    labinit();

    read_mie();
    read_mstatus();

    while (1)
    {
        print("Prime: ");
        prime = nextprime(prime);
        print_dec(prime);
        print("\n");
    }
    
}

//https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#machine-status-registers-mstatus-and-mstatush
//MIE = Machine Interrupt Enable = bit3
/* Aktiverar avbrott genom att sätta MIE biten i mstatus == 1. MIE-biten = bit 3 i mstatus.

mtvec = Machine Trap-Vector Base-Address Register
mtvec används för att ange adressen till avbrottshanteraren (interrupt handler). 
När ett avbrott inträffar, hoppar processorn till den adress som är lagrad i mtvec 
för att börja köra avbrottshanteringskoden.
csrsi = "CSR Set Immediate" - instruktion för att sätta specifika bitar i en CSR (Control and Status Register).

enable_interrupt:
    #enable undefined bit (bit 16) in mie-register
    csrsi mie, 16

    #enable MIE (bit 3) i mstatus-register
    csrsi mstatus, 3

    #set mtvec, handle_interrupt
    la t0, _isr_handler
    csrw mtvec, t0

    ret

*/
