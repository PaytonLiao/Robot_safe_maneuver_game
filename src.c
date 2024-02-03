// Operating system used: WSL (Windows subsystem for linux)
#include <ncurses.h>
#include <time.h>
#include <stdlib.h>

// Defining struct which is used to store the coordinates of the danger location
typedef struct
{
    int x;
    int y;
} danger_coordinates;

// Define the proportions of the window's dimentions with respect to the dimensions of the terminal
#define ter2wincols 3 / 4
#define ter2winrows 4 / 5

// Basically mvadd but the colour is specified
void display_coloured_character(int x, int y, char ch, int colour_code);

// Draw a rectangular game window in white
void draw_boundary(int tercols, int terrows, int Rrange[4], int centrexy[2]);

// Introduction animation
void intro (int centrexy[2], int Rpos[2]);

// Reset the position of the robot to the centre of the window
void Rpos_reset(int tercols, int terrows, int Rpos[2]);

// Move the robot in accordance to the current arrow direction
void robot_navigation(int Rrange[4], int Rpos[2], int erased_pos[2], char *arrowptr, int *livesptr, int *levelptr, int *scoreptr, int *chptr, int *speed_delayptr, int colour_mode, int crazy_mode);

// The move_{direction}_handler functions make sure the trace of the robot is cleaned and the body length stays constant
void move_left_handler(int erased_pos[2], int Rpos[2], int colour_mode);
void move_right_handler(int erased_pos[2], int Rpos[2], int colour_mode);
void move_up_handler(int erased_pos[2], int Rpos[2], int colour_mode);
void move_down_handler(int erased_pos[2], int Rpos[2], int colour_mode);

// Randomly generate a pair of coordinates that is not taken by either the robot, danger locations, the person, Big Mac, or CRAZY characters.
void random_position_generator(int Rrange[4], int tercols, int terrows, int *x, int *y, int Rpos[2], danger_coordinates *danger_coordinates_ptr, int danger_coordinates_size, int generated_pos_BM[2], int crazy_pos[2]);

// Check if a pair of coordinates is taken by danger locations, delete the danger location when the variable delete = 1
int isCoordinateInList(danger_coordinates *tracked_danger_coordinates, int size, int targetX, int targetY, int delete);

// Handle the pausing feature: press q once to enter the pausing mode, enter q again to quit the game or enter c to continue the game with a count down of three seconds.
void pausing(int *ch, int speed_delay, int *quit);

// Display ending information
void outro (int centrexy[2], int Rpos[2], int Rrange[2], int level);


