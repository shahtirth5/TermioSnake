/**
 * We add them above our includes, because the header files we’re including use
 * the macros to decide what features to expose.
 */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
// #define _GNU_SOURCE
#define TRUE 1
#define FALSE 0

// Includes
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>

// Defines & Globals
#define CTRL_KEY(k) ((k) & 0x1f) // CTRL + Key Ands the key value with 00011111
int bodyLength;
int foodX;
int foodY;
int score;
int snakeDirection;
int isDead;

// Enums
enum keys {
    ARROW_UP = 1000,
    ARROW_LEFT = 1001,
    ARROW_RIGHT = 1002,
    ARROW_DOWN = 1003,
    DEL_KEY = 1008,
    SPACE = 1009
};

// Key Press Manipulation
int isSpace = FALSE;

// all required Structures:

/**
 * @brief The GameElement is the layer over which the game will be rendered
 */
struct GameElement{
    struct termios original_termios;
};
struct GameElement E;

#pragma region  SnakeBody
// ---------- START OF SNAKE BODY ----------

/**
 * @brief Structure that defines the body of the snake along with its head and tail.
 */
struct SnakeSegment{
    int x; // defines the x coordinate of that specific snake segment
    int y; // defines the y coordinate of that specific snake segment
    struct SnakeSegment *next; // defines a pointer to the next segment of that specific snake segment
};
struct SnakeSegment *startOfBody = NULL;

/**
 * The following function adds a snake segment to the end of its body
 *
 * @param x describe this
 * @param y describe this
 */
void addSnakeSegmentToEnd(int x, int y) { // Same with commas/// write like this : attr1, attr2, attr3, ...
    struct SnakeSegment *t;
    struct SnakeSegment *temp;
    t = (struct SnakeSegment *)malloc(sizeof(struct SnakeSegment));
    bodyLength++;

    if(startOfBody == NULL) {
        startOfBody = t;
        startOfBody->x = x;
        startOfBody->y = y;
        startOfBody->next = NULL;
        return;
    }

    temp = startOfBody;
    while(temp->next != NULL){
        temp = temp->next;
    }

    temp->next = t;
    t->x = x;
    t->y = y;
    t->next = NULL;
}

/**
 * The following function adds a snake segment to the start of its body
 *
 * @param x specifies X-coordinate of the snake segment to be added
 * @param y specifies Y-coordinate of the snake segment to be added
 */
void addSnakeSegmentToStart(int x, int y) {
    struct SnakeSegment *newSegment = (struct SnakeSegment *)malloc(sizeof(struct SnakeSegment));

    if(newSegment == NULL) {
        printf("Segmentation Fault (Core dumped)");
        exit(0);
    } else {
        newSegment->x = x;
        newSegment->y = y;
        newSegment->next = startOfBody;
        startOfBody = newSegment;
        bodyLength++;
    }
}

/**
 * The following function returns pointer to a snake segment at a specific index starting from 0 being the snake's head
 * 
 * @param index defines the index of the snake segments of the snake body
 * @return a pointer to the specific snake segment at the specified index
 */ 
struct SnakeSegment* get(int index){
    int count = 0;
    struct SnakeSegment *temp = startOfBody;

    if(index == 0){
        return startOfBody;
    }else{
        while(temp->next != NULL){
            temp = temp->next;
            count++; // also leave lines if the code gets too messy ...

            if(count == index){
                return temp;
            }
        }
    }
}

/**
 * The following function returns the head of the snake
 * 
 * @return a pointer to the head (snake segment with index 0) of that specific snake segment
 */
struct SnakeSegment* getSnakeHead(){
    return startOfBody;
}

/**
 * The following function returns the tail of the snake
 * 
 * @return a pointer to the tail of that specific snake segment
 */
struct SnakeSegment* getSnakeTail(){
    return get(bodyLength-1);
}

/**
 * The following function removes the tail of the snake
 */
void removeSnakeTail(){
    struct SnakeSegment *lastSegment = getSnakeTail();
    struct SnakeSegment *secondLastSegment = get(bodyLength-2);

    // this is a different operation, that is ; its different than initailization..so leave a lien.
    secondLastSegment->next = NULL;
    free(lastSegment);
    bodyLength--;
}

