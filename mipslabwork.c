/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall

   This file should be changed by YOU! So you must
   add comment(s) here with your name(s) and date(s):

   This file modified 20180303 by Erik Martines Sanches

   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */
//Code by Erik Martines Sanches follows unless otherwise noted.
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

uint8_t frame[512]; //The current frame. Each byte represents an 8 pixel tall column.
bool prepare_frame_ok = false; //Don't prepare a frame until this is nonzero (set by the ISR).
unsigned int delay_for_random_seed = 0; //Time before a user presses a button on the start screen.
unsigned int timeoutcount = 0; //Helps establish the above variable.
int hidden_items = 0; //Items that are collected are hidden.
status game_status = CONTI;
int time_units_left = 8; //How many time units are left of the game.
enum direction {LEFT, RIGHT, UP, DOWN, NONE};
enum direction currentDirection = NONE; //Direction of player. Should perhaps be inside a struct.
enum direction oldDirection = NONE; //For keeping track of when to change direction.

typedef struct Position2D{
  float x;
  float y;
} position2D;

typedef struct Node{ //Represents an item.
  bool movable; //Whether we should be able to move it.
  bool drawable; //Whether to draw this item or not.
  int items_collected; //How many balls it has collected.
  int size; //Three sizes. [0-2]
  float offset_per_time_unit; //Three speed increases possible
  position2D vertices[4]; //Geometry of this node (currently 4 vertices connected by a edges).
} node;

node items[NUMBER_OF_ITEMS]; //Holds all the items, including the player.

/*Resets Timer 2 interrupt flag. Author: Erik Martines Sanches */
void resetT2IF(void) { IFSCLR(0) = 0x00000100; } //clears IFS(0) T2IF.
/*Checks if Timer 2 interrupt flag has been raised. Author: Erik Martines Sanches */
bool T2IF_raised(void){ return (((IFS(0) >> 8) & 0x1) == 1 ) ? 1 : 0; }

//These two lines are form the lab source ocde or the Toolchain FAQ:
void enable_interrupt(void) { asm volatile("ei"); }
void *stdout; //A workaround for enabling the use of srand() and rand().
//int mytime = 0x5957;
//char textstring[] = "text, more text, and even more text!";

/*Returns absolute value of x. Author: Erik Martines Sanches */
int abs1(int x){if (x < 0) { return -x; } else { return x;}}

/*Sets a bit in the frame array while preserving existing bits.
  Author: Erik Martines Sanches */
void draw_pixel(int x, int y){
  if(((unsigned)y < 32) && ((unsigned)x<128)){
    if (y <= 7){ frame[x] |= (0x01 << (y));} 
    else if (y >= 8 && y <= 15) {frame[128 + x] |= (0x01 << (y % 8));} 
    else if (y >= 16 && y <= 23) {frame[256 + x] |= (0x01 << (y % 16));}
    else if (y >= 24) {frame[384 + x] |= (0x01 << (y % 24));}
  }
}
/*Helper function for plotLine().
  Acquired from https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm */
void plotLineLow(int x0, int y0, int x1, int y1){
  int dx = x1 - x0;
  int dy = y1 - y0;
  int yi = 1;
  if(dy < 0){
    yi = -1;
    dy = -dy;
  }
  int D = 2 * dy - dx;
  int y = y0;
  
  int x;
  for(x = x0; x <= x1; x++){
    draw_pixel(x, y);
    if(D > 0){
      y = y + yi;
      D = D - 2 * dx;
    }
    D = D + 2 * dy;
  }
}
/*Helper function for plotLine().
  Acquired from https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm */
void plotLineHigh(int x0, int y0, int x1, int y1){
  int dx = x1 - x0;
  int dy = y1 - y0;
  int xi = 1;
  if(dx < 0){
    xi = -1;
    dx = -dx;
  }
  int D = 2 * dx - dy;
  int x = x0;
  
  int y;
  for(y = y0; y <= y1; y++){
    draw_pixel(x, y);
    if(D > 0){
      x = x + xi;
      D = D - 2 * dy;
    }
    D = D + 2 * dx;
  }
}

