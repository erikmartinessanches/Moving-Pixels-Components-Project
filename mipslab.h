/* mipslab.h
   Header file for all labs.
   This file written 2015 by F Lundevall
   Some parts are original code written by Axel Isaksson
    
   Latest update 2015-08-28 by F Lundevall
  Some parts added 20180228 by Erik Martines Sanches, see comments.

   For copyright and licensing, see file COPYING */
#include <stdbool.h>
//#include <stdio.h>

/* Modifications by Erik Martines Sanches start here*/
//Tweaking these defines adjusts the game:
#define TICK_TIME 460 // Used for decrementing a counter. In units of 1/50 seconds.
#define NUMBER_OF_ITEMS 90 //How many items the items array will hold.
#define ITEMS_FOR_SIZE_1 40//45 //Collect these many items since birth to go to size 1. (Size 0-2)
#define ITEMS_FOR_SIZE_2 55//70//Collect these many items since birth to go to size 2.
#define ITEMS_FOR_SPEED_1 8 //Collect these many items since birth to go to speed 1. (Speed 0-2)
#define ITEMS_FOR_SPEED_2 25 //Collect these many items since birth to go to speed 2.
#define INITIAL_SPEED 0.15
#define SPEED_INCREASE 0.15
#define BALL_DECELERATION 0.08
#define AVOIDANCE_DISTANCE 16 // How far away from the player balls avoid the player.
#define ITEMS_FOR_AVOIDANCE 55 //70 //How many balls the player picks up before balls start avoiding.
#define GROW_BY_PIXELS 2 //Pixels to grow by.

extern int hidden_items;
typedef enum {WON, LOST, CONTI} status;
extern status game_status;
void you_lose(void);
void you_win(void);
void winner(void);
void credits(void);

/* End of changes by Erik martines Sanches */

/* Declare display-related functions from mipslabfunc.c */
void display_image(int x, const uint8_t *data);
void display_init(void);
void display_string(int line, char *s);
void display_update(void);
uint8_t spi_send_recv(uint8_t data);

/* Declare lab-related functions from mipslabfunc.c */
char * itoaconv( int num );
void labwork(void);
int nextprime( int inval );
void quicksleep(int cyc);
void tick( unsigned int * timep );

//extern bool continue_game;
/* Declare display_debug - a function to help debugging.

   After calling display_debug,
   the two middle lines of the display show
   an address and its current contents.

   There's one parameter: the address to read and display.

   Note: When you use this function, you should comment out any
   repeated calls to display_image; display_image overwrites
   about half of the digits shown by display_debug.
*/
void display_debug( volatile int * const addr );

/* Declare bitmap array containing font */
extern const uint8_t const font[128*8];
/* Declare bitmap array containing icon */
//extern const uint8_t const icon[128];
extern const uint8_t const icon[4096];//Modified by Erik Martines Sanches for project
extern const uint8_t const winner_text[4096];//Modified by Erik Martines Sanches for project
/* Declare text buffer for display output */
extern char textbuffer[4][16];

//typedef uint8_t column; //I'm define a nonstatic unsigned type, 8 bits wide, for holding the column.
//typedef int8_t foo; //A small data type capable of negatives is helpful for drawing lines.
//uint8_t frame;

/* Declare functions written by students.
   Note: Since we declare these functions here,
   students must define their functions with the exact types
   specified in the laboratory instructions. */
/* Written as part of asm lab: delay, time2string */
void delay(int);
void time2string( char *, int );
/* Written as part of i/o lab: getbtns, getsw, enable_interrupt */
int getbtns(void);
int getsw(void);
void enable_interrupt(void);