/**
 * The following function removes all the snake segments
 */
void removeAll(){
    struct SnakeSegment *temp = startOfBody;

    while(startOfBody != NULL){
        temp = startOfBody;
        startOfBody = startOfBody->next;
        free(temp);
        bodyLength--;
    }
}

// ---------- End of Snake Body ----------

#pragma endregion

#pragma region appendbuffer
// ---------- START OF APPENDBUFFER ----------

/**
 * @brief Structure that defines a string as a character pointer and the length of that string
 */ 
struct appendBuffer{
    char* a;
    int len;
};

void reset_string(struct appendBuffer *str){
    str->a = NULL;
    str->len = 0;
}

/**
 * The following function appends string to the current string
 * 
 * @param *str Pointer to the append buffer structure object
 * @param *c Pointer to the string to append
 * @param len Length of the string to append
 */
void append(struct appendBuffer *str , const char *c , int len){
    // Reallocate the string with increased length
    char *temp = (char *)realloc(str->a , str->len+len);
    if(temp == NULL){
        return;
    }

    // Copy the string to be appended from the previous length to current length
    // Since the new length now is (str->len + len)
    memcpy(&temp[str->len] , c , len);
    str->a = temp;
    str->len += len;
}

/**
 * Free up the memory consumed by the string i.e destroy the string
 */
void destroyString(struct appendBuffer *str){
    free(str->a);
}

// ---------- End of Append Buffer ----------

#pragma endregion
struct appendBuffer string;
// reset_string(string); // Globally defined string that has appendbuffer features that we will use to render everything in the game 

#pragma region manipulatingTerminal
// ---------- Manipulating Terminal ---------- 

/**
 * The following function ends the program with an error message
 *
 * @param s The error message
 */
void die(const char *s) {

    /** Description of two functions below given in refreshScreen function */
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    /***********************************************************************/

    /**
     * Most C library functions that fail will set the global errno variable to indicate what the error was.
     * perror() looks at the global errno variable and prints a descriptive error message for it.
     * It also prints the string given to it before it prints the error message,
     * which is meant to provide context about what part of your code caused the error.
     */
    perror(s);
    exit(1);
}

/**
 * The following function disables raw mode of the terminal
 */
void disableRawMode(){
    if(tcsetattr(STDIN_FILENO , TCSAFLUSH , &E.original_termios) == -1){
        die("tcsetattr");
    }
}

/**
 * The following function enables raw mode of terminal
 */
