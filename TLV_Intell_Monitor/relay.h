#ifndef  _RELAY_H_
#define  _RELAY_H_
 
#ifndef ON
#define ON                  1
#define OFF                 0
#endif
 
/*   I/O Pin connected to PIN#40, BCM code pin number is 21 and wPi pin number is 29 */
#define RELAY_PIN           29
 
void turn_led(int cmd) ;
 
#endif   /* ----- #ifndef _RELAY_H_  ----- */