/*Plots a line between the provided points using Bresenham's line algorithm. 
  Acquired from https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm */
void plotLine(int x0, int y0, int x1, int y1){
  if (abs1(y1 - y0) < abs1(x1 - x0)){
    if(x0 > x1){
      plotLineLow(x1, y1, x0, y0);
    } else {
      plotLineLow(x0, y0, x1, y1);
    }
  } else {
    if(y0 > y1){
      plotLineHigh(x1, y1, x0, y0);
    } else {
      plotLineHigh(x0, y0, x1, y1);
    }
  }
}

/*Empties frame array, offsets according to input, puts edges into the frame array
  Author: Erik Martines Sanches. */
void prepare_frame(void){
  //Empty frame array to start on a new frame.
  uint8_t *j;
  for (j = frame; j < &frame[512]; ++j) { *j = 0x00; }

  //First determine what tempDirection should be set to
  //depending on if oldDirection and currentDirection differ. 
  enum direction tempDirection;
  //When we switch to a different direction.
  if((currentDirection != oldDirection) && (currentDirection != NONE )){
    tempDirection = currentDirection;
    oldDirection = tempDirection;
  } 
  //Keep going in the same old direction, even if a button is pressed in that direction.
  else { tempDirection = oldDirection; } 

  //Traverse the items array and put their shapes into the frame array.
  node *p;
  for(p = items; p < &items[NUMBER_OF_ITEMS]; ++p){ //Iterate over movable items.
    if (p->movable) { 
      //Now offset the vertices positions accordingly.
      int v;
      for (v = 0; v < 4; v++){ //iterate over 4 vertices.
        if (tempDirection == RIGHT) { p->vertices[v].x += p->offset_per_time_unit;}
        if (tempDirection == DOWN) {p->vertices[v].y += p->offset_per_time_unit;}
        if (tempDirection == UP) {p->vertices[v].y -= p->offset_per_time_unit;}
        if (tempDirection == LEFT) {p->vertices[v].x -= p->offset_per_time_unit;}
      }
    }
    //Now put the edges into the frame array.
    if (p->drawable){
      int i;
      for (i = 0; i < 3; i++){ //iterate the first 3 vertices and connect the dots
        plotLine((int)p->vertices[i].x, (int)p->vertices[i].y, (int)p->vertices[i+1].x, (int)p->vertices[i+1].y);
      }
      //connect last point to first.
      plotLine((int)p->vertices[i].x, (int)p->vertices[i].y, (int)p->vertices[0].x, (int)p->vertices[0].y);
    }
  }
}

/* Buttons' status. Author: Erik Martines Sanches */
int getbtns(void){
  uint8_t portDButtons = 0; //BTN 2-4 on bits RD5 - RD7
  uint8_t portFButtons = 0; //BTN 1 on RF1
  portDButtons = (PORTD >> 4) & 0xE; //This should move and mask the proper bits from PORTD.
  portFButtons = (PORTF >> 1) & 0x1; //This should move and mask the proper bits from PORTF.
  uint8_t buttonsStatus = portDButtons | portFButtons; //Combine the input from port D buttons and port F button.
  return buttonsStatus;
}

/* Checks whether player covers an item and whether a player is within the screen. Changes player's 
 offset_per_time_unit and size. Author: Erik Martines Sanches */
