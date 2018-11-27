#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ncurses.h>
//tickrate
int TICK = 100000;
int TICKINC = 1000;
//arrows 
#define DOWN  1
#define UP    2
#define LEFT  3
#define RIGHT 4
//points and length
#define MOVIN 1
#define EATIN 4 // change this when making numbers appear, this will be what they get when they hit a random number
#define SNAKELENGTH 2
//collision checks
#define USER 1
#define SELF 2
#define WALL 3
//snake items (peices, trophies, empty spacing)
#define SNAKEPEICE 'o'
#define EMPTY      ' '

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

//set the timer so we can use TICK (how fast the snake moves)
void SetTime(void);
//set the signals 
void SetSig(void);
//create the snake 
void snakeCreate(void);
//moves the snake
void snakeMove(void);
//this function draws area, snake, and food
void snakeDraw(void);
//frees taken snake mem allocated
void releaseSnake(void);
//randomly places food within the games board
void spawnFood(void);
//this function handles all errors throughout the game
void ErrorOut(char * msg);
// happens when a normal game 'error' occurs (hit wall, self, etc..)
void quitOut( int reason );
//gets the current terminal size (this is used for random fruit genorator)
void GetTermSize(int * rows, int * cols);
// starts the sig handler
void handler();
//changes direction on key press
void dirChange();
//start direction is chosen at random
void startDirection(int * direction);

//declare global struct
struct snake_piece {
	struct snake_piece * next;
	int x;
	int y;
};
typedef struct snake_piece SNAKE;

static SNAKE * snake;
static int direction;
static int rows, cols;
int score = 0; // this is the main score
int printednumber;
//ZZZZZZZZZZZZZZ
int piecesToAdd = 0;
//UUUUUUUUUUUUU

WINDOW * mainwin;
int oldsettings;

int main(void){
	
	//rng seed, timer set, register handlers
	srand( ( unsigned ) time(NULL)) ;
	SetTime();
	SetSig();

	//Curses will be set here
	if ( (mainwin = initscr()) == NULL){
		perror("Could not set ncurses");
		exit(EXIT_FAILURE); // might change to case statment inside of quit function
	}

	noecho();
	keypad(mainwin, TRUE);
	oldsettings = curs_set(0); // save term settings using curs set and 0 (will be 1 when done)
	
	//create snake and draw the board 
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

	return EXIT_SUCCESS; //doesn't
}

void SetTime(void){
	struct itimerval it;

	timerclear(&it.it_interval);
	timerclear(&it.it_value);

	//set time

	it.it_interval.tv_usec = TICK;
	it.it_value.tv_usec    = TICK;
	setitimer(ITIMER_REAL, &it, NULL);
}

//sig handlers
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

void snakeCreate(void){
	SNAKE * temp;
	GetTermSize(&rows, &cols);
	int x = 4, y = 10, i;

	//make sure we can hold the whole snake
	for (i = 0; i < SNAKELENGTH; i++){
		if (i == 0){
			if ((snake = malloc(sizeof(SNAKE))) == NULL)
				ErrorOut("Could not allocate mem");
			temp = snake;
		}
		else {
			// if this isnt the head of mr. snake
			if ((temp->next = malloc(sizeof(SNAKE))) == NULL)
				ErrorOut("Coud not allocate mem");
			temp = temp->next; // go to next peice (tail kinda)
		}
		temp->x = x;   //place each peice down
		temp->y = y++; //for the snake
	}
	temp->next = NULL;

	GetTermSize(&rows, &cols);
}

void snakeDraw(void){
	SNAKE * temp = snake;

	//area creation (simple box call and use default borders)
	box(stdscr, 0, 0); //you can change the 0's to any chars you want as the border (goes in y, x order)

	//snake gets drawn to screen
	while ( temp ){
		move(temp->y, temp->x);
		addch(SNAKEPEICE);
		temp = temp->next; // get next peice and go to top of this loop, do for all peices in given (2 initially)
	}

	//foods
	// this will change when we create the random food creator
	spawnFood();
}

void snakeMove(void){
	SNAKE * temp = snake;
	int x, y, ch;
	char trophy = printednumber + '0';
	//go to end of mr. snake

	while ( temp->next != NULL )
		temp = temp->next;

	//create new snake peice to add to it

	if ((temp->next = malloc(sizeof(SNAKE))) == NULL)
		ErrorOut("Could not allocate mem while inside of snakeMove() ");

	// find where to add mr. snakes new tail peice!

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

void spawnFood(void){
	int x, y;

	do {
		x = rand() % (cols - 3) + 1;
		y = rand() % (rows - 3) + 1;
		move(y, x);
	} while ( inch() != EMPTY ); 
	
	printednumber = rand() % 10;
	addch(printednumber+'0');
}

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


//giving back mem we took for snake during malloc
void releaseSnake(void){
	SNAKE * temp = snake;

	while( snake ){
		temp = snake->next;
		free(snake);
		snake = temp;
	}
}

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
// these are successful quits
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
		
		default:
			printf("\nBYE\n");
			printf("Your score-> %d\n", score);
			break;
	}
	exit(EXIT_SUCCESS);
}

void GetTermSize(int *rows, int *cols){
	struct winsize ws;

	if ( ioctl(0, TIOCGWINSZ, &ws) < 0 ){
		perror("Could not get term size..");
		exit(EXIT_FAILURE);
	}

	*rows = ws.ws_row;  //setting the global row
	*cols = ws.ws_col; // now cols
}

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
