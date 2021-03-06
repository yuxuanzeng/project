/* 
 * Modified by Yuxuan Zeng for ECE 497 project
 * 
 * 1. read buttons and control dot on matrix
 */

#include "Resources/BoneHeader/BoneHeader.h"
#include "Resources/adafruit/HT1632/HT1632.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "icons.c"
/* poll for buttons */
#include <poll.h>
#include <string.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <time.h>  
using namespace std;

#define BANK 1
#define DATA 28	//1_28  // 17
#define WR 17   //1_17	// 0
#define CS 16   //1_16	// 16

/* 
 * buttons 
 */
/****************************************************************
* button  PIN 	41      42
*         GPIO	20      7
***************************************************************/
#define GPIOB1  20
#define GPIOB2  7
#define MAX_BUF 64
int keepgoing = 1;
int dotx = 11;
int doty = 7;
bool toggle = false;
int gspeed[] = {1000000, 700000, 50000};
/* gLines[i] = flag, x0, y0, x1, y1
               flag = 0, no line
               flag = 1, line moving from left to right
               flag = 2, line moving from right to left */
int gLines[] = {0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,};
int gTemp = 0;
int gCredit = 0;
int gFlagspeed = 7;
int patternLength = 1;
int speed = 4;
int interval = 6;

/****************************************************************
 * signal_handler
 ****************************************************************/
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}
// end of modify

void drawDot(HT1632LEDMatrix *matrix, int erase) {
  //matrix->clearScreen();
  //matrix->setBrightness(16);
  //matrix->setTextSize(1);
  //matrix->setTextColor(1);
  if (0 != erase)
  {
     erase = 1;
  }

  matrix->drawPixel(dotx,  doty,  erase);
  matrix->drawPixel(dotx+1,doty,  erase);
  matrix->drawPixel(dotx,  doty+1,erase);
  matrix->drawPixel(dotx+1,doty+1,erase);
  matrix->writeScreen();
  //usleep(2000000);
}

void clearLines() {
	int i;
	for(i = 0; i < 80; i++)
		gLines[i] = 0;
}

// pattern 1, generate one bar of length n from left
void generatePattern1() {
	int i, j;
	for(i = 0; i < 16; i++) {
		j = 5*i;
		if(gLines[j] == 0 && gTemp % (2 * interval) == 0) {
			gLines[j] = 1;
			gLines[j + 1] = 0;
			gLines[j + 2] = 0;
			gLines[j + 3] = 0;
			gLines[j + 4] = rand() % 12 + 2;
			break;
		}
		if(gLines[j] == 0 && gTemp % (2 * interval) == interval) {
			gLines[j] = 2;
			gLines[j + 1] = 23;
			while((gLines[j + 2] = rand() % 12 + 2) > (gLines[j - 1] + 2));
			gLines[j + 3] = 23;
			gLines[j + 4] = 15;
			break;		
		}
	}
	if(rand() % 3 == 0 && gTemp % (2 * interval) == 2 * interval - 1) {
		toggle = true;
		cout << "Toggled!!";
		gTemp = -1;
		patternLength = rand() % 13 + 1;
	}
}

//pattern 2, generate 2 lines from left with interval 7 of length 8, generate 2 lines from right of length 8 with interval 7 after 5 steps
void generatePattern2() {
	int i, l;
	int interval;
	if(speed >= 2)
		interval = 6;
	else
		interval = 7;
	for(i = 0; i < 16; i++) {
		l = 5 * i;
		if(gLines[l] == 0 && gTemp % interval == 0) {
			gLines[l] = 1;
			gLines[l + 1] = 0;
			gLines[l + 2] = 0;
			gLines[l + 3] = 0;
			gLines[l + 4] = patternLength;
			break;
		} 
		if(gLines[l] == 0 && gTemp % interval == 3) {
			gLines[l] = 2;
			gLines[l + 1] = 23;
			gLines[l + 2] = patternLength + 1;
			gLines[l + 3] = 23;
			gLines[l + 4] = 15;		
			break;
		}
	}
	if(gTemp == interval * 3 - 1) {
		toggle = false;
		gTemp = 0;
	}	
}