void enableRawMode(){
    if(tcgetattr(STDIN_FILENO , &E.original_termios) == -1 ){
        die("tcgetattr");
    }
    atexit(disableRawMode);//This is a method of stdlib and will execute the parameter when the program
    // is being executed

    struct termios raw = E.original_termios;
    tcgetattr(STDIN_FILENO , &raw);
    /**
     * @c_lflag :
     * 	The c_lflag field is for “local flags”.
     *  A comment in macOS’s <termios.h> describes it as a “dumping ground for other state”.
     *  So perhaps it should be thought of as “miscellaneous flags”.
     *  The other flag fields are c_iflag (input flags), c_oflag (output flags), and c_cflag (control flags),
     *  all of which we will have to modify to enable raw mode.
     *
     * @ECHO :
     * 		=> Here ECHO is the thing that when you type something in keyboard it
     *   		gets printed in the terminal. In case of terminal we don't want that.
     *     		We will render everything by ourselves.
     *
     *		=> Echp is a bitflag, defined as 00000000000000000000000000001000 in binary.
     *  		We use the bitwise-NOT operator (~) on this value to get 11111111111111111111111111110111.
     *    		We then bitwise-AND this value with the flags field, which forces the fourth bit in the flags
     *      	field to become 0, and causes every other bit to retain its current value. Flipping
     *       	bits like this is common in C.
     *
     * @ICANON :
     * 		=> There is an ICANON flag that allows us to turn off canonical mode. This means we will finally
     * 			be reading input byte-by-byte, instead of line-by-line.
     *
     *		=> ICANON is not an input flag . Its a 'Local Flag' in the c_lflag .
     *
     * 		=> Due to negating the ICANON flag the program will exit on pressing q rather than  'q ENTER'
     *
     * @ISIG :
     * 		=> When Ctrl+C is pressed in terminal it sends a SIGINT signal to the process which causes
     * 			the process to terminate
     *
     * 		=> When Ctrl+Z is pressed in terminala it sends a SIGSTP signal to the process which causes
     * 			the process to suspend
     *
     * 		=> ISIG is not an input flag . Its a 'Local Flag' in the c_lflag .
     *
     * 		=> Due to negating the ISIG flag the program wont send SIGINT and SIGSTP signals to the process
     *
     * @IEXTEN :
     * 		=> On some systems, when you type Ctrl-V, the terminal waits for you to type another character
     * 			and then sends that character literally. For example, before we disabled Ctrl-C,
     * 		 	you might’ve been able to type Ctrl-V and then Ctrl-C to input a 3 byte.
     * 		  	We can turn off this feature using the IEXTEN flag.
     * 		   	Turning off IEXTEN also fixes Ctrl-O in macOS, whose terminal driver is otherwise
     * 		    set to discard that control character.
     *
     * 		=> IEXTEN is not an input flag . Its a 'Local Flag' in the c_lflag.
     *
     * 		=> Due to negating IEXTEN flag , Ctrl-V can now be read as a 22 byte, and Ctrl-O as a 15 byte.
     *
     **/
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_lflag &=

    /**
     * @raw.c_iflag :
     * 	It is a terminal input flag
     *
     * @IXON :
     * 		=> By default, Ctrl+S and Ctrl+Q are used for software flow control.
     * 			Ctrl-S stops data from being transmitted to the terminal until you press Ctrl-Q.
     * 		 	This originates in the days when you might want to pause the transmission of data
     * 		 	to let a device like a printer catch up. Let’s just turn off that feature.
     *
     * 		=> I in IXON stands for input
     *
     * 		=> XON comes from two terms XON and XOFF : XOFF to pause transmission AND XON to resume transmission
     *
     * @ICRNL :
     *  	=> If you run the program now and go through the whole alphabet while holding down Ctrl,
     *  	 	you should see that we have every letter except M. Ctrl-M is weird: it’s being read as 10,
     *  	 	when we expect it to be read as 13, since it is the 13th letter of the alphabet,
     *  	 	and Ctrl-J already produces a 10. What else produces 10? The Enter key does.
     *  	 	It turns out that the terminal is helpfully translating any carriage returns (13, '\r')
     *  	 	inputted by the user into newlines (10, '\n'). Let’s turn off this feature.
     *
     *     	=> I in ICRNL stands for input
     *
     * 		=> in ICRNL : CR stands for Carriage Return AND NL stands for NewLine
     *
     * 		=> Due to negating ICRNL Ctrl+M is read as 13 and ENTER is also read as 13
     *
     * @BRKINT , @IPCK , @ISTRIP :
     * 		=> Most of them are turned off by default in modern emulators. But just for safety disabled
     * 			them manually.
     *
     * 		=> When BRKINT is turned on, a break condition will cause a SIGINT signal to be sent to the program,
     * 		 	like pressing Ctrl-C.
     *
     *		=> INPCK enables parity checking, which doesn’t seem to apply to modern terminal emulators.
     *
     *  	=> ISTRIP causes the 8th bit of each input byte to be stripped, meaning it will set it to 0.
     *  	 	This is probably already turned off.
     *
     */

    raw.c_iflag &= ~(IXON);

    /**
     * @raw.c_oflag :
     * 	It is a terminal output flag
     *
     * @OPOST :
     * 		=> It translates each newline ("\n") we print into a carriage return followed by a newline ("\r\n").
     * 		 	The terminal requires both of these characters in order to start a new line of text.
     * 		 	The carriage return moves the cursor back to the beginning of the current line,
     * 		 	and the newline moves the cursor down a line, scrolling the screen if necessary.
     *
     * 		=> O stands for Output
     *
     *		=> Post stands for post processing of output
     *
     * 		=> After negating the OPOST flag , you’ll see that the newline characters we’re printing are only moving
     * 		 	the cursor down, and not to the left side of the screen.
     * 		 	To fix that, let’s add carriage returns to our printf() statements.
     */
    raw.c_oflag &= ~(OPOST);

    /**
     * @raw.c_cflag:
     * 	It is a terminal control flag
     *
     * @CS8 :
     * 		=> CS8 is not a flag, it is a bit mask with multiple bits,
     * 	 		which we set using the bitwise-OR (|) operator unlike all the flags we are turning off.
     * 	 		It sets the character size (CS) to 8 bits per byte. On my system, it’s already set that way.
     */
    raw.c_cflag |= (CS8);

    /**
     * @raw.c_cc :
     * 	CC stands for control characters
     *
     *
     * 	=> Currently, read() will wait indefinitely for input from the keyboard before it returns.
     * 		 What if we want to do something like animate something on the screen while waiting for user input?
     * 		 We can set a timeout, so that read() returns if it doesn’t get any input for a certain amount of time.
     *
     * @VMIN :
     *  	=> The VMIN value sets the minimum number of bytes of input needed before read() can return.
     *  		We set it to 0 so that read() returns as soon as there is any input to be read.
     *
     * @VTIME :
     *  => The VTIME value sets the maximum amount of time to wait before read() returns.
     * 		 It is in tenths of a second, so we set it to 1/10 of a second, or 100 milliseconds.
     * 		 If read() times out, it will return 0, which makes sense because its usual return value
     * 		 is the number of bytes read.When you run the program, you can see how often read() times out.
     * 		 If you don’t supply any input, read() returns without setting the c variable, which retains its 0
     *		 value and so you see 0s getting printed out. If you type really fast,
     *		 you can see that read() returns right away after each keypress,
     *		 so it’s not like you can only read one keypress every tenth of a second.
     */
    //raw.c_cc[VMIN] = 0;
    //raw.c_cc[VTIME] = 1;

    /**
     * The TCSAFLUSH argument specifies when to apply the change: in this case,
     * it waits for all pending output to be written to the terminal,
     * and also discards any input that hasn’t been read.
     */
    if(tcsetattr(STDIN_FILENO , TCSAFLUSH ,&raw) == -1 ){
        die("tcssetattr");
    }
}

