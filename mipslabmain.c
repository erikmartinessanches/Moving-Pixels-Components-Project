/* mipslabmain.c

   This file written 2015 by Axel Isaksson,
   modified 2015, 2017 by F Lundevall

   Update 2017-04-21 by F Lundevall
   Modified 20180228 by Erik Martines Sanches

   For copyright and licensing, see file COPYING */

#include <stdint.h>  /* Declarations of uint_32 and the like */
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include "mipslab.h" /* Declatations for these labs */
#include <stdbool.h>

void *stdout; //A workaround for using rand and srand, see FAQ. Erik Martines Sanches

int main(void)
{
  /*
	  This will set the peripheral bus clock to the same frequency
	  as the sysclock. That means 80 MHz, when the microcontroller
	  is running at 80 MHz. Changed 2017, as recommended by Axel.
	*/
  SYSKEY = 0xAA996655; /* Unlock OSCCON, step 1 */
  SYSKEY = 0x556699AA; /* Unlock OSCCON, step 2 */
  while (OSCCON & (1 << 21))
    ;                   /* Wait until PBDIV ready */
  OSCCONCLR = 0x180000; /* clear PBDIV bit <0,1> */
  while (OSCCON & (1 << 21))
    ;           /* Wait until PBDIV ready */
  SYSKEY = 0x0; /* Lock OSCCON */

  /* Set up output pins */
  AD1PCFG = 0xFFFF;
  ODCE = 0x0;
  TRISECLR = 0xFF;
  PORTE = 0x0;

  /* Output pins for display signals */
  PORTF = 0xFFFF;
  PORTG = (1 << 9);
  ODCF = 0x0;
  ODCG = 0x0;
  TRISFCLR = 0x70;
  TRISGCLR = 0x200;

  /* Set up input pins */
  TRISDSET = (1 << 8);
  TRISFSET = (1 << 1);

  /* Set up SPI as master */
  SPI2CON = 0;
  SPI2BRG = 4;
  /* SPI2STAT bit SPIROV = 0; */
  SPI2STATCLR = 0x40;
  /* SPI2CON bit CKP = 1; */
  SPI2CONSET = 0x40;
  /* SPI2CON bit MSTEN = 1; */
  SPI2CONSET = 0x20;
  /* SPI2CON bit ON = 1; */
  SPI2CONSET = 0x8000;

  /*Experimenting with scrolling the page using the OLED settings*/
  //Why doesn't this work?
  /*     spi_send_recv(0x26); //horizontal
    spi_send_recv(0x00); //dummy 
    spi_send_recv(0x00);  //start page
    spi_send_recv(0x00); //time interval 
    spi_send_recv(0x03); //end address page 3
    spi_send_recv(0x00); //dummy 
    spi_send_recv(0xFF);  //dummy
    spi_send_recv(0x2F); //activate scroll
 */

  display_init();
  //display_string(0, "KTH/ICT lab");
  //display_string(1, "in Computer");
  //display_string(2, "Engineering");
  //display_string(3, "Welcome!");
  //display_update();

  //display_image(96, icon);

  //labinit(); /* Do any lab-specific initialization */
  /* While loop modified to take into account game states by Erik Martines Sanches. */
  while (1)
  {

    if (game_status == WON)
    {
      winner();
      you_win();
      PORTECLR = 0xFF; //Turn lights off before credits.
      credits();
      game_status = CONTI;
    }
    else if (game_status == LOST)
    {
      you_lose();
      //credits();
      game_status == CONTI;
    }
    labinit();
    while (1 && (game_status == CONTI))
    {
      labwork(); /* Do lab-specific things again and again */
      if (game_status == WON || game_status == LOST)
      {
        break;
      }
    }
  }
  return 0;
}