void check_bounds_and_collisions(void){
  node *p;
  for(p = items; p < &items[NUMBER_OF_ITEMS]; ++p){ //Iterate across movable and drawable items (player).
    if (p->movable && p->drawable) {
      //Figure out largest and smallest x and y values so that we can use it for a bounding box of the movable item.
      float smallest_player_x = p->vertices[0].x - 1; //To allow all of the player's bounding box to cover the ball, due to floating point.
      float largest_player_x = p->vertices[0].x;
      float smallest_player_y = p->vertices[0].y - 1; //To allow all of the player's bounding box to cover the ball, due to floating point.
      float largest_player_y = p->vertices[0].y;

      bool allVertsOutside = false; //Whether all vertices are outside the screen.
      int count = 0; //For counting the vertices outside the screen.
      position2D *v;
      for (v = p->vertices; v < &p->vertices[4]; ++v){ //Iterate over four vertices to set the proper "rectangular bounding box".
        if (v->x > largest_player_x){
          largest_player_x = v->x;
        }
        if (v->x < smallest_player_x){
          smallest_player_x = v->x;
        }
        if (v->y > largest_player_y){
          largest_player_y = v->y;
        }
        if (v->y < smallest_player_y){
          smallest_player_y = v->y;
        }
        //Checks when movable item is totally outside the screen.
        if (v->x < 0 || v->y < 0 || v->x > 127 || v->y > 31){
          count++; //A vertex is outside the screen.
        }
        if (count == 4) { allVertsOutside = true; }
      }
      if (allVertsOutside){ //Move the player to the middle, reset abilities.
        *p = (node){true, true, 0, 0, INITIAL_SPEED, {{59,16}, {64,16}, {64,21}, {59,21}}};
        currentDirection = NONE;
      }
      //Now check whether player encompasses any of the non-movable, drawable items (balls).
      node *q;
      for(q = items; q < &items[NUMBER_OF_ITEMS]; ++q){ //For every non-movable item.
        if (q->movable == false && q->drawable == true ){//Of course this needs to check drawable I
        //certainly don't want to do coverage detection with non-drawable, hidden non-movable items!
          
          float smallest_ball_x = q->vertices[0].x; //Establish the ball's smallest and largest x and y.
          float largest_ball_x = q->vertices[0].x;
          float smallest_ball_y = q->vertices[0].y;
          float largest_ball_y = q->vertices[0].y;

          position2D *w;
          for (w = q->vertices; w < &q->vertices[4]; ++w){ //Iterate over 4 vertiecs.
            if (w->x > largest_ball_x) {largest_ball_x = w->x;}
            if (w->x < smallest_ball_x) {smallest_ball_x = w->x;}
            if (w->y > largest_ball_y) {largest_ball_y = w->y;}
            if (w->y < smallest_ball_y) {smallest_ball_y = w->y;}
          }

          //Now check if ball is contained within player's bounding box.
          if (smallest_ball_x >= smallest_player_x && largest_ball_x <= largest_player_x && 
              smallest_ball_y >= smallest_player_y && largest_ball_y <= largest_player_y){
            q->drawable = false; //The inner object (ball) hides.
            p->items_collected++;//The outer object (player) collected one inner (ball).
            hidden_items++;
            //Items_collected_since_birth++;
            if (p->items_collected == ITEMS_FOR_SPEED_1 || p->items_collected == ITEMS_FOR_SPEED_2){ 
              //p->offset_per_time_unit++; 
              p->offset_per_time_unit += SPEED_INCREASE; 
            }
            if (p->items_collected == ITEMS_FOR_SIZE_1 || p->items_collected == ITEMS_FOR_SIZE_2){
              //This grows the square player in a cool way.
              p->vertices[0].x -= GROW_BY_PIXELS;
              p->vertices[0].y -= GROW_BY_PIXELS;
              p->vertices[1].x += GROW_BY_PIXELS;
              p->vertices[1].y -= GROW_BY_PIXELS;
              p->vertices[2].y += GROW_BY_PIXELS;
              p->vertices[2].x += GROW_BY_PIXELS;
              p->vertices[3].x -= GROW_BY_PIXELS;
              p->vertices[3].y += GROW_BY_PIXELS;
              p->size++;
            }
          }
        }
      }
    }
  }
}

/*A float version of absolute value function. Author: Erik Martines Sanches */
float floatabs(float x){if (x < 0) { return -x; } else { return x;}}