// ---------- End of Manipulating Terminal ----------
#pragma endregion

#pragma region input
// ---------- Input ----------

/**
 * The following function reads a keypress
 * 
 * @return the value of keypressed according to enum keys defined earlier
 */ 
int readKey() {
    int nread;
    char c;
    /**
         * read() and STDIN_FILENO come from <unistd.h>. We are
         * asking read() to read 1 byte from the standard input
         * into the variable c, and to keep doing it until there
         * are no more bytes to read. read() returns the number of
         * bytes that it read, and will return 0 when it reaches
         * the end of a file.w
         */
    nread = read(STDIN_FILENO, &c, 1) ;

    /**
     * To detect arrow keys :
     *  => When terminal gets input as escape characters "\x1b[" followed by 'A' , 'B' , 'C' , 'D'
     *      they act as arrow keys
     */
    if(c == '\x1b'){
        char seq[3];
        if(read(STDIN_FILENO , &seq[0] , 1) != 1) return '\x1b';
        if(read(STDIN_FILENO , &seq[1] , 1) != 1) return '\x1b';
        if(seq[0] == '['){
            /**
             * Escape character for PageUp is : "\x1b[5~"
             * Escape character for PageDown is : "\x1b[6~"
             * Escape character for Home are : "\x1b[1~" , "\x1b[7~" , "\x1b[H" , "\x1bOH"
             * Escape character for End are : "\x1b[4~" , "\x1b[8~" , "\x1b[F" , "\x1bOF"
             * Escape character for Del is :  "\x1b[3~"
             */
            switch (seq[1]){
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b'; // write the character equivalent of this
    } else if(c == 32) {
        return SPACE;
    } else {
        return c;
    }
}

/**
 * The following function specifies what the game should do when a particular key is pressed
 */
void processKeyPress() {
    int c = readKey(); // again leave a line

    switch(c) {
        case CTRL_KEY('q') :write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);

        case ARROW_UP:
            {
                if(snakeDirection != 2) {
                    snakeDirection = 0;
                }
            }
            break;
        case ARROW_LEFT:
            {
                if(snakeDirection != 1 ) {
                    snakeDirection = 3;
                }
            }
            break;
        case ARROW_DOWN:
            {
                if(snakeDirection != 0 ) {
                    snakeDirection = 2;
                }
            }
            break;
        case ARROW_RIGHT:
            {
                if(snakeDirection != 3 ) {
                    snakeDirection = 1;
                }
            }
            break;
        case SPACE :
            {
                isSpace = TRUE;
            }
            break;
    }
}

// ---------- End of Input ----------
#pragma endregion


#pragma region renderingfunctions
// ---------- Rendering Functions ----------

/**
 * The following function defines the string to be written on the terminal
 */
void writeTerminal(char chars[]){
    write(STDOUT_FILENO , chars , strlen(chars));
}

/**
 * The following function lets you set the cursor position to some specific co-ordinate after which one can write something which 
 * renders it on the coordinates specified
 * 
 * @param x X-coordinate on the terminal
 * @param y Y-coordinate on the terminal
 */
void setCursorPosition(int x , int y) {
    char buffer[10];
    sprintf(buffer , "\x1b[%d;%dH" , y , x);
    if(x <= 9 && y <= 9){
        append(&string , buffer , 6);
    } else if((x > 9 && y <= 9) || (y > 9 && x <= 9)){
        append(&string , buffer , 7);
    } else if(x > 9 && y > 9){
        append(&string , buffer , 8);
    }
}

/**
 * The following function makes the borders i.e the snake boundries
 * The Border coordinates are hard coded
 *
 *  ( 0 , 2 )  - - - - - ( 76 , 2 )
 *             -       -
 *             -       -
 *             -       -
 *             -       -
 *  ( 0 ,  22) - - - - - (76 , 22)
 */
void makeBorders(){
    char buffer[10];

    append(&string , "\x1b[0;3H" , 6);
    sprintf(buffer , "Score : %d" , score);
    append(&string , buffer , strlen(buffer));



    //Top Horizontal Border
    for (int i = 0; i < 75; i++) {
        sprintf(buffer , "\x1b[2;%dH" ,i);
        if(i <= 9) {
            append(&string , buffer , 6);
        } else {
            append(&string , buffer , 7);
        }
        append(&string , "-" , 1);
    }

    //Left Vertical Border
    for (int i = 2; i < 24; i++) {
        sprintf(buffer , "\x1b[%d;0H" ,i);
        if(i<=9) {
            append(&string , buffer , 6);
        } else {
            append(&string , buffer , 7);
        }
        append(&string , "-" , 1);
    }

    //Bottom Horizontal Border
    for (int i = 0; i < 76; i++) {
        sprintf(buffer , "\x1b[25;%dH" ,i);
        if(i<=9) {
            append(&string , buffer , 7);
        } else {
            append(&string , buffer , 8);
        }
        append(&string , "-" , 1);
    }

    //Right Vertical Border
    for (int i = 2; i < 24; i++) {
        sprintf(buffer , "\x1b[%d;75H" ,i);
        if(i<=9) {
            append(&string , buffer , 7);
        } else {
            append(&string , buffer , 8);
        }
        append(&string , "-" , 1);
    }
}

/**
 * The following function refreshes the screen
 */
void refreshScreen() {
    append(&string , "\x1b[2J" , 4);  //Clears The Screen
    append(&string , "\x1b[1;1H" , 6); //Refreshes the cursor to row : 1 , col : 1
    append(&string , "\x1b[?25l" , 6); //Hides the cursor
}

/**
 * The following function draws the snake
 */
void drawSnake() {
    char c[1];

    // info about the loop if its not too obvious
    for(int i = bodyLength-1 ; i > 0 ; i--) {
        setCursorPosition(get(i)->x , get(i)->y);
        sprintf(c , "%c" , isDead ? '+' : 'O');
        append(&string , c , 1);
    }

    setCursorPosition(getSnakeHead()->x , getSnakeHead()->y);

    if(snakeDirection == 0) {
        sprintf(c , "%c" , isDead ? 'X' : 'v');
    } else if(snakeDirection == 1) {
        sprintf(c , "%c" , isDead ? 'X' : '<');
    } else if(snakeDirection == 2) {
        sprintf(c , "%c" , isDead ? 'X' : '^');
    } else if(snakeDirection == 3) {
        sprintf(c , "%c" , isDead ? 'X' : '>');
    }
    append(&string , c , 1);
}

/**
 * The following function draws food
 */
void drawFood() {
    setCursorPosition(foodX , foodY);
    append(&string , "@" , 1);
}

/**
 * The following function manages direction of snake when a key is pressed
 */
void manageDirection() {
    switch(snakeDirection) {
        case 0 : //UP
            {
                addSnakeSegmentToStart(getSnakeHead()->x, (getSnakeHead()->y - 1));
            }
            break;

        case 1 : //RIGHT
            {
                addSnakeSegmentToStart((getSnakeHead()->x + 1), getSnakeHead()->y);
            }
            break;

        case 2 : //DOWN
            {
                addSnakeSegmentToStart(getSnakeHead()->x, (getSnakeHead()->y + 1));
            }
            break;

        case 3 ://LEFT
            {
                addSnakeSegmentToStart((getSnakeHead()->x - 1), getSnakeHead()->y);
            }
            break;
    }
    removeSnakeTail();
}

/**
 * The following function manages collisions of snake
 */
void manageCollision() {
    if(getSnakeHead()->x < 1 || getSnakeHead()->x > 75){
        isDead = TRUE;
    }

    if(getSnakeHead()->y < 3 || getSnakeHead()->y > 23){
        isDead = TRUE;
    }

    if(getSnakeHead()->x == foodX && getSnakeHead()->y == foodY){
        score++;
        foodX = rand() % 75 + 1;
        foodY = rand() % 21 + 4;
        addSnakeSegmentToEnd(getSnakeTail()->x, getSnakeTail()->y);
    }

    for(int i = 0 ; i < bodyLength ; i++){
        if(i != 0 && get(i)->x == getSnakeHead()->x && get(i)->y == getSnakeHead()->y){
            isDead = TRUE;
        }
    }
}

/**
 * The following function increases the speed of the snake after a specific score is reached
 */
void manageSpeed(){
    if(score <= 5){
        usleep(100000);
    } else if(score <= 10 && score > 5){
        usleep(75000);
    } else if(score <= 15 && score >10){
        usleep(50000);
    } else if(score <= 25 && score > 15){
        usleep(40000);
    } else if(score <= 40 && score > 25 ){
        usleep(30000);
    }
}

// ---------- End of Rendering Functions ----------

#pragma endregion

/**
 * The following function runs input on an another thread
 * 
 * @param unused never used but there just because pthread library requires its parameter this specific way
 * @return void pointer just because pthread library requires its parameter this specific way
 */
void* parallelInput(void * unused){
    while(TRUE){
        processKeyPress();
    }
}

/**
 * Initialize the rendering process
 */
void setup(){
    bodyLength = 0;
    foodX = 30;
    foodY = 15;
    score = 0;
    snakeDirection = 3;
    isDead = FALSE;

    addSnakeSegmentToEnd(15, 15);
    addSnakeSegmentToEnd(16, 15);
    addSnakeSegmentToEnd(17, 15);
    addSnakeSegmentToEnd(18, 15);
}

/**
 * Update the rendering
 */
void update(){
    refreshScreen();
    makeBorders();
    drawFood();
    drawSnake();
    manageDirection();
    manageCollision();
    writeTerminal(string.a);

    reset_string(&string);

    // If dead wait till SPACE key is pressed to restart
    if(isDead){
        char buffer[100];

        refreshScreen();
        makeBorders();
        drawFood();
        drawSnake();
        writeTerminal(string.a);
        setCursorPosition(27 , 12);

        sprintf(buffer , "Game Over : Score => %d" , score);
        append(&string , buffer , strlen(buffer));
        setCursorPosition(24 , 15);

        sprintf(buffer , "Press SPACE to Start new game");
        append(&string , buffer , strlen(buffer));
        writeTerminal(string.a);

        score = 0;

        while(isSpace == FALSE);

        isDead = FALSE;
        isSpace = FALSE;

        removeAll();
        setup();
    }

    manageSpeed();
}

// Driver code
int main(){
    setup();
    enableRawMode();

    pthread_t newThread;
    pthread_create(&newThread, NULL, parallelInput , NULL);

    while(1){
        update();
    }
    return 0;
}
