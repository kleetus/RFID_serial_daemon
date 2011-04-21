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
unsigned timer;
char charcount, error, i, ans;
char cardnum[16];

void
interrupt() {
    timer++;
    TMR0L = 96;
    INTCON = 0x20; // Set T0IE, clear T0IF
}

void
main() {
    OSCCON = 0b11111111; //8Mhz
    ADCON1 |= 0x0F;
    T0CON = 0xC4;
    TMR0L = 96; //preload for about 1 second every 400 interrupts (roughly)
    INTCON.TMR0IE = 1; //enable timer0 interrupt
    TRISB = 0; 
    TRISA = 0;
    LATA = 0x00;
    LATA.F0 = 1; //turn on the door transistor
    LATA.F1 = 1; //turn on the rfid reader
    LATB.F7 = 0; //this is the green light
    LATB.F6 = 0; //this is the red light
    timer = 0; //global interrupts are still off

    UART1_Init(9600);

    Soft_UART_Init(&PORTB, 1, 0, 9600, 0); //B1 = receive, B0 = transmit, 9600, do not invert
    Delay_ms(100);

     while(1) {
         /*
         if we cycled around for about 1.5 seconds with stuff in the 
	 send buffer (cardnum) and there is not enough to send to the server, 
         then clear it out and start over
				 */
			   if(timer>600) { timer=0; memset(cardnum, '\0', 16); } 
			
         charcount=0;

         if(UART1_Data_Ready()) {
             INTCON.GIE = 0; //we don't want interrupts preempting the reading of card data
             timer = 0; //we've taken in chars, reset the counter
             cardnum[charcount++] = UART1_read();
             INTCON.GIE = 1;
         }

         if(charcount>15) {
             LATA.F1 = 0; //turn off the rfid card reader, we are sending the last scanned card, don't want people scanning during this time

             INTCON.GIE = 0; //we don't want interrupts preempting our sending of data

             for(i=0; i<charcount; i++) {
                 Soft_UART_Write(cardnum[i]); //TODO: or call a custom function that handles collision avoidance
             }

             timer = 0;

             INTCON.GIE = 1;

             ans = Soft_UART_Read(&error); //try to read once, we will most likely fail the first time (not enough time for the data to process)
             while(error) {
               if(timer>600) { INTCON.GIE = 0; goto coms_error; } //we've spent too long waiting for a reply, give up
               ans = Soft_UART_Read(&error);
             }

             INTCON.GIE = 0; //we shouldn't need interrupts until we are out of the cardnum sending handler

             if(ans == '1') {
                 LATA.F0 = 0;
                 LATB.F7 = 1;
                 Delay_ms(5000);
                 LATA.F0 = 1;
                 LATB.F7 = 0;
             }
             else if(ans == '0') {
                 LATB.F6 = 1;
                 Delay_ms(2500);
                 LATB.F6 = 0;
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
         INTCON.GIE = 1; //turn on the global interrupt
         LATA.F1 = 1; //turn the rfid reader back on
			 	 LATA.F0 = 1; //ensure the door is locked again 
     }
}