int main()
{
    // Initialize ncurses
    initscr();
    start_color();

    // Inirtialise random number generator, should only be called once.
    srand(time(NULL));

    // Allow the player to control using arrow keys
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    // Get the size of the terminal window
    int terrows, tercols;
    getmaxyx(stdscr, terrows, tercols);
    // Initialise an array to store the position of the robot
    int Rpos[2];
    // Initialise an array to store the boundary of the game window
    int Rrange[4];
    // Initialise an array to store the center of the window
    int centrexy[2];

    // Define color pairs
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);  // Pair 1: Yellow on black
    init_pair(2, COLOR_MAGENTA, COLOR_BLACK); // Pair 2 Magenta on black
    init_pair(3, COLOR_CYAN, COLOR_BLACK);    // Pair 3: Cyan on black
    init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Pair 4: Green on black
    init_pair(5, COLOR_YELLOW, COLOR_RED);    // Pair 5: Yellow on red
    init_pair(6, COLOR_CYAN, COLOR_WHITE);    // Pair 6: Cyan on white
    init_pair(7, COLOR_WHITE, COLOR_BLACK);   // Pair 7: White on black
    init_pair(8, COLOR_CYAN, COLOR_YELLOW);   // Pair 8: Cyan on Yellow

    // Drawing game boundary
    draw_boundary(tercols, terrows, Rrange, centrexy);

    // Introduction
    // Press any key to start the game
    intro (centrexy, Rpos);

    // Initialise arrow and direction
    char arrow = '>';
    char *arrowptr = &arrow;

    // initialise variables needed for processing the game
    int lives = 3;
    int level = 0;
    int fifth_of_level = 0;
    int score = 0;
    int ch;
    int speed_delay = 80;
    int *livesptr = &lives;
    int *levelptr = &level;
    int *fifth_of_levelptr = &fifth_of_level;
    int *scoreptr = &score;
    int *chptr = &ch;
    int *speed_delayptr = &speed_delay;

    // Set crazy mode condition
    char crazy[] = "CRAZY";
    int crazy_mode = 0;
    int crazy_length = 5;
    int crazy_word_num = 0;
    int crazy_pos[2] = {0, 0};
    int crazy_time_length = 10000;
    int crazy_time_left = 0;
    int crazy_speed_delay = 20;
    for (int i = 0; i < crazy_length; i++)
    {
        display_coloured_character(0, Rrange[1] - 10 + i * 2, crazy[i], 3);
    }

    // Track the path of the robot so that the trace can be deleted and keep the length of the robot constant
    int erased_pos[2];
    // Track the position of the person to be rescued
    int generated_pos_person[2];
    // Track the position of the Big Mac
    int generated_pos_BM[2] = {0, 0};
    // Track the position of the danger location using the danger_coordinates struct
    danger_coordinates *danger_coordinates_ptr = NULL;
    int danger_coordinates_size = 2;
    danger_coordinates_ptr = (danger_coordinates *)malloc(danger_coordinates_size * sizeof(danger_coordinates));

    // Display game information at the top and bottom of the window
    attron(COLOR_PAIR(1));
    mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", score, level, lives);
    mvprintw(Rrange[3] + 1, Rrange[1] - 25, "Press q to enter to pause");
    mvprintw(Rrange[3] + 2, Rrange[1] - 30, "Press q again to quit the game");
    mvprintw(Rrange[3] + 3, Rrange[1] - 19, "Press c to continue");
    attron(COLOR_PAIR(4));
    mvprintw(0, 1, "Rescue $!                               ");

    // Initialise the position of the first person to be rescued
    random_position_generator(Rrange, tercols, terrows, &generated_pos_person[0], &generated_pos_person[1], Rpos, danger_coordinates_ptr, danger_coordinates_size, generated_pos_BM, crazy_pos);
    display_coloured_character(generated_pos_person[1], generated_pos_person[0], '$', 4);

    // Start a while loop to move the robot at constant speed until the player runs out of lives or the q key is pressed
    int quit = 0;
    int *quitptr = &quit;

    // Start the game
    while (lives > 0 && quit == 0)
    {
        // Turn on non-blocking mode
        timeout(0);
        // Check if the robot is in CRAZY mode and move the robot by one step in the direction of the current arrow
        if (crazy_mode == 1)
        {
            robot_navigation(Rrange, Rpos, erased_pos, arrowptr, livesptr, levelptr, scoreptr, chptr, speed_delayptr, 6, crazy_mode);
        }
        else
        {
            robot_navigation(Rrange, Rpos, erased_pos, arrowptr, livesptr, levelptr, scoreptr, chptr, speed_delayptr, 7, crazy_mode);
        }


        // Checking if the robot rescues a person
        if (generated_pos_person[0] == Rpos[0] && generated_pos_person[1] == Rpos[1])
        {
            // Displays rescue information at the top of the window
            attron(COLOR_PAIR(4));
            mvprintw(0, 1, "One Person Rescued! Score += 10!        ");
            // Update the score and fifth_of_level
            score += 10;
            fifth_of_level += 1;
            // Generate the position of the next person to be rescued
            random_position_generator(Rrange, tercols, terrows, &generated_pos_person[0], &generated_pos_person[1], Rpos, danger_coordinates_ptr, danger_coordinates_size, generated_pos_BM, crazy_pos);
            display_coloured_character(generated_pos_person[1], generated_pos_person[0], '$', 4);
            // Increase the speed of the robot when fifth_of_level is a multiple of 5 and produce a Big Mac if there isn't currently one
            if (fifth_of_level == 5)
            {
                // Displays level up information at the top of the window
                attron(COLOR_PAIR(1));
                mvprintw(0, 1, "Level Up!                               ");
                // Reset fifth_of_leve and update level and speed_delay
                fifth_of_level = 0;
                level += 1;
                speed_delay -= 5;
                // Generate a Big Mac if there is not currently a Big Mac
                if (generated_pos_BM[0] == 0 && generated_pos_BM[1] == 0)
                {
                    random_position_generator(Rrange, tercols, terrows, &generated_pos_BM[0], &generated_pos_BM[1], Rpos, danger_coordinates_ptr, danger_coordinates_size, generated_pos_BM, crazy_pos);
                    display_coloured_character(generated_pos_BM[1], generated_pos_BM[0], 'M', 5);
                }
            }
            // Displays updated game information at the top of the window
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", score, level, lives);
            // Genereate two danger locations
            for (int i = 2; i > 0; i--)
            {
                random_position_generator(Rrange, tercols, terrows, &(danger_coordinates_ptr[danger_coordinates_size - i].x), &(danger_coordinates_ptr[danger_coordinates_size - i].y), Rpos, danger_coordinates_ptr, danger_coordinates_size, generated_pos_BM, crazy_pos);
                display_coloured_character(danger_coordinates_ptr[danger_coordinates_size - i].y, danger_coordinates_ptr[danger_coordinates_size - i].x, '#', 2);
                refresh();
            }
            // Update the danger_coordinates data
            danger_coordinates_size += 2;
            danger_coordinates_ptr = (danger_coordinates *)realloc(danger_coordinates_ptr, (danger_coordinates_size) * sizeof(danger_coordinates));
            // Generate a CRAZY character if there are current no crazy character and crazy mode is not on
            if (crazy_pos[0] == 0 && crazy_pos[1] == 0 && crazy_mode == 0)
            {
                random_position_generator(Rrange, tercols, terrows, &crazy_pos[0], &crazy_pos[1], Rpos, danger_coordinates_ptr, danger_coordinates_size, generated_pos_BM, crazy_pos);
                display_coloured_character(crazy_pos[1], crazy_pos[0], crazy[crazy_word_num], 3);
            }
            refresh();
        }


        // Lose a life when the robot hits a danger location
        else if (isCoordinateInList(danger_coordinates_ptr, danger_coordinates_size, Rpos[0], Rpos[1], 1) == 1 && crazy_mode == 0)
        {
            attron(COLOR_PAIR(2));
            mvprintw(0, 1, "Oh No You Hit An Obstacle:/             ");
            lives--;
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", score, level, lives);
            // Reset the position of the robot to centre
            Rpos_reset(tercols, terrows, Rpos);
        }


        // Gain two lives when the robot eats a Big Mac
        else if (generated_pos_BM[0] == Rpos[0] && generated_pos_BM[1] == Rpos[1])
        {
            attron(COLOR_PAIR(5));
            mvprintw(0, 1, "Yum! Big Mac Is The Best!               ");
            generated_pos_BM[0] = 0;
            generated_pos_BM[1] = 0;
            lives += 2;
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", score, level, lives);
        }


        // Light up the crazy character on the top right if the robot hits a crazy character
        else if (crazy_pos[0] == Rpos[0] && crazy_pos[1] == Rpos[1])
        {
            display_coloured_character(0, Rrange[1] - 10 + crazy_word_num * 2, crazy[crazy_word_num], 6);
            crazy_pos[0] = 0;
            crazy_pos[1] = 0;
            // Point crazy_word_num to the next CRAZY character
            crazy_word_num++;
            // Enter CRAZY mode when all five characters are collected
            if (crazy_word_num == 5)
            {
                attron(COLOR_PAIR(6));
                mvprintw(0, 1, "C R A Z Y   M O D E!!!!!!!!!!!!!!!!!!!!!");
                // Reset crazy_word_num and turn on CRAZY mode
                crazy_word_num = 0;
                crazy_mode = 1;
                // Initialise the reamaining CRAZY time
                crazy_time_left = crazy_time_length / crazy_speed_delay;
                // Increase the speed to CRAZY Mode speed 
                speed_delayptr = &crazy_speed_delay;
            }
        }
        
        
        // Count down CRAZY time
        if (crazy_time_left > 0)
        {
            crazy_time_left--;
            // Shine the crazy word on the top right
            if (crazy_time_left % 2 == 0)
            {
                for (int i = 0; i < crazy_length; i++)
                {
                    display_coloured_character(0, Rrange[1] - 10 + i * 2, crazy[i], 3);
                }
            }
            else
            {
                for (int i = 0; i < crazy_length; i++)
                {
                    display_coloured_character(0, Rrange[1] - 10 + i * 2, crazy[i], 6);
                }
            }
            // If crazy time run out, reset crazy mode = 0
            if (crazy_time_left <= 0)
            {
                attron(COLOR_PAIR(4));
                mvprintw(0, 1, "Rescue Person $!                        ");
                crazy_time_left = 0;
                crazy_mode = 0;
                // Reduce the speed to normal
                speed_delayptr = &speed_delay;
            }
        }
        
        
        // Call the pausing function when q is pressed
        if (ch == 113)
        {
            pausing(chptr, speed_delay, quitptr);
        }
    }
    timeout(-1);

    // Refresh the screen
    refresh();

    // Game Ending: display player's score and easter egg
    outro(centrexy, Rpos, Rrange, level);

    // Wait for a key press before exiting
    getch();

    // Free the dynamic allocated memory
    free(danger_coordinates_ptr);

    // End ncurses
    endwin();

    return 0;
}