/*This allows balls to avoid the player. Author: Erik Martines Sanches */
void avoid_player(void){
  //First get first player's speed.
  float players_offset_per_time_unit;
  //Figure out player's largest and smallest x and y values so that we can use it for a bounding box.
  float smallest_player_x; //To allow all of the player's bounding box to cover the ball, due to floating point.
  float largest_player_x;
  float smallest_player_y; //To allow all of the player's bounding box to cover the ball, due to floating point.
  float largest_player_y;
  node *r;
  for(r = items; r < &items[NUMBER_OF_ITEMS]; ++r){
    if(r->movable){ 
      players_offset_per_time_unit = r->offset_per_time_unit;
      //Figure out largest and smallest x and y values so that we can use it for a bounding box of the movable item.
      smallest_player_x = r->vertices[0].x; //To allow all of the player's bounding box to cover the ball, due to floating point.
      largest_player_x = r->vertices[0].x;
      smallest_player_y = r->vertices[0].y; //To allow all of the player's bounding box to cover the ball, due to floating point.
      largest_player_y = r->vertices[0].y;
      position2D *o;
      for (o = r->vertices; o < &r->vertices[4]; ++o){ //Iterate over four vertices to set the proper "rectangular bounding box".
        if (o->x > largest_player_x){
          largest_player_x = o->x;
        }
        if (o->x < smallest_player_x){
          smallest_player_x = o->x;
        }
        if (o->y > largest_player_y){
          largest_player_y = o->y;
        }
        if (o->y < smallest_player_y){
          smallest_player_y = o->y;
        }
      }
    } 
  }
  //Now we know the "middle" of the bounding box.
  position2D middle_of_player = {smallest_player_x + ((largest_player_x - smallest_player_x)/2), smallest_player_y + ((largest_player_y - smallest_player_y)/2)};
  node *b; //Now iterate over all balls and offset them accordingly.
  for(b = items; b < &items[NUMBER_OF_ITEMS]; ++b){
    if((b->movable == false) && b->drawable){ //Balls are found as non-movable, ironically. Clean that up later.
      //Get balls first vertex position in order to compare its location to the location of the player.
      position2D balls_first_vertex = b->vertices[0];
      //Only avoid when close.
      if((floatabs(middle_of_player.x - balls_first_vertex.x) < AVOIDANCE_DISTANCE) && (floatabs(middle_of_player.y - balls_first_vertex.y) < AVOIDANCE_DISTANCE)){   
        if (currentDirection == UP){
          if (balls_first_vertex.y > 3 && middle_of_player.y > balls_first_vertex.y){ //Player is lower than ball, ball avoids up.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (balls_first_vertex.y > middle_of_player.y && balls_first_vertex.y < 29){ //Player is higher than ball and ball avoids down.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (middle_of_player.x >= balls_first_vertex.x && balls_first_vertex.x > 3){ //Player to the right of the ball, ball avoids to the left.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (middle_of_player.x < balls_first_vertex.x && balls_first_vertex.x < 125){ //Player is to the left of the ball, ball avoids right.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }   
        } 
        else if (currentDirection == DOWN){
          if (balls_first_vertex.y > 3 && middle_of_player.y > balls_first_vertex.y){ //Player is lower than ball, ball avoids up.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (balls_first_vertex.y > middle_of_player.y && balls_first_vertex.y < 29){ //Player is higher than ball and ball avoids down.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (middle_of_player.x >= balls_first_vertex.x && balls_first_vertex.x > 3){ //Player to the right of the ball, ball avoids to the left.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (middle_of_player.x < balls_first_vertex.x && balls_first_vertex.x < 125){ //Player is to the left of the ball, ball avoids right.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }   
        } /*The above UP and DOWN cases work well! Now do LEFT nad RIGHT.*/
        else if (currentDirection == LEFT){
          if (middle_of_player.x >= balls_first_vertex.x && balls_first_vertex.x > 3){ //Player to the right of the ball, ball avoids to the left.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          if (middle_of_player.x < balls_first_vertex.x && balls_first_vertex.x < 125){ //Player is to the left of the ball, ball avoids right.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (middle_of_player.y > balls_first_vertex.y && balls_first_vertex.y > 3 ){ //Player is lower than ball, ball avoids up.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          else if (balls_first_vertex.y > middle_of_player.y && balls_first_vertex.y < 29){ //Player is higher than ball and ball avoids down.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }    
        }
        else if (currentDirection == RIGHT){
          if (middle_of_player.x >= balls_first_vertex.x && balls_first_vertex.x > 3){ //Player to the right of the ball, ball avoids to the left.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
         if (middle_of_player.x < balls_first_vertex.x && balls_first_vertex.x < 125){ //Player is to the left of the ball, ball avoids right.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].x += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          if (middle_of_player.y > balls_first_vertex.y && balls_first_vertex.y > 3 ){ //Player is lower than ball, ball avoids up.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y -= players_offset_per_time_unit - BALL_DECELERATION;
            }
          }
          if (balls_first_vertex.y > middle_of_player.y && balls_first_vertex.y < 29){ //Player is higher than ball and ball avoids down.
            int v;
            for (v = 0; v < 4; v++){ //iterate over 4 vertices.
              b->vertices[v].y += players_offset_per_time_unit - BALL_DECELERATION;
            }
          }  
        }
      }
    }
  }
}

/* Polling buttons for input. Author: Erik Martines Sanches */
void process_input(void){ //This gets called on every frame for now.
  switch (getbtns()){
    case 0x0: currentDirection = NONE; break;
    case 0x1: currentDirection = UP; break; //BTN1 right
    case 0x2: currentDirection = DOWN; break; //BTN2 up
    case 0x4: currentDirection = RIGHT; break; //BTN3 down
    case 0x8: currentDirection = LEFT; break;//BTN4 left
  }
}

#define DISPLAY_CHANGE_TO_COMMAND_MODE_ERIK (PORTFCLR = 0x10)
#define DISPLAY_CHANGE_TO_DATA_MODE_ERIK (PORTFSET = 0x10)
/*Credits shown when game is won. Author: Erik Martines Sanches*/
void credits(void) {
  display_string(0, "Programming");
  display_string(1, "");
  display_string(2, "Erik Martines");
  display_string(3, "Sanches");

  DISPLAY_CHANGE_TO_COMMAND_MODE_ERIK;
  spi_send_recv(0x2E); //deactivate scroll

  display_update();
  
  DISPLAY_CHANGE_TO_COMMAND_MODE_ERIK;
  spi_send_recv(0x2A); //vertical and right horiz scroll
  spi_send_recv(0x00); //dummy
  spi_send_recv(0x00); //start page address
  spi_send_recv(0x00); //5 frames between each scroll step
  spi_send_recv(0x03); //end page address
  spi_send_recv(0x00); //vertical scrolling offset in rows...
  spi_send_recv(0x2F); //activate scroll

  //Update display and wait for user input.
  int counter_old = timeoutcount;
  // bool changeMessage = true;
  currentDirection = NONE;
  quicksleep(1599999);
	while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30) ){
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
  DISPLAY_CHANGE_TO_COMMAND_MODE_ERIK;
  spi_send_recv(0x2E); //deactivate scroll
}

void winner(void){
  timeoutcount = 0;
  display_image(0, winner_text);
  int counter_old = timeoutcount;
  while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30)){
      delay_for_random_seed = timeoutcount;
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
}

/* Winning function. Author: Erik Martines Sanches */
void you_win(void) {
  display_string(0, "Impressive. You");
  display_string(1, "picked up all");
  display_string(2, "the balls from");
  display_string(3, "the court.");
  display_update();
  //Update display and wait for user input.
  int counter_old = timeoutcount;
  currentDirection = NONE;
	while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30) ){
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
}

