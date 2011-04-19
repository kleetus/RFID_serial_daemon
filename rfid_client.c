void interrupt() {


}


char cardnum[16];

void main() {
	ADCON1 |= 0x0F;
	OSCCON 0x11111111; //8Mhz	
	TRISA=0x00;
	TRISB.F6 = 0;
	TRISB.F7 = 0;
	LATA=0x00;
	LATB=0x00;
	
	UART1_Init(9600);
	Software_UART_Init(PORTB, 0, 1, 9600, 0);
	
	char count, error, i, ans;
	char cardnum[16];
	
	while(1) {
		count=0;
		if(uart1_data_ready()) {
			cardnum[count++] = UART1_read();
		}
		if(count>15) {
			LATA.F1 = 0;
			//disable uart interrupt
			//enable timer0 interrupt
			//init timer0
			for(i=0; i<count; i++) {
				error = Soft_UART1_Write(cardnum[i]); //or call a custom function that handles collision avoidance
			}
			while(1) {
				ans = Soft_UART1_Read(&error);
				if(ans=='1') {
					
				}
				else if(ans=='0') {
					
				}
				else {
					
				}
			}
			
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
	7.	when cardnum is has a strlen of greater than 15, 
	8.	send the reset pin on the rfid chip to 0 
	9.	disable the interrtupt for UART1
	10.	enable the interrupt for TIMER0
	11.	initialize timer0
	12.	then send data to other serial port TODO: might need some collision detection here such as:
				read serial port: if anything is there, wait a cycle (10ms), for a maximum of 10 cycles, and read again until there is nothing there, then wait random time and send out
				otherwise: just blast out the data on the soft_uart
	13.	start listening for the response
	14.	if interrupt routine fires, assert a fast blink (like .25 seconds) B6 -AND- B7 to show that coms are down, do this for 2 seconds
				set a varible showing we should stop waiting for a response and exit response loop (this will allow the user to scan their card again), proceed to step 19.
	15.	gather response
	16.	if response is '1' then:
			assert 0 on A0 for 5 seconds (or however long the time to allow entry should be)
			assert 1 on B7 for 5 seconds (or however long the time to allow entry should be)
	17.	if response is '0' then:
			assert 1 on B6 for 2-3 seconds to show the user there was an error
	18.	if response is neither '0' or '1':
			assert/deassert every .5 seconds (slow blink) B6 for 2-3 seconds to show the user there was a runtime error and not necessarily a  explicit disallow
	19. exit response loop.
	20. disable timer0 interrupt
	21. enable interrupt for UART1 again
	22. reassert the reset pin to get the rfid reader back into service
	23. loop back to main loop start
		
	
	*/


}