// display_coloured_character print the desired character in the desired colour
void display_coloured_character(int y, int x, char ch, int colour_code)
{
    // Set desired color pair
    attron(COLOR_PAIR(colour_code));
    mvaddch(y, x, ch);
    // Set color pair 1 (original)
    attron(COLOR_PAIR(1));
}

// draw_broundary takes the dimensions of the terminal screen and adjusts to the desired window size according to the drfined proportion
// The function then seta the innitial position of the R character and store it in Rpo which is an integer array with a length of 2
// The funciton also stores the range of which the R character is allowed to access
void draw_boundary(int tercols, int terrows, int Rrange[4], int centrexy[2])
{
    // Drawing game boundary
    float wincols = tercols * ter2wincols;
    float winrows = terrows * ter2winrows;
    // printw("%f", wincols);
    // printw("%f", winrows);
    for (int i = 2; i < (int)wincols; i++)
    {
        mvaddch(1, i, ACS_HLINE);
        mvaddch(winrows, i, ACS_HLINE);
    }
    for (int i = 2; i < (int)winrows; i++)
    {
        mvaddch(i, 1, ACS_VLINE);
        mvaddch(i, wincols, ACS_VLINE);
    }
    mvaddch(1, 1, ACS_ULCORNER);
    mvaddch(1, wincols, ACS_URCORNER);
    mvaddch(winrows, 1, ACS_LLCORNER);
    mvaddch(winrows, wincols, ACS_LRCORNER);

    // Calculate the centre position
    centrexy[0] = wincols / 2;
    centrexy[1] = winrows / 2;

    // Stroing the range of R
    Rrange[0] = 1;
    Rrange[1] = wincols;
    Rrange[2] = 1;
    Rrange[3] = winrows;
    // Display game information at the bottom of the game window
    attron(COLOR_PAIR(7));
    mvprintw(Rrange[3] + 2, Rrange[0], "<@ = Robot");
    // Set color pair 4
    attron(COLOR_PAIR(4));
    mvprintw(Rrange[3] + 3, Rrange[0], "$ = Person to be rescued");
    // Set color pair 2
    attron(COLOR_PAIR(2));
    mvprintw(Rrange[3] + 4, Rrange[0], "# = Obstacle");
    // Set color pair 3
    attron(COLOR_PAIR(6));
    mvprintw(Rrange[3] + 5, Rrange[0], "<@ = The robot goes CRAZY just like getting the star in super mario!");
    // Set color pair 5
    attron(COLOR_PAIR(5));
    mvprintw(Rrange[3] + 6, Rrange[0], "M = Big Mac that gives you two extra lives");
    return;
}