void newLines() {
	srand((unsigned)time(NULL));
	if(toggle == false) {
		generatePattern1();
	}
	else
		generatePattern2();
	gTemp++;
}

void refreshLines() {
	int i, l;
	for (i = 0; i < 16; i++) {
		l = 5*i;
		if (1 == gLines[l]) { 
			// renew the value of x0 and x1 of every existing lines 
			gLines[l+1]++;
			gLines[l+3]++;
			if(gLines[l+1] > 23) {
				//the left line is out of boundary
				gLines[l] = 0;  //clear the line
        		} 
     		} else if(2 == gLines[l]) {
			gLines[l+1]--;
			gLines[l+3]--;
			if(gLines[l+1] < 0) {
				gLines[l] = 0;
			}
		}
  	}
}

void drawLines(HT1632LEDMatrix *matrix, int erase){
  	int i = 0;
  	int l;

  	if (0 != erase) {
     		erase = 1;
  	}

  	for (i = 0; i < 16; i++) {
     		l = 5*i;
     		if (0 != gLines[l]) {
        		matrix->drawLine(gLines[l+1], gLines[l+2], gLines[l+3],gLines[l+4], erase); 
     		}
  	}
  	matrix->writeScreen();
}

// set the value of gFlagspeed according to gCredit
void set_speed(void){
  	if (gCredit < 40) {
    		gFlagspeed = 7;
  	} else if (gCredit < 70) {
    		gFlagspeed = 5;
  	} else {
	    	gFlagspeed = 1;
  	}
}

void flashSpeed(HT1632LEDMatrix *matrix, int sp){
  	char message[] = "Lev0";
  	int j = 0;
  	int i = 0;

  	message[3] += sp;
  
  	matrix->setBrightness(16);
  	matrix->setTextSize(1);
  	matrix->setTextColor(1);
  	matrix->clearScreen();
  	matrix->setCursor(0,4);
  	for(i=0;message[i]; i++) {
      		matrix->write(message[i]);
      	}
  	matrix->writeScreen();
  	// Blink!
  	for(j=0;j<5;j++) {
  		matrix->blink(true);
  		usleep(200000);
  		matrix->blink(false);
  	}
  	//clear matrix
  	matrix->clearScreen();
}

void showWelcome(HT1632LEDMatrix *matrix) {
  	//Display "BeagleBone"
  	char message[] = "Run!";
	int i;
  	matrix->setBrightness(16);
  	matrix->setTextSize(1);
  	matrix->setTextColor(1);
  	
	matrix->setCursor(9, 5);
        matrix->write(51);
	matrix->writeScreen();
	matrix->clearScreen();
	usleep(1000000);
	matrix->setCursor(9, 5);
	matrix->write('2');
	matrix->writeScreen();
	matrix->clearScreen();
	usleep(1000000);
	matrix->setCursor(9, 5);
	matrix->write('1');
	matrix->writeScreen();
	matrix->clearScreen();
	usleep(1000000);

	matrix->setCursor(1, 5);
	for(i=0;message[i]; i++) {
      			matrix->write(message[i]);
      		}
	matrix->writeScreen();
	usleep(100000);

  
  	// Blink!
  	matrix->blink(true);
  	usleep(100000);
  	matrix->blink(false);
  	//clear matrix
  	matrix->clearScreen();

  	// Dim down and then up
	/*  for (int8_t i = 15; i >= 0; i--) {
    		matrix->setBrightness(i);
    		usleep(100000);
  	}
  	for (int8_t i = 0; i < 16; i++) {
    		matrix->setBrightness(i);
    		usleep(100000);
  	}

  	// Blink again!
  	matrix->blink(true);
  	usleep(2000000);
  	matrix->blink(false);
  
  	// Dim again
  	for (int8_t i = 15; i >= 0; i--) {
    		matrix->setBrightness(i);
    		usleep(100000);
  	}*/
}

