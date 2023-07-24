/*
 * Import header files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "assign02.pio.h"

/*
 * Define constants && Globals
 */
#define IS_RGBW true  // Will use RGBW format
#define NUM_PIXELS 1  // There is 1 WS2812 device in the chain
#define WS2812_PIN 28 // The GPIO pin that the WS2812 connected to

char *set_input_array;
char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char *alpha_morse[] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
                       "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-",
                       "...-", ".--", "-..-", "-.--", "--.."};
char *num_morse[] = {"-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----."};
char *words[] = {"cave",
                 "copy",
                 "dock",
                 "lick",
                 "run",
                 "owl",
                 "free",
                 "sink",
                 "scold",
                 "hold",
                 "smoke",
                 "part",
                 "vex",
                 "able",
                 "bang",
                 "nose",
                 "tan",
                 "van",
                 "sob",
                 "blue",
                 "nap"};
char *words_morse[] = {
    "-.-. .- ...- .",
    "-.-. --- .--. -.--",
    "-.. --- -.-. -.-",
    ".-.. .. -.-. -.-",
    ".-. ..- -.",
    "--- .-- .-..",
    "..-. .-. . .",
    "... .. -. -.-",
    "... -.-. --- .-.. -..",
    ".... --- .-.. -..",
    "... -- --- -.- .",
    ".--. .- .-. -",
    "...- . -..- ",
    ".- -... .-.. .",
    "-... .- -. --.",
    "-. --- ... .",
    "- .- -.",
    "...- .- -.",
    "... --- -...",
    "-... .-.. ..- .",
    "-. .- .--."};

struct words_hash_table
{ // Hash table to store words and their associated morse code
    char *word;
    char *morse;
};

struct words_hash_table *hashArray[20];
struct words_hash_table *item;