void intro (int centrexy[2], int Rpos[2]){
    // Press any key to start the game
    attron(COLOR_PAIR(1));
    mvprintw(centrexy[1], centrexy[0] - 16, "Press any key to start the game!");
    getch();
    Rpos[0] = centrexy[0] - 17;
    Rpos[1] = centrexy[1];
    timeout(0);
    while (Rpos[0] != centrexy[0] + 17)
    {
        Rpos[0]++;
        
        napms(50);
        getch();
        int erased_pos[2] = {Rpos[0]-1, Rpos[1]};
        mvaddch(erased_pos[1], erased_pos[0], ' ');
        display_coloured_character(Rpos[1], Rpos[0], '>', 7);
        erased_pos[0] = Rpos[0];
        erased_pos[1] = Rpos[1];
    }
    timeout(-1);
}

void Rpos_reset(int tercols, int terrows, int Rpos[2])
{
    float wincols = tercols * ter2wincols;
    float winrows = terrows * ter2winrows;
    // Calculate the centre position
    int x = wincols / 2;
    int y = winrows / 2;

    // Clear the arrow at its last position
    mvaddch(Rpos[1], Rpos[0], ' ');

    // Storing the initial position of R
    Rpos[0] = x;
    Rpos[1] = y;
}

void robot_navigation(int Rrange[4], int Rpos[2], int erased_pos[2], char *arrow, int *lives, int *level, int *score, int *ch, int *speed_delay, int colour_mode, int crzay_mode)
{
    napms(*speed_delay);
    *ch = getch();
    // if ESC is pressed, end the program
    if (*ch == 113)
    {
        return;
    }

    switch (*ch)
    {
    case KEY_LEFT:
        *arrow = '<';
        break;
    case KEY_RIGHT:
        *arrow = '>';
        break;
    case KEY_UP:
        *arrow = '^';
        break;
    case KEY_DOWN:
        *arrow = 'v';
        break;
    default:
        break;
    }
    if (*arrow == '<')
    {
        if (Rpos[0] > Rrange[0] + 1)
        {
            move_left_handler(erased_pos, Rpos, colour_mode);
        }
        else
        {
            *arrow = '>';
            move_right_handler(erased_pos, Rpos, colour_mode);
            if (crzay_mode == 0)
            {
                (*lives)--;
                attron(COLOR_PAIR(2));
                mvprintw(0, 1, "Oh No You Hit The Wall:/                ");
            }
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", *score, *level, *lives);
        }
    }
    else if (*arrow == '>')
    {
        if (Rpos[0] < Rrange[1] - 1)
        {
            move_right_handler(erased_pos, Rpos, colour_mode);
        }
        else
        {
            *arrow = '<';
            move_left_handler(erased_pos, Rpos, colour_mode);
            if (crzay_mode == 0)
            {
                (*lives)--;
                attron(COLOR_PAIR(2));
                mvprintw(0, 1, "Oh No You Hit The Wall:/                ");
            }
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", *score, *level, *lives);
        }
    }
    else if (*arrow == '^')
    {
        if (Rpos[1] > Rrange[2] + 1)
        {
            move_up_handler(erased_pos, Rpos, colour_mode);
        }
        else
        {
            *arrow = 'v';
            move_down_handler(erased_pos, Rpos, colour_mode);
            if (crzay_mode == 0)
            {
                (*lives)--;
                attron(COLOR_PAIR(2));
                mvprintw(0, 1, "Oh No You Hit The Wall:/                ");
            }
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", *score, *level, *lives);
        }
    }
    else if (*arrow == 'v')
    {
        if (Rpos[1] < Rrange[3] - 1)
        {
            move_down_handler(erased_pos, Rpos, colour_mode);
        }
        else
        {
            *arrow = '^';
            move_up_handler(erased_pos, Rpos, colour_mode);
            if (crzay_mode == 0)
            {
                (*lives)--;
                attron(COLOR_PAIR(2));
                mvprintw(0, 1, "Oh No You Hit The Wall:/                ");
            }
            attron(COLOR_PAIR(1));
            mvprintw(Rrange[3] + 1, Rrange[0], "Score: %d     Level: %d      Lives: %d", *score, *level, *lives);
        }
    }
    display_coloured_character(Rpos[1], Rpos[0], *arrow, colour_mode);
}

