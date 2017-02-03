#include "sys.h"

// globals

#define ASC 48 // ascii A 

u8 threshold; // holds pot value
u8 menu_state = 0; // screen state
u8 button_state = 0; // used for button delay


// start message 
char welcome0[8] = { 'S', 'U', 'L', 'P', 'R', 'O'};
char welcome1[8] = { 13, 13, 13, 13, 13, 13}; 

const char lcd_init_bytes[] = {0x33,0x32,0x28,0x01,0x0c,0x06,0x00,0x00}; 

// used for lcd screen drawing
char lcd0[9] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x0 }; 
char lcd1[9] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x0 }; 

// current screen char
char *lcd_ptr;

void init(void); // inits
void lcd_init(void); // init lcd 
void tmr(void); // timer interrupt function, pwm
void lowrupt(void); // called on low priority interrupt
void hirupt(void); // called on high priority interrupt
void blink(void); // change alive led state
void button(void); // handle button press
void lcd_char(char c); // write one char to lcd screen
void welcome_str(void); // desplays welcome animation
void welcome_help(void); // helper function welcome message
void lcd_thres(u8 v); // display threshold 
void lcd_temp(u8 temp); // display temp
void num2txt(char *str, u8 val); // writes val into str as decimal number
void lcd(u8 line, const char *str); // show str on screen, line 0 = upper row, line 1 = lower row
void lcd_clear(char line); // clears lcd0 and lcd1 with spaces
u8 rpg(void); // read rpg value 
u8 pot(void); // get pot value
u8 read_temp(void); // get temp sensor value


// interrupts


#pragma code highVector=0x08
void atHighVector(void)
{
 _asm GOTO hirupt _endasm
}
#pragma code

#pragma code lowVector=0x18
void atLowVector(void)
{
 _asm GOTO lowrupt _endasm
}
#pragma code



void main()
{
    u8 pot_val = 0;
    u8 set_off = 0;
    u8 temp_val = 0;
    u8 tmp = 0;
    init();
    while(1) {
	button(); // button changes menu_state
         pot_val = pot();          
         temp_val = read_temp();
         if(temp_val <= pot_val) {
             // shutdown pwm 
             if(!set_off) {
                 TRISC = 0xff;                          
                 set_off = 1;
             }
         }
         // turn on pwm
         else if(set_off) {
             TRISC = 0xfb;            
             set_off = 0;
         }
         // update pwm value
         if(!set_off) {
	     // higher frequency when far from temparature 
             CCPR1L = 0xff - ((pot_val - temp_val) * 10); 
         }
	 // display values on screen
         if(menu_state) {
             lcd_temp(temp_val);             
         }
         else
             lcd_thres(pot_val);
    }
}

void init()
{
    ADCON0 = 0x61; // Sets up the A/D converter for the POT.   
    ADCON1 = 0x4e; // Enable PORTA & PORTE digital I/O pins for the A/D converter.
    // Enable PORTA & PORTE digital I/O pins for the A/D converter.
    
    TRISA = 0xe1; 
    TRISC = 0xfb;
    TRISB = 0xdb;
    TRISD = 0x0f;
    TRISE = 0x04;
    PORTA = 0x10; // leds off
     
    T2CONbits.TMR2ON = 1; // enable timer2
    T2CONbits.T2CKPS0 = 1; // prescaler: 11 = 1:16
    T2CONbits.T2CKPS1 = 1; 
    T2CONbits.TOUTPS0 = 0; // postscaler 0000 = 1:1
    T2CONbits.TOUTPS1 = 0; 
    T2CONbits.TOUTPS2 = 0; 
    T2CONbits.TOUTPS3 = 0;     
    RCONbits.IPEN = 1; // enable priority levels 
    IPR1bits.TMR2IP = 0; // timer2 low interrupt 
    PIE1bits.TMR2IE = 1; // enable interrupt on timer2
    TMR2 = 0; // initial value for timer2
    PR2 = 255; // end value for timer2
    INTCONbits.GIEL = 1; // low interrupts
    INTCONbits.GIEH = 1; // high interrupts    

    // pwm for ccp1
    CCP1CONbits.CCP1M2 = 1; 
    CCP1CONbits.CCP1M3 = 1; 

    CCP1CONbits.DC1B0 = 1; // pwm duty cycle <0:1>
    CCP1CONbits.DC1B1 = 1;     
    CCPR1L = 0x63; // duty cycle <9:2>
    
    threshold = 10;
    menu_state = 0;
    button_state = 0;
    lcd_init();
    lcd_clear(0); // clear line 0
    lcd_clear(1); //clear line 1
    welcome_str(); // start animation

}

#pragma interrupt hirupt
void hirupt(void)                   
{

}

#pragma interruptlow lowrupt
void lowrupt(void)
{
    // handles only interrups from timer2
    if (PIR1bits.TMR2IF) {
        tmr();
    }
}

void tmr()
{
    // tmr2 is reset automatically
    // pwm handles itself when TMR2IF is set to 1
    // needs to reset the flag
     PIR1bits.TMR2IF = 0;
}


// toggle alive led
void blink()
{
     if(PORTAbits.RA4)
         PORTAbits.RA4 = 0;
     else 
         PORTAbits.RA4 = 1;    
}