/* Losing function. Author: Erik Martines Sanches */
void you_lose(void) {
  if (NUMBER_OF_ITEMS - hidden_items < 3) {
    display_string(0, "You lost with");
    display_string(1, "one ball left!");
    display_string(2, "Concentrate.");
    display_string(3, "You've got this.");
  }
  else if (NUMBER_OF_ITEMS - hidden_items < 4) {
    display_string(0, "You lost.");
    display_string(1, "Just two balls");
    display_string(2, "left!");
    display_string(3, "Don't despair.");
  } else if (NUMBER_OF_ITEMS - hidden_items < 5) {
    display_string(0, "You lost.");
    display_string(1, "Three balls");
    display_string(2, "left. Summon");
    display_string(3, "your powers.");
  } else if (NUMBER_OF_ITEMS - hidden_items < 10) {
    display_string(0, "You lost.");
    display_string(1, "");
    display_string(2, "Don't get");
    display_string(3, "discouraged!");
  } else if (NUMBER_OF_ITEMS - hidden_items < 15) {
    display_string(0, "You lost.");
    display_string(1, "But I admit,");
    display_string(2, "this time it");
    display_string(3, "was close.");
  } else if (NUMBER_OF_ITEMS - hidden_items < 20) {
    display_string(0, "You lost.");
    display_string(1, "Train harder");
    display_string(2, "than anyone");
    display_string(3, "else!");
  } else if (NUMBER_OF_ITEMS - hidden_items < 25) {
    display_string(0, "Not quite.");
    display_string(1, "The will is");
    display_string(2, "greater than");
    display_string(3, "the means.");
  } else if (NUMBER_OF_ITEMS - hidden_items < 30) {
    display_string(0, "You lost but");
    display_string(1, "don't give up.");
    display_string(2, "There is a way.");
    display_string(3, "");
  } else if (NUMBER_OF_ITEMS - hidden_items < 40) {
    display_string(0, "You lost.");
    display_string(1, "What will the");
    display_string(2, "club owner say?");
    display_string(3, "");
  } else {
    display_string(0, "You lost.");
    display_string(1, "A lot of balls");
    display_string(2, "were left on the");
    display_string(3, "court.");
  }
  //Update display and wait for user input.
  display_update();
  int counter_old = timeoutcount;
  currentDirection = NONE;
	while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30) ){
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
}