// The move_{direction}_handler functions keep track of the position of the arrow and the body, and make sure the trace of the body is deleted
void move_left_handler(int erased_pos[2], int Rpos[2], int colour_mode)
{
    mvaddch(erased_pos[1], erased_pos[0], ' ');
    display_coloured_character(Rpos[1], Rpos[0], '@', colour_mode);
    erased_pos[0] = Rpos[0];
    erased_pos[1] = Rpos[1];
    Rpos[0]--;
}

void move_right_handler(int erased_pos[2], int Rpos[2], int colour_mode)
{
    mvaddch(erased_pos[1], erased_pos[0], ' ');
    display_coloured_character(Rpos[1], Rpos[0], '@', colour_mode);
    erased_pos[0] = Rpos[0];
    erased_pos[1] = Rpos[1];
    Rpos[0]++;
}

void move_up_handler(int erased_pos[2], int Rpos[2], int colour_mode)
{
    mvaddch(erased_pos[1], erased_pos[0], ' ');
    display_coloured_character(Rpos[1], Rpos[0], '@', colour_mode);
    erased_pos[0] = Rpos[0];
    erased_pos[1] = Rpos[1];
    Rpos[1]--;
}

void move_down_handler(int erased_pos[2], int Rpos[2], int colour_mode)
{
    mvaddch(erased_pos[1], erased_pos[0], ' ');
    display_coloured_character(Rpos[1], Rpos[0], '@', colour_mode);
    erased_pos[0] = Rpos[0];
    erased_pos[1] = Rpos[1];
    Rpos[1]++;
}

