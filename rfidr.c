// 
// rfidr	RFID reader.
//			To read an RFID sent by the RDM630 over the RS232 to the Pi
//		
//			Requires the WiringPi library by Gordon @ Drogon
//			see: https://projects.drogon.net/raspberry-pi/wiringpi/ which has
//					full instructions on how to download and install 
//
// Tasks:
//		(1) Can I read this all in one go, e.g. read(fd, etc...) putting in the expected no. of bytes
//				No! We get the first 8 bytes then the next 6!
//				So... Add a little sleep to allow them all to accumulate?
//		Done!	It works! I get a consistent 14 char read of 12 readable chars from each tag
//
//		(2)	Once read, how do I use the checksum to extract the id characters?
//				Not sure about this. The tags have a printed number on them, but that is
//				not what I'm getting from each card. Should I?
//		Todo
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <wiringSerial.h>

// globals for timing -- move to class?
char str_time[200] = "invalid time\0";
time_t start_time = 0;
time_t seconds_after_start = 0;

// 
// Function to update the current time point in the program.
// Will get the time from the system using a time() function call
// and update the globals str_time and seconds_after_start
//
int update_time_point() {
	
	time_t t;
	struct tm *tmp;
	
	t = time(NULL);
	
	if (start_time == 0) { 
		start_time = t; 
	} else {
		seconds_after_start = t - start_time;
	}
	
	tmp = localtime(&t);
	if (tmp != NULL) {
		strftime(str_time, sizeof(str_time), "%d/%m/%y %T", tmp);
		return 0;
	} else {
		return -1;
	}
}

int main () {
	
	// setup timing
	update_time_point();
	
	// Open and rewrite an output file - TODO: unique name
	FILE *of;
	of = fopen("test.out", "w");
	fprintf(of, "Test run\n"); 
	
	// Use WiringPi's function to open the serial device
	int fd ;
	if ((fd = serialOpen ("/dev/ttyAMA0", 9600)) < 0) {
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}

	const struct timespec delay = {0, 250000}; // {tv_sec, tv_nsec}
	
	for (;;) { // Loop, getting and printing each ID presented
		
		// Data sent is 14 bytes, including a 02 as first and 03 as last (to be stripped)
		char rx_buffer[15] = "12345678901234\0"; // nevertheless, make sure null terminated!
		char rfid[12]; // buffer for the actual id and checksum
		
		// Can't seem to read it all in one go, probably 9200 baud rate is too slow.
		// So try to sleep for a bit...
		int d = serialDataAvail(fd);
		while (d < 14) {
			//printf("%d bytes available\n", d); // Don't print unless delay is about a second!
			nanosleep(&delay, NULL);
			d = serialDataAvail(fd);
		}
		
		int len = read(fd, (void*)rx_buffer, 14);
		if (len < 0) {
			printf("Error: %s\n", strerror(errno));
		} else if (len == 0) {
			printf("*\n");	// No data after block
		} else {
			
			// Got an id, so update timing
			update_time_point();
			
			// Strip leading and trailing character
			strncpy(rfid, &(rx_buffer[1]), 12);
			rfid[12] = '\0';
			
			printf("%s\tat %s\t%u\n", rfid, str_time, seconds_after_start);
			
			// re-open output file in append mode
			of = fopen("test.out", "a");
			fprintf(of, "%s\tat %s\t%u\n", rfid, str_time, seconds_after_start);
			fclose(of);
			// close output (don't know when the kill signal is coming
			
		}
	}
}