unsigned long hashstring(char word[]) // djb2 hashing algorithm
{
    unsigned long hash = 5381;
    int c;

    while (c = *word++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct words_hash_table *search(char *word)
{
    // get the hash
    int hashIndex = hashstring(word);

    // move in array until an empty
    while (hashArray[hashIndex] != NULL)
    {

        if (hashArray[hashIndex]->word == word)
            return hashArray[hashIndex];

        // go to next cell
        ++hashIndex;

        // wrap around the table
        hashIndex %= 20;
    }

    return NULL;
}

void insert(char *word, char *morse)
{
    struct words_hash_table *item = (struct words_hash_table *)malloc(sizeof(struct words_hash_table));
    item->word = word;
    item->morse = morse;

    int hashIndex = hashstring(word);

    // move in array until an empty cell
    while (hashArray[hashIndex] != NULL)
    {
        // go to next cell
        ++hashIndex;

        // wrap around the table
        hashIndex %= 20;
    }
}

void initialise_hash_table()
{
    for (int i = 0; i < 19; i++)
    {
        insert(words[i], words_morse[i]);
    }
}

absolute_time_t start_time;
int level_number;
int lives;
char level_selection[5];
bool game_status = false;

// Declare the main assembly code entry point.
void main_asm();

/*
 * Responsible for displaying the welcome message
 * Contains the rules and options to choose level
 */
void welcome_message(); // complete

/**
 * @brief Wrapper function used to call the underlying PIO
 *        function that pushes the 32-bit RGB colour value
 *        out to the LED serially using the PIO0 block. The
 *        function does not return until all of the data has
 *        been written out.
 *
 * @param pixel_grb The 32-bit colour value generated by urgb_u32()
 */
static inline void put_pixel(uint32_t pixel_grb);

/**
 * @brief Function to generate an unsigned 32-bit composit GRB
 *        value by combining the individual 8-bit paramaters for
 *        red, green and blue together in the right order.
 *
 * @param r     The 8-bit intensity value for the red component
 * @param g     The 8-bit intensity value for the green component
 * @param b     The 8-bit intensity value for the blue component
 * @return uint32_t Returns the resulting composit 32-bit RGB value
 */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

/*
 * Sets the LED color to indicate the status of the game
 * Blue - Game not in progress
 * Green - Game in Progress; Lives = 3
 * Yellow - Lives = 2
 * Orange - Lives = 1
 * Red - Game Over
 */
void set_rgb(); // complete

/*
 * Loads the level corresponding to the level chosen by the user
 */
void load_level(); // complete

/*
 * Checks the input morse code against the actual morse code of teh given character
 * Returns 1 if correctly matched, 0 otherwise
 */
int check_pattern(int level_number, char *given_char, char *morse_code_input); // complete

/**
 * Function to generate random character for use in levels 1 and 2
 */
char generate_random_character(); // complete

/*
 * Displays the message banner when player wins the game
 */
void game_over_success(); // need to add watchdog timer

/*
 * Displays the message banner for when player loses all its lives
 */
void game_over_failure(); // need to add watchdog timer

/*
 * Subroutine to play level 1 of the game
 */
void level_1(); // complete

/*
 * Subroutine to play level 2 of the game
 */
void level_2(); // complete

/*
 * Subroutine to play level 3 of the game
 */
void level_3(); // nearly complete

/*
 * Subroutine to play level 4 of the game
 */
void level_4(); // nearly complete

/**
 * Subroutine to print stats of the level
 */
void print_level_stats(int num_wins, int num_losses); // complete

/*
 * Main entry point for the code
 */
int main()
{
    stdio_init_all();
    // initialise_hash_table();
    watchdog_enable(9000, 1);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);

    welcome_message();

    main_asm();
    load_level();
    return (0);
}

// Initialise a GPIO pin – see SDK for detail on gpio_init()
void asm_gpio_init(uint pin)
{
    gpio_init(pin);
}

// Set direction of a GPIO pin – see SDK for detail on gpio_set_dir()
void asm_gpio_set_dir(uint pin, bool out)
{
    gpio_set_dir(pin, out);
}

// Get the value of a GPIO pin – see SDK for detail on gpio_get()
bool asm_gpio_get(uint pin)
{
    return gpio_get(pin);
}

// Set the value of a GPIO pin – see SDK for detail on gpio_put()
void asm_gpio_put(uint pin, bool value)
{
    gpio_put(pin, value);
}

// Enable falling-edge interrupt – see SDK for detail on gpio_set_irq_enabled()
void asm_gpio_set_irq(uint pin)
{
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
}

void start_timer()
{
    start_time = get_absolute_time();
}

int end_timer()
{
    int end_time = (int)absolute_time_diff_us(start_time, get_absolute_time());
    return end_time / 100000;
}

void set_input(int case_received)
{
    switch (case_received)
    {
    case 0:
        strcat(set_input_array, ".");
        break;
    case 1:
        strcat(set_input_array, "-");
        break;
    case 2:
        strcat(set_input_array, " ");
        break;
    case 3:
        strcat(set_input_array, "\0");
        break;
    }
}

void welcome_message()
{
    printf("----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
    printf("#       #       #  # # # #  #        # # # #   # # # #  #       #   # # # #\n");
    printf(" #     # #     #   #        #        #         #     #  ##     ##   #      \n");
    printf("  #   #   #   #    # # # #  #        #         #     #  # #   # #   # # # #\n");
    printf("   # #     # #     #        #        #         #     #  #  # #  #   #      \n");
    printf("    #       #      # # # #  # # # #  # # # #   # # # #  #   #   #   # # # #\n");
    printf("Group 0\n----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
}

void game_over_success()
{
    printf("----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
    printf("# # # #      #      #       #  # # # #     # # # #  #       #  # # # #  # # # # \n");
    printf("#           # #     ##     ##  #           #     #   #     #   #        #     # \n");
    printf("#   # #    #   #    # #   # #  # # # #     #     #    #   #    # # # #  # # # # \n");
    printf("#     #   # # # #   #  # #  #  #           #     #     # #     #        #    #  \n");
    printf("# # # #  #       #  #   #   #  # # # #     # # # #      #      # # # #  #     # \n");
    printf("\n");
    printf("#     #  # # # #  #     #    #       #       #  #  #     #  # \n");
    printf(" #   #   #     #  #     #     #     # #     #   #  # #   #  # \n");
    printf("  # #    #     #  #     #      #   #   #   #    #  #  #  #  # \n");
    printf("   #     #     #  #     #       # #     # #     #  #   # #    \n");
    printf("   #     # # # #  # # # #        #       #      #  #     #  # \n");
    printf("----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
}

void game_over_failure()
{
    printf("----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
    printf("# # # #      #      #       #  # # # #     # # # #  #       #  # # # #  # # # # \n");
    printf("#           # #     ##     ##  #           #     #   #     #   #        #     # \n");
    printf("#   # #    #   #    # #   # #  # # # #     #     #    #   #    # # # #  # # # # \n");
    printf("#     #   # # # #   #  # #  #  #           #     #     # #     #        #    #  \n");
    printf("# # # #  #       #  #   #   #  # # # #     # # # #      #      # # # #  #     # \n");
    printf("\n");
    printf("#     #  # # # #  #     #    #        # # # #  # # # #  # # # #  # \n");
    printf(" #   #   #     #  #     #    #        #     #  #        #        # \n");
    printf("  # #    #     #  #     #    #        #     #  # # # #  # # # #  # \n");
    printf("   #     #     #  #     #    #        #     #        #  #          \n");
    printf("   #     # # # #  # # # #    # # # #  # # # #  # # # #  # # # #  # \n");
    printf("----------------------------------------------------------------------------------------------\n----------------------------------------------------------------------------------------------\n");
}

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

void set_rgb()
{

    if (game_status == false)
    {
        // Set LED to BLUE once the game opens but hasnt started
        put_pixel(urgb_u32(0x00, 0x00, 0xFF));
    }
    else
    {
        switch (lives)
        {
        case 3:
            put_pixel(urgb_u32(0x80, 0xFF, 0x00));
            break;

        case 2:
            put_pixel(urgb_u32(0xFF, 0xFF, 0x00));
            break;

        case 1:
            put_pixel(urgb_u32(0xFF, 0x80, 0x00));
            break;

        case 0:
            put_pixel(urgb_u32(0xFF, 0x00, 0x00));
            break;
        }

        printf("You have %d lives left\n", lives);
    }
}

void load_level()
{
    int level_number;

    set_rgb();

    printf("Please choose a level using the corresponding morse code:\n");
    printf("Level 1 ( .---- ) :\tIndividual characters with their equivalent Morse code provided.\n");
    printf("Level 2 ( ..--- ) :\tIndividual characters without their equivalent Morse code provided.\n");
    printf("Level 3 ( ...-- ) :\tIndividual words with their equivalent Morse code provided.\n");
    printf("Level 4 ( ....- ) :\tIndividual words without their equivalent Morse code provided.\n");
    // scanf("%d", level_number);

    
    while (1)
    {
        // set_input_array used in the following code is the global array which stores the user input
        // - name may need to be changed to correspond to the actual variable (as of right now it has not been defined)
        if (strcmp(set_input_array, ".----") == 0)
        {
            printf("Level 1 selected!\n");
            level_number = 1;
            memset(set_input_array, 0, sizeof(set_input_array));
            break;
        }
        else if (strcmp(set_input_array, "..---") == 0)
        {
            printf("Level 2 selected!\n");
            level_number = 2;
            memset(set_input_array, 0, sizeof(set_input_array));
            break;
        }
        else if (strcmp(set_input_array, "...--") == 0)
        {
            printf("Level 3 selected!\n");
            level_number = 3;
            memset(set_input_array, 0, sizeof(set_input_array));
            break;
        }
        else if (strcmp(set_input_array, "....-") == 0)
        {
            printf("Level 4 selected!\n");
            level_number = 4;
            memset(set_input_array, 0, sizeof(set_input_array));
        }
        else
        {
            printf("Invalid input, try again!\n");
            memset(set_input_array, 0, sizeof(set_input_array));
        }
        sleep_ms(2000);
    }

    memset(set_input_array, 0, sizeof(set_input_array));

    switch (level_number)
    {
    case 1:
        level_1();
        break;
    case 2:
        level_2();
        break;
    case 3:
        level_3();
        break;
    case 4:
        level_4();
        break;
    default:
        level_1();
        break;
    }
}

char generate_random_character()
{
    int random_index = random() % 36;
    char random_letter = alphabet[random_index];
    return random_letter;
}

int check_pattern(int level_number, char *given_char, char *morse_code_input)
{
    switch (level_number)
    {
    case 1:
        for (int i = 0; i < sizeof(alpha_morse); i++)
        {
            if (strcmp(alpha_morse[i], given_char) == 0)
            {
                if (strcmp(alpha_morse[i], morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }

        for (int i = 0; i < sizeof(num_morse); i++)
        {
            if (strcmp(num_morse[i], given_char) == 0)
            {
                if (strcmp(num_morse[i], morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        break;
    case 2:
        for (int i = 0; i < sizeof(alpha_morse); i++)
        {
            if (strcmp(alpha_morse[i], given_char) == 0)
            {
                if (strcmp(alpha_morse[i], morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        for (int i = 0; i < sizeof(num_morse); i++)
        {
            if (strcmp(num_morse[i], given_char) == 0)
            {
                if (strcmp(num_morse[i], morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        break;
    case 3:
        for (int i = 0; i < sizeof(words); i++)
        {
            if (strcmp(words[i], given_char) == 0)
            {
                int hashIndex = hashstring(given_char);
                if (strcmp(hashArray[hashIndex]->morse, morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        break;
    case 4:
        for (int i = 0; i < sizeof(words); i++)
        {
            if (strcmp(words[i], given_char) == 0)
            {
                int hashIndex = hashstring(given_char);
                if (strcmp(hashArray[hashIndex]->morse, morse_code_input) == 0)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        break;
    default:
        printf("You have reached the darkness shrouding the void \n");
        return 0;
        break;
    }
    return 0;
}
void level_1()
{
    lives = 3;
    int correct_try_count = 0;
    int fail_count = 0;
    int consecutive_wins = 0;

    game_status = true;
    set_rgb();

    while (1)
    {
        char *morse_value;
        char *given_char = generate_random_character(); // given char is a global variable

        int index_for_morse;
        for (int i = 0; i < sizeof(alpha_morse); i++)
        {
            if (strcmp(given_char, alphabet[i]) == 0)
            {
                index_for_morse = i;
                if (index_for_morse > 25)
                {
                    index_for_morse = index_for_morse - 26;
                    morse_value = num_morse[index_for_morse];
                }
                else
                {
                    morse_value = alpha_morse[index_for_morse];
                }
                break;
            }
        }

        printf("Input the corresponding morse code for the following letter to progress to the next level:\n");
        printf("Letter: %c\n", given_char);
        printf("Morse code: %s\n", morse_value);

        while (1)
        {
            memset(set_input_array, 0, sizeof set_input_array);
            main_asm();
            if (check_pattern(1, morse_value, set_input_array) == 1)
            {
                correct_try_count++;
                consecutive_wins++;
                if (lives < 3)
                {
                    lives++;
                }

                printf("Congratulations, that is correct! You are %i/5 of the way to the next level!\n %i lives remaining\n", consecutive_wins, lives);
                break;
            }
            else
            {
                lives--;
                fail_count++;
                consecutive_wins = 0;
                printf("That is incorrect - Progress Reset - %i lives remaining\n", lives);
            }

            set_rgb();

            if (lives == 0)
            {
                printf("You have run out of lives - Game Over!\n");
                break;
            }
        }

        if (lives == 0)
        {
            print_level_stats(correct_try_count, fail_count);
            game_over_failure();
        }

        if (consecutive_wins == 5)
        {
            printf("You have now completed this level. Moving to level 2.\n");
            print_level_stats(correct_try_count, fail_count);
            level_2();
        }
    }
}

void level_2()
{
    lives = 3;
    int correct_try_count = 0;
    int fail_count = 0;
    int consecutive_wins = 0;

    game_status = true;
    set_rgb();

    while (1)
    {
        char *morse_value;
        char *given_char = generate_random_character(); // given char is a global variable

        int index_for_morse;
        for (int i = 0; i < sizeof(alpha_morse); i++)
        {
            if (strcmp(given_char, alphabet[i]) == 0)
            {
                index_for_morse = i;
                if (index_for_morse > 25)
                {
                    index_for_morse = index_for_morse - 26;
                    morse_value = num_morse[index_for_morse];
                }
                else
                {
                    morse_value = alpha_morse[index_for_morse];
                }
                break;
            }
        }

        printf("Input the corresponding morse code for the following letter to progress to the next level:\n");
        printf("Letter: %c\n", given_char);

        while (1)
        {
            memset(set_input_array, 0, sizeof set_input_array);
            main_asm();
            if (check_pattern(2, morse_value, set_input_array) == 1)
            {
                correct_try_count++;
                consecutive_wins++;

                if (lives < 3)
                {
                    lives++;
                }

                printf("Congratulations, that is correct! You are %i/5 of the way to the next level!\n %i lives remaining\n", consecutive_wins, lives);
                break;
            }
            else
            {
                lives--;
                fail_count++;
                consecutive_wins = 0;
                printf("That is incorrect - Progress Reset - %i lives remaining\n", lives);
            }

            set_rgb();

            if (lives == 0)
            {
                printf("You have run out of lives - Game Over!\n");
                break;
            }
        }

        if (lives == 0)
        {
            print_level_stats(correct_try_count, fail_count);
            game_over_failure();
        }

        if (consecutive_wins == 5)
        {
            printf("You have now completed this level. Moving to level 3.\n");
            print_level_stats(correct_try_count, fail_count);
            level_3();
        }
    }
}

void level_3()
{
    lives = 3;
    int correct_try_count = 0;
    int fail_count = 0;

    game_status = true;
    set_rgb();
    while (1)
    {
        int random_index = random() % 20;

        printf("Input the corresponding morse code for the following word to progress to the next level:\n");
        printf("Word: %c\n", words[random_index]);
        printf("Morse code: %s\n", words_morse[random_index]);

        while (1)
        {
            memset(set_input_array, 0, sizeof set_input_array);
            if (check_pattern(3, words_morse[random_index], set_input_array) == 1) // need to change these inputs since it's level 3
            {
                correct_try_count++;

                if (lives < 3)
                {
                    lives++;
                }

                printf("Congratulations, that is correct! You are %i/5 of the way to the next level!\n %i lives remaining\n", correct_try_count, lives);
                break;
            }
            else
            {
                lives--;
                fail_count++;
                printf("That is incorrect - %i lives remaining\n", lives);
            }

            set_rgb();

            if (lives == 0)
            {
                printf("You have run out of lives - Game Over!\n");
                break;
            }
        }

        if (lives == 0)
        {
            print_level_stats(correct_try_count, fail_count);
            game_over_failure();
        }

        if (correct_try_count == 5)
        {
            printf("You have now completed this level. Moving to level 2.\n");
            print_level_stats(correct_try_count, fail_count);
            level_4();
        }
    }
}

void level_4()
{
    lives = 3;
    int correct_try_count = 0;
    int fail_count = 0;

    game_status = true;
    set_rgb();

    while (1)
    {
        int random_index = random() % 20;

        printf("Input the corresponding morse code for the following word to progress to the next level:\n");
        printf("Word: %c\n", words[random_index]);

        while (1)
        {
            memset(set_input_array, 0, sizeof set_input_array);
            main_asm();
            if (check_pattern(4, words_morse[random_index], set_input_array) == 1) // need to change these inputs since it's level 4
            {
                correct_try_count++;

                if (lives < 3)
                {
                    lives++;
                }

                printf("Congratulations, that is correct! You are %i/5 of the way to the next level!\n %i lives remaining\n", correct_try_count, lives);
                break;
            }
            else
            {
                lives--;
                fail_count++;
                printf("That is incorrect - %i lives remaining\n", lives);
            }

            set_rgb();

            if (lives == 0)
            {
                printf("You have run out of lives - Game Over!\n");
                break;
            }
        }

        if (lives == 0)
        {
            print_level_stats(correct_try_count, fail_count);
            game_over_failure();
        }

        if (correct_try_count == 5)
        {
            printf("You have now completed this level. Moving to level 2.\n");
            print_level_stats(correct_try_count, fail_count);
            game_over_success();
        }
    }
}

void print_level_stats(int num_wins, int num_losses)
{
    int win_percentage = num_wins / (num_wins + num_losses);
    printf("Total number of attempts: %i", num_wins + num_losses);
    printf("Number of successful attempts: %i\n", num_wins);
    printf("Number of failed attempts: %i\n", num_losses);
    printf("Success Rate: %i%% ", win_percentage);
}