/* Updates game status depending on state. Author: Erik Martines Sanches */
void set_game_status(void){
  if (timeoutcount % TICK_TIME == 0){ time_units_left--; }
  if(hidden_items == NUMBER_OF_ITEMS - 1){ 
    game_status = WON;
    currentDirection = NONE;
    } else if(time_units_left == 0){
      game_status = LOST;
      currentDirection = NONE;
      PORTECLR = 0xFF;
    } else {
      game_status = CONTI;
    }
}

/* Lights the LEDs to show time units left before game over. Author: Erik Martines Sanches */
void update_leds(void){
  switch(time_units_left){
    case 8: PORTECLR = 0xFF; PORTESET = 0xFF; break;
    case 7: PORTECLR = 0xFF; PORTESET = 0x7F; break;
    case 6: PORTECLR = 0xFF; PORTESET = 0x3F; break;
    case 5: PORTECLR = 0xFF; PORTESET = 0x1F; break;
    case 4: PORTECLR = 0xFF; PORTESET = 0x0F; break;
    case 3: PORTECLR = 0xFF; PORTESET = 0x07; break;
    case 2: PORTECLR = 0xFF; PORTESET = 0x03; break;
    case 1: PORTECLR = 0xFF; PORTESET = 0x01; break;
    case 0: PORTECLR = 0xFF; PORTESET = 0x00; break;
  }
}

/* Waits for user input on the start screen. Uses the time taken to seed srand().
   Author: Erik Martines Sanches */
void time_start_screen(void){
  timeoutcount = 0;
  display_image(0,icon);
  int counter_old = timeoutcount;
  while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30)){
      delay_for_random_seed = timeoutcount;
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
}

/* Shows the objective of the game and waits for a button press. 
   Author: Erik Martines Sanches */
void show_mission(){
  int counter_old = timeoutcount;
  currentDirection = NONE;
    display_string(0, "Grab all balls");
    display_string(1, "before the LEDs");
    display_string(2, "turn off. Don't");
    display_string(3, "go too far out.");
    display_update();
	while(1){
    process_input();
    if ((currentDirection == RIGHT || currentDirection == DOWN || currentDirection == UP || currentDirection == LEFT) && ((timeoutcount - counter_old) > 30) ){
      quicksleep(1599999); //To give some time for pushbutton to lift.
      currentDirection = NONE;
      break;
    }
  }
} 

/* Initially we insert the items into items array, with appropriate settings 
  Author: Erik Martines Sanches */
