/*
Snake Game C implementation
Authors: Jacob Pawlak, Zachary Urso

This project was laid out and designed from a basic format. Using an agile approach to tackle small tasks for the larger result
we were able to divide and conquer the tasks at hand into several for each developer.

Below you will be able to go through the code and see the functions in which we both implemented. Most were written together, 
but added names to those of major progess to completion.

*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ncurses.h>
//tickrate
int TICK = 199000;
int TICKINC = 1000;
// direction that I talk about at the end, it is easier to define a name so you know direction by name not numbers
#define DOWN  1
#define UP    2
#define LEFT  3
#define RIGHT 4
// points and length
#define MOVIN 1
#define EATIN 4 // change this when making numbers appear, this will be what they get when they hit a random number
#define SNAKELENGTH 1
// collision checks
#define USER 1
#define SELF 2
#define WALL 3
#define WIN  4
// snake items (peices, trophies, empty spacing)
#define SNAKEPEICE 'o'
#define EMPTY      ' '
// numbers 'chars' that are placed as trophy
#define ZERO       '0'
#define ONE        '1'		
#define TWO        '2'
#define THREE      '3'
#define FOUR       '4'
#define FIVE       '5'
#define SIX        '6'
#define SEVEN      '7'
#define EIGHT      '8'
#define NINE       '9'

/*
Here we are giving prototypes of our functions
*/

// set the timer so we can use TICK (how fast the snake moves)
void SetTime(void);
// set the signals 
void SetSig(void);
// create the snake 
void snakeCreate(void);
// moves the snake
void snakeMove(void);
// this function draws area, snake, and food
void snakeDraw(void);
// frees taken snake mem allocated
void releaseSnake(void);
// randomly places food within the games board
void spawnFood(void);
// this function handles all errors throughout the game
void ErrorOut(char * msg);
// happens when a normal game 'error' occurs (hit wall, self, etc..)
void quitOut( int reason );
// gets the current terminal size (this is used for random fruit genorator)
void GetTermSize(int * rows, int * cols);
// starts the sig handler
void handler();
// changes direction on key press
void dirChange();
// start direction is chosen at random
void startDirection(int * direction);

// declare global struct
struct snake_piece {
	struct snake_piece * next;
	int x;
	int y;
};
typedef struct snake_piece SNAKE;

static SNAKE * snake;
static SNAKE * tail;
static int direction;
static int rows, cols;
int score = 0; // this is the main score
int printednumber;
int piecesToAdd = SNAKELENGTH-1;
int slength;

WINDOW * mainwin;
int oldsettings;

/*
Written by Jaocb Pawlak
This is our main: here we are able to do many preparations before we start the code, as well as where we can handle diretional
changes in a loop that is infinite until an alarm is handled by user quitting, error, or by a game rule.
We set the seed for the random number generator using the srand and time functions, and then set the timer and the signal handler
initializing curses is a curial part of the program, and we handle if there is an error.  We then set noecho() so commands will
not be echoed out to the screen. We initialize our window (mainwin) and set keypad to TRUE. Another important part is grabbing
the current settings before we change anything for good, this is boolean 0-1 (1 for setting back to old). We then enter the game
by choosing a direction, creating the snake (intializing and allocating memory, and then draw the first part to the screen).
Our while llook then runs which is dependent on arrow key switching and keys being hit to quit.
*/
int main(void){
	
	// rng seed, timer set, register handlers
	srand( ( unsigned ) time(NULL)) ;
	SetTime();
	SetSig();

	// Curses will be set here.. error out with perror if problem occurs
	if ( (mainwin = initscr()) == NULL){
		perror("Could not set ncurses");
		exit(EXIT_FAILURE);
	}

	noecho(); // don't echo out user input
	keypad(mainwin, TRUE); // user the mainwindow / keys
	oldsettings = curs_set(0); // save term settings using curs set and 0 (will be 1 when done)
	
	// give snake random direction, create snake, and draw the board
	startDirection(&direction);
	snakeCreate();
	snakeDraw();

	while (1){
		int arrow = getch();

		switch( arrow ){

			case KEY_UP:
				dirChange(UP);
				break;
			case KEY_DOWN:
				dirChange(DOWN);
				break;
			case KEY_LEFT:
				dirChange(LEFT);
				break;
			case KEY_RIGHT:
				dirChange(RIGHT);
				break;
			case 'Q':
			case 'q':
			case KEY_BACKSPACE:
				quitOut(USER);
				break;
		}
	}

	return EXIT_SUCCESS; // doesn't actually ever get here because it works
}
/*
Written by Jacob Pawlak
This function sets the timer 'TICK' using setitimer (which is called to 'update') TICK will be the speed at which the
snake is travelling. This is set global so we do not have to pass it all around, and then we are able to update in a linear
way when trophies are eaten
*/
void SetTime(void){
	struct itimerval it;

	timerclear(&it.it_interval);
	timerclear(&it.it_value);

	//set time

	it.it_interval.tv_usec = TICK;
	it.it_value.tv_usec    = TICK;
	setitimer(ITIMER_REAL, &it, NULL);
}

