/*

 pins:
	the only required connections to the PIC are
	(2) Vss (which is GND)
	(1) Vdd (which is +5v)
	any IO Ports required (listed below)
	using internal OSC + PLLx4 to achieve 8Mhz
	MCLR does not need a pull resistor
         B7 is green light meaning entry is allowed while lit
         B6 is red light meaning entry is not allowed due to an error or explicit rejection by server
         A0 is the port and asserts to lock the door, deasserts to allow access
         A1 is the reset pin for the rfid

*/
char count, i;
unsigned short data = 0, ro = 0; 
unsigned short *er;
unsigned long timer; //this is 32bit..4.2ish billion

void
main() {
	OSCCON = 0b11111111; //8Mhz
	ADCON1 |= 0x0F;
	TRISA = 0;
	TRISB = 0; 
	LATA.F0 = 1; //turn on the door transistor
	LATA.F1 = 1; //turn on the rfid reader
	LATB.F7 = 0; //this is the green light
	LATB.F6 = 0; //this is the red light
	er = &ro;

	UART1_Init(9600);
	//interrupts should not be used because the soft uart routines need hard real time accouting for time use
  Soft_UART_Init(&PORTB, 1, 0, 9600, 0); //B1 = receive, B0 = transmit, 9600, do not invert

  Delay_ms(100);

  while(1) {
  	if(UART1_Data_Ready()) {
    	Soft_UART_Write(UART1_read());
    }
		if(count>15) { //this is when can expect an answer from the servers
			LATA.F1 = 0; //turn off the rfid card reader, we are sending the last scanned card, don't want people scanning during this time

			timer = 0;
			do {
				timer++;
				if(timer>=0xFFFFF) { goto coms_error; } // <- FFFFF is about a million, make this a sane time out, not sure how long it would take to count this high up
				data = Soft_Uart_Read(er); } 
			while (*er);
		
			if(data == '1') {
			    LATA.F0 = 0;
			    LATB.F7 = 1;
			    Delay_ms(5000); //not sure what a good value to keep the door open
			    LATA.F0 = 1;
			    LATB.F7 = 0;
			}
			else if(data == '0') {
				for(i=0; i<5; i++) {
			    LATB.F6 = ~LATB.F6;
			    Delay_ms(600);
				}
			}
			else {
coms_error:
				LATB.F6=1;
				LATB.F7=0;
				for(i=0; i<10; i++) {
					LATB.F6 = ~LATB.F6;
			 		LATB.F7 = ~LATB.F7;
			 		Delay_ms(200);
			   }
			}
		}	
		LATA.F1 = 1; //turn the rfid reader back on
		LATA.F0 = 1; //ensure the door is locked again 
	}
}