void random_position_generator(int Rrange[4], int tercols, int terrows, int *x, int *y, int Rpos[2], danger_coordinates *danger_coordinates_ptr, int danger_coordinates_size, int generated_pos_BM[2], int crazy_pos[2])
{

    // The do while loop makes sure that the cooridinates generated is valid i.e. it is not too close to the current position of the arrow/it is not one of the danger locations that already exist/it is the the coordinate of the person
    int centrexy[2];
    Rpos_reset(tercols, terrows, centrexy);
    do
    {
        *x = rand();
        *y = rand();
        // Adjust the value of r to be between the size of the window
        *x = *x % (Rrange[1] - Rrange[0] - 1) + Rrange[0] + 1;
        *y = *y % (Rrange[3] - Rrange[2] - 1) + Rrange[2] + 1;
    } while ((abs(*x - Rpos[0]) < 3 && abs(*y - Rpos[1]) < 3) && (*x != centrexy[0] && *y != centrexy[1]) && (isCoordinateInList(danger_coordinates_ptr, danger_coordinates_size, *x, *y, 0) == 0) && (*x != generated_pos_BM[0] && *y != generated_pos_BM[1]) && (*x != crazy_pos[0] && *y != crazy_pos[1]));
}

int isCoordinateInList(danger_coordinates *tracked_danger_coordinates, int size, int targetX, int targetY, int delete)
{
    for (int i = 0; i < size; ++i)
    {
        if (tracked_danger_coordinates[i].x == targetX && tracked_danger_coordinates[i].y == targetY)
        {
            if (delete == 1)
            {
                tracked_danger_coordinates[i].x = 0;
                tracked_danger_coordinates[i].y = 0;
            }
            return 1; // Found the coordinate in the list
        }
    }
    return 0; // Coordinate not found in the list
}

void pausing(int *ch, int speed_delay, int *quit)
{
    attron(COLOR_PAIR(1));
    mvprintw(0, 1, "Game paused!                            ");
    timeout(-1);
    *ch = 0;
    while (*ch != 99 && *ch != 113)
    {
        *ch = getch();
        if (*ch == 113)
        {
            *quit = 1;
        }
    }
    timeout(0);
    if (*ch == 99)
    {
        for (float f = 3; f > 0; f -= 0.01)
        {
            napms(10);
            getch();
            attron(COLOR_PAIR(1));
            mvprintw(0, 1, "Game starting in %.2fsec                     ", f);
        }
        attron(COLOR_PAIR(1));
        mvprintw(0, 1, "Game continued!                         ");
        napms(speed_delay);
    }
}

void outro (int centrexy[2], int Rpos[2], int Rrange[2], int level){
    attron(COLOR_PAIR(1));
    mvprintw(centrexy[1], centrexy[0] - 6, "Game Over :(");
    mvprintw(centrexy[1]+1, centrexy[0] - 29, "Even if you only got to level %d I am still proud of you!", level); 
    Rpos[0] = Rrange[0]+1;
    Rpos[1] = Rrange[3]-1;
    char easter_egg[] = "A legend called Payton Liao once reached Level 21005153";
    int easter_egg_writer_pos = 0;
    timeout(0);
    while (easter_egg_writer_pos < 55)
    {
        Rpos[0]++;
        napms(50);
        getch();
        int erased_pos[2] = {Rpos[0]-1, Rpos[1]};
        display_coloured_character(erased_pos[1], erased_pos[0], easter_egg[easter_egg_writer_pos++], 8);
        display_coloured_character(Rpos[1], Rpos[0], '~', 8);
        erased_pos[0] = Rpos[0];
        erased_pos[1] = Rpos[1];
    }
    timeout(-1);
    mvprintw(centrexy[1]+2, centrexy[0] - 16, "Press any key to leave the game"); 
}