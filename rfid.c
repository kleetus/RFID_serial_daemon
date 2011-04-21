unsigned cnt;
char count, error, i, ans, counter;
char cardnum[16];

void interrupt() {
     cnt++;
     TMR0L = 96;
     INTCON = 0x20; // Set T0IE, clear T0IF
}

void main() {
     OSCCON = 0b11111111; //8Mhz
     ADCON1 |= 0x0F;
     T0CON = 0xC4;
     TMR0L = 96; //preload for about 1 second every 400 interrupts (roughly)
     INTCON.TMR0IE = 1; //enable timer0 interrupt
     TRISB = 0; 
     TRISA = 0;
     LATA=0x00;
     LATA.F0 = 1;
     LATA.F1 = 1;
     LATB.F7 = 0;
     LATB.F6 = 0;
     cnt = 0; //global interrupts are still off

     UART1_Init(9600);

     Soft_UART_Init(&PORTB, 1, 0, 9600, 0); //B1 = receive, B0 = transmit, 9600, do not invert
     Delay_ms(100);

     while(1) {
				 /*
				   if we cycled around for about 1.5 seconds with stuff in the 
					 send buffer (cardnum) and there is not enough to send to the server, 
					 then clear it out and start over
				 */
			   if(cnt>600) { cnt=0; memset(cardnum, '\0', 16); } 
			
         count=0;

         if(UART1_Data_Ready()) {
             INTCON.GIE = 0; //we don't want interrupts preempting the reading of card data
             cnt = 0; //we've taken in chars, reset the counter
             cardnum[count++] = UART1_read();
             INTCON.GIE = 1;
         }

         if(count>15) {
             LATA.F1 = 0; //turn off the rfid card reader, we are sending the last scanned card, don't want people scanning during this time

             INTCON.GIE = 0; //we don't want interrupts preempting our sending of data

             for(i=0; i<count; i++) {
                 Soft_UART_Write(cardnum[i]); //TODO: or call a custom function that handles collision avoidance
             }

             cnt = 0;

             INTCON.GIE = 1;

             ans = Soft_UART_Read(&error); //try to read once, we will most likely fail the first time (not enough time for the data to process)
             while(error) {
               if(cnt>600) { INTCON.GIE = 0; goto coms_error; } //we've spent too long waiting for a reply, give up
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

        /*code

        pins:
                B7 is green light meaning entry is allowed while lit
                B6 is red light meaning entry is not allowed due to an error or explicit rejection
                A0 is the port and asserts to lock the door, deasserts to allow access
                A1 is the reset pin for the rfid

        1. init hardware
        2. set the counter to 0
        3. setup interrupts

        4. init both serial ports
        5. enter main loop
        6. cycle until data ready on UART1, populate data structure (cardnum) one at a time
        7.        when cardnum is has a strlen of greater than 15,
        8.        send the reset pin on the rfid chip to 0
        9.        disable the interrtupt for UART1 not needed
        10.        enable the interrupt for TIMER0
        11.        initialize timer0
        12.        then send data to other serial port TODO: might need some collision detection here such as:
                                read serial port: if anything is there, wait a cycle (10ms), for a maximum of 10 cycles, and read again until there is nothing there, then wait random time and send out
                                otherwise: just blast out the data on the soft_uart
        13.        start listening for the response
        14.        if interrupt routine fires, assert a fast blink (like .25 seconds) B6 -AND- B7 to show that coms are down, do this for 2 seconds
                                set a varible showing we should stop waiting for a response and exit response loop (this will allow the user to scan their card again), proceed to step 19.
        15.        gather response
        16.        if response is '1' then:
                        assert 0 on A0 for 5 seconds (or however long the time to allow entry should be)
                        assert 1 on B7 for 5 seconds (or however long the time to allow entry should be)
        17.        if response is '0' then:
                        assert 1 on B6 for 2-3 seconds to show the user there was an error
        18.        if response is neither '0' or '1':
                        assert/deassert every .5 seconds (slow blink) B6 for 2-3 seconds to show the user there was a runtime error and not necessarily a  explicit disallow
        19. exit response loop.
        20. disable timer0 interrupt
        21. enable interrupt for UART1 again not needed
        22. reassert the reset pin to get the rfid reader back into service
        23. loop back to main loop start


        */