void showScore(HT1632LEDMatrix *matrix) {
  int credit = 0;
  int i;

  if (10000 <= gCredit)
  {
     credit = 9999;
  }
  else
  {
     credit = gCredit;
  }

  matrix->clearScreen();
  matrix->setCursor(0, 5); 
  i = credit/1000;
  credit = credit%1000;
  matrix->write(i+48);

  i = credit/100;
  credit = credit%100;
  matrix->write(i+48);

  i = credit/10;
  credit = credit%10;
  matrix->write(i+48);

  i = credit;
  matrix->write(i+48);
	
  matrix->writeScreen();

}

// check whether there is collision of dot and bars
bool isCollision() {
	int i;
	int l;
	for(i = 0; i < 16; i++) {
		l = 5 * i;
		if(0 != gLines[l]) {
			if(11 == gLines[l+1] || 12 == gLines[l+1] ) { // bar's in the middle
				if(doty > gLines[l+4] || (doty + 1) < gLines[l+2]) {
					continue;
				} else {
					return true;
				}
			}
		}
	}
	return false;
}

void updateLevel() {
	// speed 4
	if(gCredit > 150 && gCredit <= 200) {
		speed = 3;
		interval = 7;
	} else if(gCredit > 200 && gCredit <= 300) {
		speed = 2;
		interval = 9;
	} else if(gCredit > 300) {
		speed = 1;
		interval = 16;
	}
}

int main(void) {
  	printf("Starting...\n");
  
	HT1632LEDMatrix matrix = HT1632LEDMatrix(BANK, DATA, WR, CS);
  	matrix.begin(HT1632_COMMON_16NMOS);
  
  	/* define vars needed by gpio reading */
  	struct pollfd fdset[3];
	int nfds = 3;
	int timeout, rc;
	int gpio1_fd,gpio2_fd;
	char buf[127];
	int len;

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	/* export and configure button gpio */
	export_gpio(GPIOB1);
	set_gpio_direction(GPIOB1, "in");
	set_gpio_edge(GPIOB1, "falling");
	gpio1_fd = gpio_fd_open(GPIOB1);

	export_gpio(GPIOB2);
	set_gpio_direction(GPIOB2, "in");
	set_gpio_edge(GPIOB2, "falling");
	gpio2_fd = gpio_fd_open(GPIOB2);

	timeout = 10;

  	printf("Clear\n");
  	matrix.clearScreen();
  
  	printf("welcome !\n");
  	showWelcome(&matrix);

  	printf("Done!\n");

	int temp = 0;
  	while (keepgoing) {
		// clear screen
		matrix.clearScreen();
		// draw bars
		drawLines(&matrix, 1);
		
		// draw dot
		drawDot(&matrix, 1);
		
		// listen to button
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
      
		fdset[1].fd = gpio1_fd;
		fdset[1].events = POLLPRI;

		fdset[2].fd = gpio2_fd;
		fdset[2].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
      
		if (rc == 0) {
			printf(".");
		}
	
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("button 1 pressed\r\n");
			//dot move down
			doty += 1;
			if (14 < doty) {
				doty = 14;
			}
		}

		if (fdset[2].revents & POLLPRI) {
			lseek(fdset[2].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[2].fd, buf, MAX_BUF);
			printf("button 2 pressed\r\n");
			//dot move up
			doty -= 1;
			if (0 > doty) {
				doty = 0;
			}
		}

		if (fdset[0].revents & POLLIN) {
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}
		
		// check collision
		if(isCollision()) { // bars are in dot area, collision
			cout << "\nCollision Detected! Dotx:" << dotx << "Doty:" << doty << "\n";  
			cout << "Credit:" << gCredit << "\n";
			break;
		} else { // no collision
			// calculate score
			gCredit++;
			updateLevel();
			temp++;
			if(temp % speed == 0) {
				newLines();
				refreshLines();
			}
		}
	}

	// show score
	showScore(&matrix);

        gpio_fd_close(gpio1_fd);
	gpio_fd_close(gpio2_fd);
	unexport_gpio(GPIOB1);
	unexport_gpio(GPIOB2);

	fflush(stdout);

  	return 0;
}