/*
Written by Jacob Pawlak
This sets the sig handlers for the program, another function near end shows use. ALRM is user input to move snake and others
are built for errors.
*/
void SetSig(void){
	struct sigaction sa;

	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGTERM,  &sa, NULL);
	sigaction(SIGINT,   &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGTSTP, &sa, NULL);
}

//written by Zachary Urso
void snakeCreate(void){
	SNAKE * temp;
	GetTermSize(&rows, &cols);
	int x = cols/2, y = rows/2;
	if ((snake = malloc(sizeof(SNAKE))) == NULL)
	{
		ErrorOut("Could not allocate mem");
	}
	temp = snake;
	temp->x = x;
	temp->y = y;
	temp->next = NULL;
	tail = temp;
	slength = SNAKELENGTH;
}
//Written by Zachary Urso
void snakeDraw(void){
	SNAKE * temp = snake;

	//area creation (simple box call and use default borders)
	box(stdscr, 0, 0); //you can change the 0's to any chars you want as the border (goes in y, x order)

	//snake head gets drawn to screen
	move(temp->y, temp->x);
	addch(SNAKEPEICE);
	//foods initial spawn (will change when we set timer on food most likely)
	spawnFood();
}

//Written by Zachary Urso
void snakeMove(void){
	SNAKE * temp = tail;
	int x, y, ch;
	char trophy = printednumber + '0';
	//create new snake peice to add to it

	if ((temp->next = malloc(sizeof(SNAKE))) == NULL)
		ErrorOut("Could not allocate mem while inside of snakeMove() ");

	// Jacob Pawlak: find where to add mr. snakes new tail peice!
	switch(direction){
		case DOWN:
			x = temp->x;
			y = temp->y + 1;
			break;
		case UP:
			x = temp->x;
			y = temp->y - 1;
			break;
		case LEFT:
			x = temp->x - 1;
			y = temp->y;
			break;
		case RIGHT:
			x = temp->x + 1;
			y = temp->y;
			break;
	}

	//fill the new tail peice
	temp       = temp->next;
	temp->next = NULL;
	temp->x    = x;
	temp->y    = y;
	tail = temp;
	//check collision switch statement
	move(y, x);
	ch = inch();
	addch(SNAKEPEICE);
	if(ch==EMPTY||ch==trophy)
	{	
		score += MOVIN;
		if(ch==trophy)
		{
			piecesToAdd += trophy - '0';
			slength += trophy - '0';
			if(slength >= (rows+cols))
			{
				quitOut(WIN);
			}
			TICK-=TICKINC*(trophy - '0');
			spawnFood();
               		SetTime();
               		score += EATIN;
		}
		if(piecesToAdd==0)
		{
			temp = snake->next;
			move(snake->y, snake->x);
			addch(EMPTY);
			free(snake);
			snake = temp;
		}
		else
		{
			piecesToAdd--;
		}
		refresh();
	}
	else if(ch==SNAKEPEICE)
	{
		quitOut(SELF);
	}
	else
	{
		quitOut(WALL);
	}
}

/*
Written by Jacob Pawlak
This function spawns a random trophy which is represented by any number 1-9, the coordinates are chose at random as well
and denoted by x and y. We use the rows and cols globals (screen size dependant) to help generate random places.
*/
void spawnFood(void){
	int x, y;

	do {
		x = rand() % (cols - 3) + 1;
		y = rand() % (rows - 3) + 1;
		move(y, x);
	} while ( inch() != EMPTY ); 
	
	printednumber = (rand() % 9)+1;
	addch(printednumber+'0');
}
/*
Written by Jacob Pawlak

When the user provides input this function is called, it provides the direction of which the snake will move in using x
and y coordinates and the linked list. This function also quits out when snake hits itself. at the ende direction is set to d
*/
void dirChange(int d){
	SNAKE * temp = snake;

	//go to end of mr.snakey
	while ( temp->next != NULL )
		temp = temp->next;

	switch(d){
		case LEFT:
			if (direction == RIGHT )
				quitOut(SELF); // if hit itself reverse
			move(temp->y, temp->x - 1);
			break;

		case RIGHT:
			if (direction == LEFT )
				quitOut(SELF); // if hit itself reverse
			move(temp->y, temp->x + 1);
			break;

		case UP:
			if (direction == DOWN )
				quitOut(SELF);// if hit itself reverse
			move(temp->y - 1, temp->x);
			break;

		case DOWN:
			if(direction == UP)
				quitOut(SELF);// if hit itself reverse
			move(temp->y + 1, temp->x);
			break;
	}
	direction = d;
}

