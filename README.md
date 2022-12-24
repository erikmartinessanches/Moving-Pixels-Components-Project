# Moving Pixels Components Project

Collect all the balls before all of the LEDs turn off! As you collect more balls, your body adapts by becoming faster and growing in size. Just like in sports, balls start avoiding the player. And watch out, if you leave this court, you will be teleported to the middle, losing speed and size. Based on some provided code for displaying strings and updating the screen over SPI. I also lifted Bresenham's line algorithm but I could have showed it off better with more irregular shapes. I also used a program to translate a picture into the arrays making up the retro looking text.

The game has a 512 byte long array representing the frame. A TMR2 raises an IFS with a frequency of 50 Hz which polls the input. Preparation of a new frame begins by applying an offset according to input, empties the frame array and sets the appropriate bits in the frame array to be later displayed. Further checks determine whether all of the player’s vertices have left the screen, in which case the player is moved to the middle and has its size and speed reset to default. It was fun figuring out how to change the display array each frame, among other things. The game goes to start after a win or a loss.

I mainly wrote mipslabwork.c and modified other files where noted.