void insert_items(void){
  int i;
  for(i = 0; i < NUMBER_OF_ITEMS - 1; i++){ //Insert balls and leave room for player.
    int random_x = rand() % 125;
    int random_y = rand() % 29;
    items[i] = (node){false, true, 0, 0, 0.0, {{random_x+1, random_y}, {random_x+2, random_y+1}, {random_x+1, random_y+2}, {random_x, random_y+1}}}; //Partially randomised locations.
  }
  items[NUMBER_OF_ITEMS-1] = (node){true, true, 0, 0, INITIAL_SPEED, {{59,16}, {64,16}, {64,21}, {59,21}}}; //Insert player in the middle of the screen.
}

/* This function is called repetitively from the main program 
    Author: Erik Martines Sanches*/
void labwork(void){
  if(prepare_frame_ok){
    prepare_frame_ok = false;
    process_input();
    update_leds();
    prepare_frame(); //Clears frame. This also applies offsets items.
    if (hidden_items >= ITEMS_FOR_AVOIDANCE){ //Balls avoid player if player is big enough and has picked up a number of balls.
      node *p;
      for (p = items; p < &items[NUMBER_OF_ITEMS]; ++p){
        if (p->movable && p->size == 2){
          avoid_player();
        }
      }
    }
    check_bounds_and_collisions(); //If outside screen, reset into start position. If over pickupable object, do pick it up and potentially tranform.
    display_image(0, frame); 
    set_game_status(); //Whether to continue the game or not.
  }
}

/* Initializes the game on every round. Author: Erik Martines Sanches */
void labinit(void) {
  T2CON = 0x0;	      //Stop timer and clear control register.
	T2CONSET = 0x70;    //Set prescale to 256.
	TMR2 = 0x0;		      //Clear timer register.
	//80000000/256/10 = 31250 decimal = 0x7A12 this is for 0.1s.
	//PR2 = 0x7A12;
  PR2 = 0x186A; //50Hz
	//Setting up interrupts for timer2:
	IFSCLR(0) = 0x100;  //Clear T2IF in IFS2 register.
	IPCSET(2) = 0x1F;   //Configuring interrupt priority and subpriority to 7 and 3 (highest).
	IECSET(0) = 0x100;  //Set T2IE interrupt enable bit in IEC0 register.
	enable_interrupt(); //Enables interrupt.
	T2CONSET = 0x8000;  //Start timer.
  //Setup registers for input change notification (pushbuttons). 
  //BTN3 Chipkit pin# 36 / RD6
  // TRISDSET = 0x40;   //Set pushbutton btn 3 on RD6 as input.
  // CNENSET = 0x8000;  //CNEN15 enable
  // CNCONSET = 0x8000;   //CNCON on
  // CNPUESET = 0x8000;   // change notice pull up enable
/*   IFSCLR(0) = 0x8;//Clear INT0 IF interrupt flag
  IPCSET(0) = 0x1F000000; //INT0 priority and subpriority set to max.
  IECSET(0) = 0x8; //INT0 Interrupt enable. */
  currentDirection = NONE;
  hidden_items = 0;
  timeoutcount = 0;
  delay_for_random_seed = 0;
  PORTECLR = 0xFF;
  time_start_screen();    //Get a time period to seed srand() with.
  show_mission();
  srand(delay_for_random_seed);
  insert_items();         //Inserts into items array.
  time_units_left = 8;
  currentDirection = NONE;
  oldDirection = NONE;
  game_status = CONTI;
}

/* Interrupt Service Routine. Author: Erik Martines Sanches*/
void user_isr(void){
/*      if (((IFS(0) >> 3) & 0x1) == 1 ){ //check if interrupt flag from BTN2 pin 34, has been set
      PORTECLR = 0xFF;
      PORTEINV = 0x80;
      draw_pixel(0,0);
      IFSCLR(0) = 0x8; //clear the flag
    } */
  	if(T2IF_raised()) {
      timeoutcount++;
      //if(timeoutcount == 10){
      //  resetT2IF();
      prepare_frame_ok = true;
      //  timeoutcount = 0;
      //} else {
      resetT2IF();
      //}
	}
  return;
}
