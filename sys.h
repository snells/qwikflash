#ifndef SYS_H
#define SYS_H

#include <p18cxxx.h>
#include <delays.h>

#define SYS_FREQ        1000000L
#define _XTAL_FREQ        1000000L
#define FCY             SYS_FREQ/4


// easier to use basic types 
typedef unsigned char u8;
typedef char s8;    
typedef unsigned int u16;
typedef signed int s16;


// saner delay functions
#define DELAY_US10() do { Delay10TCYx(2); Nop(); Nop(); Nop(); Nop(); Nop(); } while(0)
#define DELAY_US50() Delay10TCYx(12)
#define DELAY_US100() Delay10TCYx(25)
#define DELAY_MS1() Delay100TCYx(25)
#define DELAY_MS5() Delay100TCYx(125)
#define DELAY_MS10() Delay1KTCYx(25)
#define DELAY_MS50() Delay1KTCYx(125)
#define DELAY_MS100() Delay1KTCYx(250)
#define DELAY_MS500() do { Delay10KTCYx(100); DELAY_MS100(); } while(0)

#endif