// change menu state when pressed
// buffer the press with button_state so the change happens when you release
void button()
{    
    if(PORTDbits.RD3 && button_state) {
        button_state = 0;
        menu_state = menu_state ? 0 : 1;
        return;
    }
    if(!PORTDbits.RD3) {
        button_state = 1;
    }
}

// needs to wait + write the init chars to screen
void lcd_init(void)
{
    DELAY_MS100();
    PORTEbits.RE0 = 0; // command
    lcd_ptr = lcd_init_bytes;
    while(*lcd_ptr) {
        lcd_char(*lcd_ptr); // write char to screen
        lcd_ptr++;
    }    
}

// displays welcome text from welcome0 and welcome1 one character at time
void welcome_str(void)
{
    u8 n = 0;
    // only 6 chars
    while(n < 6) {
	// initially show cleared screen
        if(n != 0) {
            lcd0[n - 1] = ' ';
            lcd1[n - 1] = ' ';
        }
        lcd0[n] = welcome0[n];
        lcd1[n] = welcome1[n];      
        welcome_help(); // show the chars for short time
        n++;
    }
    lcd_clear(0);
    lcd_clear(1); 
    welcome_help();
    DELAY_MS500();        
}

// shows the current screen state for second
void welcome_help(void)
{
    lcd(0, lcd0);
    lcd(1, lcd1);
    DELAY_MS500();
    DELAY_MS500();    
}
// display threshold
void lcd_thres(u8 v)
{
    const char thres_txt[9] = { 'T', 'H', 'R', 'E', 'S', ' ', ' ', ' ', 0 };
    lcd(0, thres_txt); // write whole line 1
    lcd_clear(1);  // needs to clear line 1
    num2txt(lcd1, v); // v into acsii
    lcd(1, lcd1); // write line
}
// display temp
void lcd_temp(u8 temp)
{
    const char temp_txt[9] = { 'T', 'E', 'M', 'P', ' ', ' ', ' ', ' ', 0 };
    lcd(0, temp_txt);    
    lcd_clear(1);
    num2txt(lcd1, temp); // temp can only be 3 numbers 
    lcd1[3] = 'C'; // temparature unit 
    lcd(1, lcd1);
}

// write value as ascii into str
void num2txt(char *str, u8 val)
{
    //val can only be max 3 numbers
    char *s = str;
    u8 tmp = 0;
    tmp = val / 100; // hunders 
    *s = ASC + tmp;  // as ascii number
    s++;
    val -= tmp * 100; 
    tmp = val / 10; // tens 
    *s = ASC + tmp; // as ascii 
    s++;
    val -= tmp * 10; // ones 
    *s = ASC + val;
}

// display one char 
void lcd_char(char c)
{
    PORTEbits.RE1 = 1; // lcd input 
    PORTD = c; // upper nibble
    PORTEbits.RE1 = 0; // send to lcd
    // needs to wait for the screen to process
    DELAY_MS1();
    DELAY_MS1();
    DELAY_MS1();
    DELAY_MS1();    
    DELAY_MS1();        
    PORTEbits.RE1 = 1; // lcd input
    c <<= 4; // lower nibble
    PORTD = c;
    PORTEbits.RE1 = 0; // send to lcd
    DELAY_MS1();
    DELAY_MS1();
    DELAY_MS1();
    DELAY_MS1();    
    DELAY_MS1();        
    
}

// display line
// 0 upper line, 1 lower line
void lcd(u8 line, const char *str)
{
    PORTEbits.RE0 = 0; // cursor position
    lcd_char(line ? 0xc0 : 0x80); // line 0 or line 1
    PORTEbits.RE0 = 1; // text 
    while(*str) {
        lcd_char(*str);
        str++;
    }          
}

// clear ucd buffer with spaces 
void lcd_clear(char line)
{
    u8 n = 0;
    char *ptr = line ? lcd1 : lcd0;
    while(n < 8) {
        ptr[n] = ' ';
        n++;
    }
    
}

// rotary pulse generator value
u8 rpg(void)
{
    u8 tmp = PORTD & 0x06; // only 2 bits are for rpg
    static u8 rpg_tmp = 0;
    if(tmp != rpg_tmp) {     
        rpg_tmp = tmp;
        return 1;
    }
    else {
        return 0;
    }    
}

// pot value
u8 pot(void)
{
    ADCON0bits.GO_DONE = 1; // start conversion
    while(ADCON0bits.GO_DONE) // wait 
        ;
    return ADRESH;    
}


// read value from temperature sensor
u8 read_temp()
{
    u8 temp_val = 0;
    ADCON0 = 0x41; // AN0 for temp sensor
    ADCON1bits.ADFM = 1; // ADRESL for temp sensor
    DELAY_US50(); // wait
    ADCON0bits.GO_DONE = 1; // start conversion
    while(ADCON0bits.GO_DONE) //wait 
        ;
    // convert read value into celcius 
    temp_val = ADRESL - 110; 
    temp_val = temp_val * 0.461;  //5256) / 11400 ~ 0.461
    ADCON0 = 0x61; // set for POT 
    ADCON1bits.ADFM = 0; // back to ADRESH 
    return temp_val;
}