/*
Written by Jacob Pawlak
This function gives back the memory we allocated during malloc for the snake peices. Goes through the linked list snake
and frees, moves to next, so on and so forth depending on how big it ends up growing.
*/
void releaseSnake(void){
	SNAKE * temp = snake;

	while( snake ){
		temp = snake->next;
		free(snake);
		snake = temp;
	}
}

/*
Written by Jacob Pawlak
When we error out and it is not in the case of a user quitting, losing, or winning, then the error is handled here
we end the window put settings back to before we started, refresh, release snake, and provide the perror message
*/
void ErrorOut(char * msg){

	// lets clean this up due to error

	delwin(mainwin);
	curs_set(oldsettings);
	endwin();
	refresh();
	releaseSnake(); // give back allocated snake mem

	perror(msg);
	exit(EXIT_FAILURE);
}

/*
Written by Jacob Pawlak
When a user quits out using the commands provided in the code, wins, hits itself, or hits the wall then we handle it 
through this function, giving the users score and the reason why we ended the game
*/

void quitOut(int reason){

	// lets clean this up due to quit call
	delwin(mainwin);
	curs_set(oldsettings);
	endwin();
	refresh();
	releaseSnake(); // give back allocated snake mem

	switch(reason) {
		case WALL:
			printf("\nYou hit the wall! GG\n");
			printf("Your score-> %d\n", score);
			break;

		case SELF:
			printf("\nYou hit yourself! GG\n");
			printf("Your score-> %d\n", score);
			break;

		case WIN:
			printf("\nYou win! GG\n");
                        printf("Your score-> %d\n", score);
                        break;
		
		default:
			printf("\nBYE\n");
			printf("Your score-> %d\n", score);
			break;
	}
	exit(EXIT_SUCCESS);
}

/*
Written by Jacob Pawlak
Gets the terminal size for the current terminal using ioctl. ncurses has a prebuilt using key words such as ROWS COLS but I
found implenting this was we can give error, and us the globals set to our own way
*/
void GetTermSize(int *rows, int *cols){
	struct winsize ws;

	if ( ioctl(0, TIOCGWINSZ, &ws) < 0 ){
		perror("Could not get term size..");
		exit(EXIT_FAILURE);
	}

	*rows = ws.ws_row;  //setting the global row
	*cols = ws.ws_col; // now cols
}

/*
Written by Jacob Pawlak
Here is the other part of the handler that I spoke of before. We use the previous function to name, but this is what will happen
when each alarm is going off. If ALRM then we call snakemove() and return, if we hit TERM or INT then we close out the game
*/
void handler(int signum){

	switch( signum ){
		case SIGALRM:
			//gets from the timer (tickrate)
			snakeMove();
			return;

		case SIGTERM:
		case SIGINT:

			//do some cleanup... bc we're nice hehe
			delwin(mainwin);
			curs_set(oldsettings);
			endwin();
			refresh();
			releaseSnake(); // give back allocated snake mem
			exit(EXIT_SUCCESS); 
	}
}
/*
Written by Jacob Pawlak
Here is where we define the randomness for the start direction. I simply chose a random number from 1-4, based off that number from
the rng we are able to provide a case statement for each one, i think it is cleaner than using if statements, therefor we used case
for direction. I defined UP DOWN LEFT RIGHT as globals so when we move elsewhere at other times then the names are clear to see
instead of using 1-4
*/
void startDirection(int *direction){

	int randomNumber;
	randomNumber = rand() % 4 + 1;

	switch( randomNumber ){
		case 1:
			*direction = UP;
			break;
		case 2:
			*direction = DOWN;
			break;
		case 3:
			*direction = LEFT;
			break;
		case 4:
			*direction = RIGHT;
			break;

	}

}
