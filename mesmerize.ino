/* mesmerize.ino
 * A complete implementation of the memory game as described in the
 * 19COA202 coursework specification. This implementation extends system
 * functionality beyond the minimal feature set.
 * 
 * Author - Joseph Marcus Tungate
 * Last Modified - 05/04/2020. 
 */
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_MCP23017.h>
#include <EEPROM.h>

/* Uncomment the following comment to enable debug messages via. the serial port.
 * Note that using debug features significantly decreases available dynamic memory 
 * by approximately 12%, program storage by 6%, and reduces the overall responsiveness 
 * of the game which may make timing based aspects of the gameplay behave incorrectly.
 */
#define DEBUG

#ifdef DEBUG
bool DEBUG_ENABLED = true;
#endif

/* The number of Symbols in the Arrow Symbol set.
 * Symbol arrays which are passed to arrow_createSymbolSet should be this length.
 */
#define ARROW_SYMBOL_SET_LENGTH 4

#define DISP_TOP 0
#define DISP_BOTTOM 1
#define DISP_CENTRE 7

//The addess in EEPROM where the random seed is stored.
#define EEP_SEED_ADDRESS 0
//The address in EEPROM where the high score table is stored.
#define EEP_HIGH_SCORE_TABLE_ADDRESS 4
//The number of entries in the high score table.
#define EEP_HIGH_SCORE_TABLE_SIZE 10
//The number of characters in an alias which are stored in EEPROM.
#define EEP_ALIAS_LENGTH 4

//Minimum and maximum values for GameProfile and RoundProfile fields.
#define GAMEP_ROUND_COUNT_MIN 1
#define GAMEP_ROUND_COUNT_MAX 100
#define GAMEP_ROUND_COUNT_INF 1000
#define GAMEP_LIFE_COUNT_MIN 1
#define GAMEP_LIFE_COUNT_MAX 10
#define ROUNDP_SEQUENCE_LENGTH_MIN 4
#define ROUNDP_SEQUENCE_LENGTH_MAX 15
#define ROUNDP_UNIQUE_SYMBOLS_MIN 2
#define ROUNDP_UNIQUE_SYMBOLS_MAX 4
#define ROUNDP_DISPLAY_TIME_MIN 500
#define ROUNDP_DISPLAY_TIME_MAX 2500
#define ROUNDP_INPUT_TIME_MIN 500
#define ROUNDP_INPUT_TIME_MAX 3000
//The length of the symbols array in RoundProfile.
#define ROUNDP_SYMBOLS_LENGTH 4

/* Each tile (custom character) in the Arrow tileset as ordered by index.
 * The names mostly refer to the tile's meaning when rendering an upwards pointing arrow.
 * For a better idea of what each tile represents, see arrow_createTileset()
 */
enum ArrowTileset{
   VERT_LEFT_STEM,
   VERT_RIGHT_STEM,
   HORIZ_BOT_STEM,
   HORIZ_TOP_STEM,
   TOP_LEFT_HEAD,
   TOP_RIGHT_HEAD,
   BOT_LEFT_HEAD,
   BOT_RIGHT_HEAD
};

/* All possible byte patterns, and their meanings, that are returned
 * by disp_buttonPress and disp_timedButtonPress.
 */
enum disp_Button{
  NONE_BUTTON = 0b00000000,
  SELECT_BUTTON = 0b00000001,
  RIGHT_BUTTON = 0b00000010,
  DOWN_BUTTON = 0b00000100,
  UP_BUTTON = 0b00001000,
  LEFT_BUTTON = 0b00010000
};

/* The colours available to lcd.setBacklight and functions which accept
 * backlight values. Ordered by their numeric values. CURRENT simply
 * maintains the current state of the backlight.
 */
enum RGBColour{
  NONE,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  VIOLET,
  TEAL,
  WHITE,
  CURRENT
}; 

/* Different text alignments available for disp_printStr.
 * CURRENT_POS keeps the cursor in the position it was when
 * disp_printStr last finished executing.
 */
enum TextAlign{
  LEFT,
  CENTRE,
  RIGHT,
  CURRENT_POS
};


//typedefs. Mostly for shortening struct names.
typedef struct gameProfile GameProfile;
typedef Adafruit_RGBLCDShield LCDShield;
typedef struct roundProfile RoundProfile;
typedef struct symbol Symbol;
typedef struct highScore HighScore;


//struct definitions.
/* A Symbol is a 2x2 image as made from tiles (custom characters) in a tileset.
 * Symbols are used for rendering 'complex' images to the lcd display. 
 * Symbols will not render properly unless disp_loadTileset is first called with the Symbol's tileset.
 * Size: 5 bytes. (18/04/2020)
 */
struct symbol{
  /* Each byte holds the index of the desired tile within the tile's tileset.
   * byte is the appropriate datatype here as every tileset has exactly eight tiles.
   */
  byte topLeft;
  byte topRight;
  byte bottomLeft;
  byte bottomRight;
  //A character representation of the Symbol. Useful for comparing Symbols.
  char value;
};

/* A RoundProfile holds data for controlling the gameplay of a round of the memory game.
 * By modifying a RoundProfile, the round can be made easier or harder.
 * Size: 33 bytes. (18/04/2020)
 */
struct roundProfile{
  //The length of the round's random sequence.
  unsigned short sequenceLength;
  //All Symbols that can be used in this round's sequence.
  Symbol symbols[ROUNDP_SYMBOLS_LENGTH];
  //The number of Symbols in symbols to use in the round's sequence.
  unsigned short uniqueSymbols;
  /* The function which is pointed to should take a byte, as returned by
   *  disp_timedButtonPress or disp_buttonPress, and map it to the value
   *  field of an appropriate Symbol in symbols.
   */
  char (*inputToSymbolValue)(byte);
  //The length of time, in milliseconds, that a single Symbol is displayed on screen.
  unsigned int displayTime;
  //The length of time, in milliseconds, that the user has to make an input.
  unsigned int inputTime;
  //The player's as of starting the round score.
  unsigned long score;
};

/* A GameProfile holds data for controlling the gameplay of an entire game of the memory game.
 * Differing GameProfiles are used to implement different game modes.
 * Size: 50 bytes. (30/04/2020)
 */
struct gameProfile{
  //Defines the gameplay style of the next round to be played.
  RoundProfile rProfile;
  /* The function pointed to by this field is called between rounds to change the contents of rProfile.
   * This function should be used to implement game mode difficulties.
   */
  void (*changeRoundComplexity)(RoundProfile*, unsigned int);
  //Determines whether changeRoundComplexity is called between rounds.
  bool isComplexityChanging;
  //The number of rounds in this game.
  unsigned short roundCount;
  //The number of lives the player starts the game with.
  unsigned short lifeCount;
  //The title of the game.
  char gameTitle[10];  
};

/* A HighScore combines a player score with their alias, primarily for EEPROM functions.
 * Size: 8 bytes. (18/04/2020)
 */
struct highScore {
  unsigned long score;
  char alias[EEP_ALIAS_LENGTH];
};


//Function prototypes.
void arrow_createDownSymbol(Symbol *destination);
void arrow_createLeftSymbol(Symbol *destination);
void arrow_createRightSymbol(Symbol *destination);
void arrow_createUpSymbol(Symbol *destination);
void arrow_createSymbolSet(Symbol destination[]);
void arrow_createBiPromptSymbol(Symbol *destination);
void arrow_createDownPromptSymbol(Symbol *destination);
void arrow_createUpPromptSymbol(Symbol *destination);
void arrow_createTileset(char destination[][8]);

void delayByPoll(unsigned long duration);

byte disp_buttonPress(LCDShield lcd);
char disp_characterSelector(char startingChar, LCDShield lcd);
void disp_countdown(char message[], LCDShield lcd);
void disp_cycleColours(unsigned int totalDuration, unsigned int colourDuration, LCDShield lcd);
void disp_gameLost(unsigned long score, LCDShield lcd);
void disp_gameWon(unsigned long score, unsigned int bonusPoints, LCDShield lcd);
void disp_incorrectInput(Symbol* correctSymbolp, LCDShield lcd);
void disp_intro(LCDShield lcd);
void disp_loadTileset(byte tileset[][8], LCDShield lcd);
void disp_printStr(char message[], unsigned short line, TextAlign alignment, RGBColour backlight, bool clearScreen, LCDShield lcd);
void disp_roundCount(unsigned int currentRound, unsigned int totalRounds, LCDShield lcd);
void disp_roundWon(char message[], unsigned int bonusPoints, LCDShield lcd);
void disp_score(unsigned long score, unsigned int scoreIncrement, LCDShield lcd);
void disp_sequence(const Symbol *sequence[], unsigned int sequenceLength, unsigned int displayTime, LCDShield lcd);
void disp_symbol(const Symbol *symbolp, unsigned short topLeftPos, LCDShield lcd);
void disp_time(unsigned short milliseconds, LCDShield lcd);
byte disp_timedButtonPress(unsigned int duration, unsigned int *timeTakenp, LCDShield lcd);
void disp_timeout(LCDShield lcd);
void disp_title(char title[], char subtitle[], RGBColour backlight, LCDShield lcd);

bool eep_addHighScore(HighScore *hsp);
void eep_initialiseHighScoreTable();
bool eep_isHighScore(unsigned long score);
char eep_readChar(unsigned int address);
HighScore eep_readHighScore(unsigned int tableIndex);
void eep_readHighScoreTable(HighScore destination[]);
unsigned long eep_readSeed();
unsigned long eep_readULong(unsigned int address);
void eep_sortHighScoreTable();
void eep_writeChar(char data, unsigned int index);
void eep_writeHighScore(HighScore *hsp, unsigned int tableIndex);
void eep_writeHighScoreTable(HighScore highScoreTable[]);
void eep_writeSeed(unsigned int seed);
void eep_writeULong(unsigned long data, unsigned int index);

void chooseRandomSymbols(const Symbol symbols[], Symbol* destination[], unsigned int symbolsLength, unsigned int destinationLength);
void generateSymbolSequence(Symbol *symbolps[], Symbol *destination[], unsigned int symbolpsLength, unsigned int destinationLength);
void getAlias(char destination[], LCDShield lcd);
char inputToArrowSymbol(byte input);
char mirrorInputToArrowSymbol(byte input);
unsigned int inputTimeToScore(unsigned int inputTime);

void randomRoundComplexity(RoundProfile *profile, unsigned int roundNumber);
void initPracticeGameProfile(GameProfile* destination);
void initEasyGameProfile(GameProfile* destination);
void easyRoundComplexity(RoundProfile *profile, unsigned int roundNumber);
void initMediumGameProfile(GameProfile* destination);
void mediumRoundComplexity(RoundProfile *profile, unsigned int roundNumber);
void initHardGameProfile(GameProfile* destination);
void hardRoundComplexity(RoundProfile *profile, unsigned int roundNumber);
void initMirrorGameProfile(GameProfile *destination);

void runConfigurationMenu(GameProfile* profile, LCDShield lcd);
void runGame(GameProfile* profile, LCDShield lcd);
void runLeaderboard(LCDShield lcd);
void runMenu(LCDShield lcd);
bool runRound(RoundProfile *profile, LCDShield lcd);


//Setup and loop
void setup() {
  //The LCDShield to be used as the UI.
  Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
  lcd.begin(16, 2);
  
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("--Mesmerize Serial Debugger--");
  #endif

  //Read the seed from EEPROM to avoid starting with the same seed every boot.
  randomSeed(eep_readSeed());
  //Load the arrow tileset into the lcd so that arrow symbols can be drawn.
  byte tileset[8][8];
  arrow_createTileset(tileset);
  disp_loadTileset(tileset, lcd);

  //Display the intro and run the system.
  disp_intro(lcd);
  runMenu(lcd);
}

//loop isn't needed in this program.
void loop() {
}

//Function definitions.
/* arrow functions are used for loading the arrow tileset
 * and its Symbols in memory. The arrow tile and symbol set is the
 * only tile and symbol set implemented in this version of mesmerize,
 * but the system is built in a way such that arbitrary
 * tilesets and symbols can be readily implemented if there
 * exists the storage to support them. 
 */
/* Copies into destination the Symbol data for a down arrow. 
 * destination - A pointer to the Symbol into which the data is written.
 */
void arrow_createDownSymbol(Symbol *destination){
  destination->topLeft = VERT_LEFT_STEM;
  destination->topRight = VERT_RIGHT_STEM;
  destination->bottomLeft = BOT_LEFT_HEAD;
  destination->bottomRight = BOT_RIGHT_HEAD;
  destination->value = 'd';
}

/* Copies into destination the Symbol data for a left arrow. 
 * destination - A pointer to the Symbol into which the data is written.
 */
void arrow_createLeftSymbol(Symbol *destination){
  destination->topLeft = TOP_LEFT_HEAD;
  destination->topRight = HORIZ_TOP_STEM;
  destination->bottomLeft = BOT_LEFT_HEAD;
  destination->bottomRight = HORIZ_BOT_STEM;
  destination->value = 'l';
}

/* Copies into destination the Symbol data for a right arrow. 
 * destination - A pointer to the Symbol into which the data is written.
 */
void arrow_createRightSymbol(Symbol *destination){
  destination->topLeft = HORIZ_TOP_STEM;
  destination->topRight = TOP_RIGHT_HEAD;
  destination->bottomLeft = HORIZ_BOT_STEM;
  destination->bottomRight = BOT_RIGHT_HEAD;
  destination->value = 'r';
}

/* Copies into destination the Symbol data for a up arrow. 
 * destination - A pointer to the Symbol into which the data is written.
 */
void arrow_createUpSymbol(Symbol *destination){
  destination->topLeft = TOP_LEFT_HEAD;
  destination->topRight = TOP_RIGHT_HEAD;
  destination->bottomLeft = VERT_LEFT_STEM;
  destination->bottomRight = VERT_RIGHT_STEM;
  destination->value = 'u';
}

/* Copies into destination the up, down, left, and right arrow Symbols.
 * destination - An array of at least ARROW_SYMBOL_SET_LENGTH 
 *               into which the arrow symbol set is copied.
 */
void arrow_createSymbolSet(Symbol destination[]){
  arrow_createUpSymbol(&destination[0]);
  arrow_createLeftSymbol(&destination[1]);
  arrow_createDownSymbol(&destination[2]);
  arrow_createRightSymbol(&destination[3]);
}

/* Copies into destination the Symbol data for a bi-directional prompt.
 * destination - A pointer to the Symbol into which the data is copied.
 */
void arrow_createBiPromptSymbol(Symbol *destination){
  destination->topLeft = BOT_RIGHT_HEAD;
  destination->topRight = ' ';
  destination->bottomLeft = TOP_RIGHT_HEAD;
  destination->bottomRight = ' ';
  destination->value = 'b';
}

/* Copies into destination the Symbol data for a down prompt.
 * destination - A pointer to the Symbol into which the data is copied.
 */
void arrow_createDownPromptSymbol(Symbol *destination){
  destination->topLeft = ' ';
  destination->topRight = ' ';
  destination->bottomLeft = TOP_RIGHT_HEAD;
  destination->bottomRight = ' ';
  destination->value = 'd';
}

/* Copies into destination the Symbol data for an up prompt.
 * destination - A pointer to the Symbol into which the data is copied.
 */
void arrow_createUpPromptSymbol(Symbol *destination){
  destination->topLeft = BOT_RIGHT_HEAD;
  destination->topRight = ' ';
  destination->bottomLeft = ' ';
  destination->bottomRight = ' ';
  destination->value = 'u';
}

/* Copies into destination the tileset necessary to draw arrow symbols.
 * destination - The 2D byte array into which the tileset is copied. 
 */
void arrow_createTileset(byte destination[][8]){
  const byte arrowTiles[][8] = {
   {
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011
  },

  {
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000
  },
  
  {
    0b11111,
    0b11111,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
  },
  
  {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b11111
  },

  {
    0b00000,
    0b00000,
    0b00000,
    0b00001,
    0b00011,
    0b00111,
    0b01111,
    0b11111
  },

  {
    0b00000,
    0b00000,
    0b00000,
    0b10000,
    0b11000,
    0b11100,
    0b11110,
    0b11111
  },

  {
    0b11111,
    0b01111,
    0b00111,
    0b00011,
    0b00001,
    0b00000,
    0b00000,
    0b00000
  },

  {
    0b11111,
    0b11110,
    0b11100,
    0b11000,
    0b10000,
    0b00000,
    0b00000,
    0b00000
  }};

  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      destination[i][j] = arrowTiles[i][j];
    }
  }
}

/* Halts program execution for duration milliseconds.
 * duration - The length of time, in milliseconds, that the program should halt execution.
 */
void delayByPoll(unsigned long duration){
  unsigned long endTime = millis() + duration;

  while(millis() < endTime)
  ;
}

/* disp functions provide an interface between the lcd and the rest of the program. 
 * The idea is that disp functions allow the program to use the lcd without having to call 
 * lots of the lower level lcd and string functions which often obfuscate the 'main idea' of the program. 
 * Character arrays have been chosen to represent strings over String for the sake of space.
 */
/* Waits for the user to make a button press and returns a byte representing their input.
 * lcd - The LCDShield from which to read the button press
 * Returns - A byte representing the button press. Each bit in the byte is a flag for
 *           each button on the display. Use enum disp_buttons to compare values.
 */
byte disp_buttonPress(LCDShield lcd){
  //Use this static variable to prevent multiple readings from a single press.
  static bool buttonDown = false;
  byte input;

  while(true){
    input = lcd.readButtons();

    if(buttonDown == true && input == NONE_BUTTON)
      buttonDown = false;

    else if(buttonDown == false && input){
      buttonDown = true;
      break;
    }
  }

  return input;
}

/* Displays an alphanumeric character selector which users navigate with the LCDShield buttons.
 * Returns the character as selected by the user.
 * startingChar - The character at which to start the selector, must be alphanumeric.
 * lcd - The LCDShield to use as the UI.
 * Returns - The character selected by the user.
 */
char disp_characterSelector(char startingChar, LCDShield lcd){
  char currentChar = 0;
  char previousChar;
  char nextChar;
  bool isCharSelected = false;
  byte input;

  char bufferStr[3];

  //Ensures the selector starts on a valid character.
  if(startingChar >= '0' && startingChar <= '9' ||
     startingChar >= 'a' && startingChar <= 'z' || 
     startingChar >= 'A' && startingChar <= 'Z') {

    currentChar = startingChar; 

    //Addresses edge cases.
    while(!isCharSelected){
      switch(currentChar){
        case 'A':
          previousChar = '9';
          nextChar = 'B';
          break;
          
        case 'Z':
          previousChar = 'Y';
          nextChar = 'a';
          break;
    
        case 'a':
          previousChar = 'Z';
          nextChar = 'b';
          break;
    
        case 'z':
          previousChar = 'y';
          nextChar = '0';
          break;
    
        case '0':
          previousChar = 'z';
          nextChar = '1';
          break;
    
        case '9':
          previousChar = '8';
          nextChar = 'A';
          break;
    
        default:
          previousChar = currentChar - 1;
          nextChar = currentChar + 1;
          break;
      }

      sprintf(bufferStr, "<%c", previousChar);
      disp_printStr(bufferStr, DISP_BOTTOM, LEFT, CURRENT, false, lcd);
      sprintf(bufferStr, "%c", currentChar);
      disp_printStr(bufferStr, DISP_BOTTOM, CENTRE, CURRENT, false, lcd);
      sprintf(bufferStr, "%c>", nextChar);
      disp_printStr(bufferStr, DISP_BOTTOM, RIGHT, CURRENT, false, lcd);
  
      input = disp_buttonPress(lcd);
  
      switch(input){
        case LEFT_BUTTON:
          currentChar = previousChar;
          break;
  
        case RIGHT_BUTTON:
          currentChar = nextChar;
          break;
  
        case SELECT_BUTTON:
          isCharSelected = true;
          break;
      } 
    }
  }
  return currentChar;
}

/* Renders a countdown animation to the screen.
 * message - The message with which the countdown is titled.
 * lcd - The LCDShield to use as the UI.
 */
void disp_countdown(char message[], LCDShield lcd){  
  disp_printStr(message, DISP_TOP, CENTRE, RED, true, lcd);
  disp_printStr("03", DISP_BOTTOM, CENTRE, RED, false, lcd);
  delayByPoll(300);
  disp_printStr("02", DISP_BOTTOM, CENTRE, YELLOW, false, lcd);
  delayByPoll(300);
  disp_printStr("01", DISP_BOTTOM, CENTRE, GREEN, false, lcd);
  delayByPoll(300);
  disp_printStr("GO", DISP_BOTTOM, CENTRE, WHITE, false, lcd);
  delayByPoll(200);
  lcd.clear();  
}

/* Cycles lcd's backlight through all available colours for totalDuration milliseconds
 * where the colour changes every colourDuration milliseconds.
 * totalDuration - The length of time, in milliseconds, to cycle the backlight.
 * colourDuration - The length of time, in milliseconds, for which a single colour will be displayed during the cycle.
 * lcd - The LCDShield upon which to perform this function. 
 */
void disp_cycleColours(unsigned int totalDuration, unsigned int colourDuration, LCDShield lcd){
  const unsigned long ENDTIME = millis() + totalDuration;
  RGBColour colour = RED;

  while(millis() < ENDTIME){
    if(colour > WHITE)
      colour = RED;

    lcd.setBacklight(colour);
    delayByPoll(colourDuration);
    colour = (int)colour + 1;
  }
}

/* Displays a screen which notifies the player that the game is lost.
 * score - The score the player earned this game.
 * lcd - The LCDShield to use as the UI.
 */
void disp_gameLost(unsigned long score, LCDShield lcd){
  char scoreStr[11];
  sprintf(scoreStr, "%lu", score);
  
  disp_printStr("GAME OVER", 0, CENTRE, RED, true, lcd);
  delayByPoll(2000);
  disp_printStr("Score", 0, CENTRE, RED, true, lcd);
  disp_printStr(scoreStr, 1, CENTRE, RED, false, lcd);
  delayByPoll(4000);
}

/* Displays a screen which notifies the player that the game is won.
 * score - The score that the player earnt this round. 
 * bonusPoints - Extra points that the user recieved for winning the game.
 * lcd - The LCDShield to use as the UI.
 */
void disp_gameWon(unsigned long score, unsigned int bonusPoints, LCDShield lcd){
  char bufferStr[11];
    
  disp_printStr("WINNER", 0, CENTRE, NONE, true, lcd);
  disp_cycleColours(2000, 100, lcd);
    
    if(bonusPoints > 0){
      sprintf(bufferStr, "+%u", bonusPoints); 
      disp_printStr(bufferStr, 1, CENTRE, GREEN, false, lcd);
      delayByPoll(1000);
    }
    sprintf(bufferStr, "%lu", score);
    disp_printStr("Score", 0, CENTRE, GREEN, true, lcd);
    disp_printStr(bufferStr, 1, CENTRE, GREEN, false, lcd);
    delayByPoll(4000);
}

/* Displays a screen which notifies the player that their repeat of the round's sequence was incorrect.
 * correctSymbolp - A pointer to the Symbol which the player should have entered.
 * lcd - The LCDShield upon which to perform this function.
 */
void disp_incorrectInput(Symbol* correctSymbolp, LCDShield lcd){
  disp_printStr("Correct Symbol", 0, CENTRE, RED, true, lcd);
  delayByPoll(1000);
  lcd.clear();
  disp_symbol(correctSymbolp, 7, lcd);
  delayByPoll(2000);
}

/* Displays an introductory screen. Shown when the system first powers on.
 * lcd - The LCDShield to use as a UI.
 */
void disp_intro(LCDShield lcd){
  disp_printStr("MESMERIZE", 0, CENTRE, WHITE, true, lcd);
  disp_cycleColours(2000, 200, lcd);
  lcd.setBacklight(WHITE);
  delayByPoll(1000);

  
}

/* 'Loads' tileset into the lcd's custom character data.
 *  Must be ussd before any Symbols made from tileset can be rendered.
 *  tileset - An 8x8 multidimensional byte array which contains the tile data.
 *  lcd - The LCDShield upon which this function is performed.
 */
void disp_loadTileset(byte tileset[][8], LCDShield lcd){
  for(int i = 0; i < 8; i++){
    lcd.createChar(i, tileset[i]);
  }
}

/* Displays a screen which notifies the player that they have achieved a high score.
 * highScore - The player's score.
 * lcd - The LCDShield to use as the UI.
 */
void disp_newHighscore(unsigned long highscore, LCDShield lcd){
  char strBuffer[11];
  sprintf(strBuffer, "%lu", highscore);
  
  disp_printStr("NEW HIGH SCORE", 0, CENTRE, WHITE, true, lcd);
  disp_printStr(strBuffer, 1, CENTRE, WHITE, false, lcd);
  disp_cycleColours(3000, 100, lcd);
}

/*A general use function for displaying strings (char[]) on the lcd.
 * message - The string to be displayed on the screen. Should not be longer than 16 characters.
 * line - The number of the line on which to print the message. Can be 0 or 1.
 *        Use the DISP_TOP, and DISP_BOTTOM macros for readability.
 * alignment - Specifies whether message is left, centre, right justified or if the
 *             cursor shouldn't be moved. 
 * backlight - The RGBColour to set the lcd backlight.
 * lcd - The LCDShield to use as the UI.
 */
void disp_printStr(char message[], unsigned short line, TextAlign alignment, RGBColour backlight, bool clearScreen, LCDShield lcd){
  unsigned short pos = 0;
  
  switch(alignment){
    case LEFT:
      pos = 0;
      break;

    case CENTRE:
      pos = 8 - (strlen(message) + 1) / 2;
      break;

    case RIGHT:
      pos = 16 - strlen(message);
      break;

    case CURRENT_POS:
      break;
      
    default:
      pos = 0;
      break;
  }
  
  if(clearScreen) lcd.clear();
  if(backlight != CURRENT) lcd.setBacklight(backlight);
  if(alignment != CURRENT_POS) lcd.setCursor(pos, line);
  lcd.print(message);  
}

/* Displays a screen which notifies the user of how many rounds they have played and have left to play.
 * currentRound - The number of the current round.
 * totalRounds - The number of rounds to be played this game.
 * lcd - The LCDShield to use as the UI.
 */
void disp_roundCount(unsigned short currentRound, unsigned short totalRounds, LCDShield lcd){
  char bufferStr[17];
  if(totalRounds == GAMEP_ROUND_COUNT_INF)
    sprintf(bufferStr, "Round: %hu/%c", currentRound, 0b11110011);
  else
    sprintf(bufferStr, "Round: %hu/%hu", currentRound, totalRounds);
  disp_printStr(bufferStr, 0, CENTRE, WHITE, true, lcd);
  delayByPoll(1000);
}

/* Displays a screen which notifies the player that the round is won.
 * message - A message to display on the screen. Should be a review of their performance.
 * bonusPoints - The number of points the player gets for completing the round.
 * lcd - The LCDShield to use as the UI.
 */
void disp_roundWon(char message[], unsigned int bonusPoints, LCDShield lcd){  
  disp_printStr(message, DISP_TOP, CENTRE, GREEN, true, lcd);
  
  if(bonusPoints > 0){
    char strBuffer[12];
    sprintf(strBuffer, "+%u", bonusPoints);
    disp_printStr(strBuffer, DISP_BOTTOM, CENTRE, CURRENT, false, lcd);
  }
       
  delayByPoll(1500);
}

/* Displays the player's score and its most recent increment.
 * Also changes the backlight to correspond with the size of increment.
 * Used to build up the UI during the input stage of a round.
 * totalScore - The player's total score up until this point in the game.
 * increment - The most recent increase to the player's points.
 * lcd - The LCDShield to use as the UI.
 */
void disp_score(unsigned long totalScore, unsigned int increment, LCDShield lcd){
  RGBColour colour;
  char bufferStr[12];
  switch(increment){
    case 100:
      colour = WHITE;
      break;

    case 125:
      colour = GREEN;
      break;
      
    case 150:
      colour = TEAL;
      break;

    case 200:
      colour = VIOLET;
      break;

    default:
      colour = WHITE;
  }

  sprintf(bufferStr, "%lu", totalScore);
  disp_printStr(bufferStr, DISP_TOP, LEFT, colour, false, lcd);

  sprintf(bufferStr, "+%u", increment);
  disp_printStr(bufferStr, DISP_BOTTOM, LEFT, CURRENT, false, lcd);
}

/* Displays the Symbols as referenced in sequence such that each Symbol is displayed for displayTime ms.
 * sequence - An array of pointers to Symbols to be drawn. The sequence is drawn in order.
 * sequenceLength - The length of sequence.
 * displayTime - The duration in milliseconds for which a single Symbol in sequence is displayed.
 * lcd - The LCDShield upon which this function is performed.
 */
void disp_sequence(Symbol *sequence[], unsigned int sequenceLength, unsigned int displayTime, LCDShield lcd){
  lcd.clear();
  
  for(int i = 0; i < sequenceLength; i++){
    disp_symbol(sequence[i], DISP_CENTRE, lcd);
    delayByPoll(displayTime);
    lcd.clear();
    //Used to create a brief empty screen between Symbols. Needed if the same Symbol appears consecutively.
    delayByPoll(150);
  }
}

/* Displays a Symbol on the display with its top left corner at x coordinate topLeftPos.
 * Takes aprox. 33ms (down from 254ms) to execute on Arduino Uno.
 * symbolp - A pointer to the Symbol to be displayed on the screen.
 * topLeftPos - The x-coordinate of the top left of the Symbol.
 * lcd - The LCDScreen upon which this function is performed.
 */
void disp_symbol(const Symbol *symbolp, unsigned short topLeftPos, Adafruit_RGBLCDShield lcd){  
  lcd.setCursor(topLeftPos, DISP_TOP);
  lcd.write(symbolp->topLeft);
  lcd.write(symbolp->topRight);
  lcd.setCursor(topLeftPos, DISP_BOTTOM);
  lcd.write(symbolp->bottomLeft);
  lcd.write(symbolp->bottomRight);
}

/* Displays a time, as given in milliseconds, on the screen in seconds.
 * Used to build up the UI during the input stage of the round.
 * milliseconds - The time, in milliseconds, to be displayed.
 * lcd - The LCDShield upon which this function is performed.
 */
void disp_time(unsigned short milliseconds, Adafruit_RGBLCDShield lcd){
  lcd.setCursor(12, 0);
  lcd.print(milliseconds / 1000.0);
}

/* Waits for the user to make a directional button press for the given duration and returns a byte representing their input.
 * Draws a timer to the top right of the display and stores the length of time it took the user to make an input in the variable pointed to by timeTakenp.
 * If no button press is made during the duration, BUTTON_NONE is returned.
 * duration - The length of time the user has to make an directional input.
 * timeTakenp - A pointer to a variable where the time taken to make an input is stored in milliseconds.
 * lcd - The LCDShield from which to read the button press and display the timer.
 * Returns - A byte representing the button press. Each bit in the byte is a flag for
 *           each button on the display. Use enum disp_buttons to compare values.
 */
byte disp_timedButtonPress(unsigned int duration, unsigned int *timeTakenp, Adafruit_RGBLCDShield lcd){
  const unsigned long START_TIME = millis();
  const unsigned long END_TIME = START_TIME + duration;
  unsigned long nextDisplay = START_TIME;
  unsigned long currentTime;
  byte input;
  const byte DIRECTION_MASK = 0b00011110;
  //Used to ensure that only button presses are read, not the holding down of buttons.
  static bool buttonDown = false;

  //Continues to read the buttons until the time runs out or the user presses a button.
  while((currentTime = millis()) < END_TIME){
    input = lcd.readButtons();

   //Draw the timer every 100ms.
    if(currentTime >= nextDisplay){
      nextDisplay += 100;
      disp_time(END_TIME - currentTime, lcd);
    }
    
    if(buttonDown == true && input == NONE_BUTTON)
      buttonDown = false;

    else if(buttonDown == false && (input & DIRECTION_MASK)){
      buttonDown = true;
      break;
    }
  }
    
  //If the user ran out of time, return 0 to indicate time out.
  if(currentTime >= END_TIME)
    input = NONE_BUTTON;

  *timeTakenp = currentTime - START_TIME;
  
  return input;
}

/* Displays a screen which notifies the player that they have ran out of time to make an input.
 * lcd - The LCDShield to use as the UI. 
 */
void disp_timeout(Adafruit_RGBLCDShield lcd){
  disp_printStr("TIME OUT", 0, CENTRE, YELLOW, true, lcd);
  delayByPoll(1000);
}

/* A general use function for displaying title cards.
 * Used primarily for displaying the titles of games before they begin.
 * title - The title of the title card.
 * subtitle - The subtitle of the title card.
 * backlight - The RGBValue that the lcd backlight should take.
 * lcd - The LCDShield upon which this function is performed.
 */
void disp_title(char title[], char subtitle[], RGBColour backlight, LCDShield lcd){
  disp_printStr(title, DISP_TOP, LEFT, backlight, true, lcd);
  delayByPoll(1000);
  disp_printStr(subtitle, DISP_BOTTOM, RIGHT, CURRENT, false, lcd);
  delayByPoll(2000);
}

/* eep functions simplify the reading, writing, and organisation of EEPROM.
 * In this program, EEPROM is divided into two distinct sectors, the seed
 * and the high score table. 
 * 
 * The seed sector is used for storing the value of  the random seed during power off. 
 * This is necessary to prevent the same 'random' numbers being generated.
 * 
 * The high score table sector is used for maintaining a record of the top ten
 * high scores. The high scores are stored such that they are always in descending
 * order by score.
 */
/* Checks whether the score is in fact a high score and, if so, then writes it
 * to the high score table at the particular . Returns whether or not the given score was stored.
 * hsp - A pointer to a HighScore to be written to the high score table.
 * Returns - true if the high score was added to the table. false otherwise.
 */
bool eep_addHighScore(HighScore *hsp){
  bool isAdded = eep_isHighScore(hsp->score);

  if(isAdded){
    //Overwrite the lowest high score in the table.
    eep_writeHighScore(hsp, EEP_HIGH_SCORE_TABLE_SIZE - 1);
    eep_sortHighScoreTable();
  }
  return isAdded;
}

/* Initialises the high score table in EEPROM to dummy scores.
 * Called when the system first turns on.
 * Should only be called to reset the high score table.
 */
void eep_initialiseHighScoreTable(){
  HighScore dummyHS = {6000, "JMT"};

  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE; i++){
    eep_writeHighScore(&dummyHS, i);
    dummyHS.score -= 500;
  }
}

/* Returns whether the given score is a high score by comparing it with the high score table.
 * score - The score which is to be compared with the contents of the high score table.
 * returns - true if score is a highscore. false otherwise.
 */
bool eep_isHighScore(unsigned long score){
  HighScore highScoreTable[EEP_HIGH_SCORE_TABLE_SIZE];
  bool isHighScore = false;
  
  eep_readHighScoreTable(highScoreTable);

  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE && !isHighScore; i++){
    if(highScoreTable[i].score < score)
      isHighScore = true;
  }

  return isHighScore;
}

/* Reads the character at address and then returns it.
 * address - The address of the char to be read. Must be between 0 and 1023.
 * Returns - The character at address. 0 if address is invalid.
 */
char eep_readChar(unsigned int address){
  unsigned long charBuffer = 0;

  if(address <= EEPROM.length() - sizeof(char)){
    for(int i = sizeof(char) - 1; i >= 0; i--){
      //Right shift to make room for the next reading.
      charBuffer = charBuffer << 8;
      //Read the next 8 most significant bits of the char into the byte.
      byte byteBuffer = EEPROM.read(address + i);
      //Write those bits into the 8 least significant bits of charBuffer.
      charBuffer += byteBuffer;
    }
  }
  return charBuffer;
}

/* Reads the HighScore at the table index tableIndex and returns it.
 * tableIndex - The position in the high score table where the high score is located.
 *              Must be between 0 and 9.
 * Returns - The HighScore located at tableIndex.
 *           An unitialised HighScore if tableIndex is invalid.
 */
HighScore eep_readHighScore(unsigned int tableIndex){
  HighScore bufferHS;
  
  if(tableIndex < EEP_HIGH_SCORE_TABLE_SIZE){
    unsigned int actualAddress = EEP_HIGH_SCORE_TABLE_ADDRESS + sizeof(HighScore) * tableIndex;
    bufferHS.score = eep_readULong(actualAddress);

    for(int i = 0; i < EEP_ALIAS_LENGTH; i++){
      bufferHS.alias[i] = eep_readChar(actualAddress + sizeof(unsigned long) + i * sizeof(char));
    }
  }
  
  return bufferHS;
}

/* Reads and returns the entire high score table as an array of HighScores.
 * destination - An array of HighScores to write the high score table. 
 */
void eep_readHighScoreTable(HighScore destination[]){
  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE; i++){
    destination[i] = eep_readHighScore(i);
  }
}

/* Read and returns the random seed from EEPROM.
 * Should be used to set the set the random seed of the system
 * when the system turns on.
 */
unsigned long eep_readSeed(){
  return eep_readULong(EEP_SEED_ADDRESS);
}

/* Reads an unsigned long at address from EEPROM.
 * address - The location in EEPROM to read the unsigned long from.
 *           Should be between 0 - 1020.
 * Returns - The unsigned long at address in EEPROM.
 *           0 if address is invalid.
 */
unsigned long eep_readULong(unsigned int address){
  unsigned long longBuffer = 0;
  
  if(address <= EEPROM.length() - sizeof(unsigned long)){
    for(int i = sizeof(unsigned long) - 1; i >= 0; i--){
      //Right shift to make room for the next reading.
      longBuffer = longBuffer << 8;
      //Read the next 8 most significant bits of the long into a byte.
      byte byteBuffer = EEPROM.read(address + i);
      //Write those bits to the 8 least significant bits of longBuffer.
      longBuffer += byteBuffer;
    }
  }
  return longBuffer;
}

/* Sorts the high score table in descending order by score.
 */
void eep_sortHighScoreTable(){
  HighScore highScoreTable[EEP_HIGH_SCORE_TABLE_SIZE];
  HighScore *highScoreTablep[EEP_HIGH_SCORE_TABLE_SIZE];

  eep_readHighScoreTable(highScoreTable);

  //Sorting is done on these pointers to the HighScores
  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE; i++)
    highScoreTablep[i] = &highScoreTable[i];

  //Sort highScoreTablep by the score fields by insertion sort.
  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE - 1; i++){
    byte maxIndex = i;
    for(int j = i; j < EEP_HIGH_SCORE_TABLE_SIZE; j++){
      if(highScoreTablep[j]->score > highScoreTablep[maxIndex]->score)
        maxIndex = j;
    }
    HighScore *temp = highScoreTablep[i];
    highScoreTablep[i] = highScoreTablep[maxIndex];
    highScoreTablep[maxIndex] = temp; 
  }

  //Write the HighScores back into EEPROM by using the sorted pointers.
  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE; i++)
    eep_writeHighScore(highScoreTablep[i], i);   
}

/* Writes a char, data, to EEPROM at address.
 * address muat be between 0 and 1023. 
 */
void eep_writeChar(char data, unsigned int address){

  if(address <= EEPROM.length() - sizeof(char)){
    for(int i = 0; i < sizeof(char); i++){
      //Store the next 8 least significant bits of the char in a byte.
      byte byteBuffer = (byte)(data & 0xff);
      //Write that byte to the EEPROM.
      EEPROM.update(address + i, byteBuffer);
      //Right shift data so that it is ready for masking.
      data = data >> 8;
    }
  }
}

/* Writes a HighScore, as pointed to by hsp, to the high score table in EEPROM at tableIndex.
 * hsp - A pointer to the HighScore which is to be written to memeory.
 * tableIndex - The position in the high score table to write the HighScore.
 *              Must be between 0 and EEP_HIGH_SCORE_TABLE_SIZE - 1.
 */
void eep_writeHighScore(HighScore *hsp, unsigned int tableIndex){
  
  if(tableIndex < EEP_HIGH_SCORE_TABLE_SIZE){
    unsigned int actualIndex = EEP_HIGH_SCORE_TABLE_ADDRESS + tableIndex * sizeof(HighScore);
    eep_writeULong(hsp->score, actualIndex);

    for(int i = 0; i < EEP_ALIAS_LENGTH; i++){
      eep_writeChar(hsp->alias[i], actualIndex + sizeof(unsigned long) + i * sizeof(char));
    }
  }
}

/* Stores the array of HighScores in EEPROM as the high score table.
 * table - An array of HighScores of EEP_HIGH_SCORE_TABLE_SIZE length
 *         to be written into EEPROM.
 */
void eep_writeHighScoreTable(HighScore table[]){
  for(int i = 0; i < EEP_HIGH_SCORE_TABLE_SIZE; i++)
    eep_writeHighScore(&table[i], i);
}

/* Writes the random seed into EEPROM.
 * seed - The random seed to write into EEPROM.
 */
void eep_writeSeed(unsigned long seed){
  eep_writeULong(seed, EEP_SEED_ADDRESS);
}

/* Writes an unsigned long into EEPROM at the given address.
 * data - The unsigned long to write into EEPROM.
 * address - The address in EEPROM where data will written.
 *           Must be between 0 and 1020.
 */
void eep_writeULong(unsigned long data, unsigned int address){ 
  if(address <= EEPROM.length() - sizeof(unsigned long)){
    for(int i = 0; i < sizeof(unsigned long); i++){
      //Store the next 8 least significant bits of the long in a byte.
      byte byteBuffer = (byte)(data & 0xff);
      //Write that byte to the EEPROM.
      EEPROM.update(address + i, byteBuffer);
      //Right shift data so that it is ready for masking.
      data = data >> 8;
    }
  }
}

/* The following functions are used for implementing the logic
 * of the memory game and the memory game system. 
 */
/* Randomly chooses destinationLength Symbols from symbols and stores pointers to these randomly chosen symbols in destination.
 * symbols - An array of Symbols from which destinationLength random Symbols are to be chosen.
 * destination - An array of Symbol pointers where the randomly chosen Symbols' references will be stored.
 * symbolsLength - length of symbols.
 * destinationLength - length of destination.
 */
void chooseRandomSymbols(const Symbol symbols[], Symbol* destination[], unsigned int symbolsLength, unsigned int destinationLength){
  //An array of pointers to the contents of symbols.
  Symbol *symbolReferences[symbolsLength];
  for(int i = 0; i < symbolsLength; i++){
    symbolReferences[i] = &symbols[i];
  }

  //Swap the first destinationLength elements of symbolReferences with random elements from symbolReferences.
  for(int i = 0; i < destinationLength; i++){
    int randomIndex = random(0, symbolsLength);
    Symbol *temp = symbolReferences[randomIndex];
    symbolReferences[randomIndex] = symbolReferences[i];
    symbolReferences[i] = temp;
  }
  
  for(int i = 0; i < destinationLength; i++){
    destination[i] = symbolReferences[i];
  }
}

/* Generates a randomly ordered array of Symbol pointers, from symbols, of length sequenceLength and stores them in sequence.
 * At least one of each Symbol pointer in symbols will be in the sequenece, hence symbolsLength <= sequenceLength.
 * symbols - An array of pointers to Symbols from which the sequence is to be made.
 * sequence - The array where the randomly generated array is to be stored.
 * symbolsLength - The length of symbols.
 * sequenceLength - The length of sequence.
 */
void generateSymbolSequence(Symbol* symbols[], Symbol* sequence[], unsigned int symbolsLength, unsigned int sequenceLength){
  bool containsAllSymbols = false;

  while(!containsAllSymbols){
    for(int i = 0; i < sequenceLength; i++)
      sequence[i] = symbols[random(0, symbolsLength)];

    //Check that at least one of each symbol appears in the sequence. 
    containsAllSymbols = true;

    for(int i = 0; i < symbolsLength && containsAllSymbols; i++){
      bool symbolFound = false;
      
      for(int j = 0; j < sequenceLength && !symbolFound; j++){
        if(symbols[i] == sequence[j])
          symbolFound = true;
      }
  
      if(!symbolFound)
        containsAllSymbols = false;
    }
  }
}

/* Gets the player's alias and stores it in destination for entry in the high score table.
 * destinatioon - A pointer to a character array of length EEP_ALIAS_LENGTH to store the alias.
 * lcd - An LCDShield to use as the UI.
 */
void getAlias(char destination[], LCDShield lcd){
  char selectedChar = 'A';

  disp_printStr("Enter 3", DISP_TOP, CENTRE, WHITE, true, lcd);
  disp_printStr("letter alias", DISP_BOTTOM, CENTRE, CURRENT, false, lcd);

  delayByPoll(2000);
  lcd.clear();
  
  for(int i = 0; i < EEP_ALIAS_LENGTH - 1; i++){
    selectedChar = disp_characterSelector(selectedChar, lcd);
    destination[i] = selectedChar;
    destination[i + 1] = '\0';

    disp_printStr(destination, DISP_TOP, CENTRE, WHITE, false, lcd);
  }
  destination[EEP_ALIAS_LENGTH - 1] = '\0';

  disp_cycleColours(1000, 100, lcd);
  lcd.setBacklight(WHITE);
  
}

/* Maps input, as returned by disp_getInput or disp_getTimedInput, 
 * to the value of a Symbol in the Arrow Symbol set.
 * input - A byte as read from the lcd.
 * Returns - The value of a Symbol in the Arrow Symbol set.
 *           0 if the input byte cannot be mapped.
 */ 
char inputToArrowSymbol(byte input){
  char value;
    
  switch(input){
    case RIGHT_BUTTON:
      value = 'r';
      break;

    case LEFT_BUTTON:
      value = 'l';
      break;

    case DOWN_BUTTON:
      value = 'd';
      break;

    case UP_BUTTON:
      value = 'u';
      break;

    default:
      value = 0;
      break;
  }
  return value;
}

/* Maps the time it took the user to enter a symbol to a score.
 * Lower times map to higher scores.
 * timeToInput - The time, in milliseconds, that it took the user to make an input.
 * Returns - A score corresponding to the user's input time.
 */
unsigned int inputTimeToScore(unsigned int timeToInput){
  //100pts for recalling the correct symbol
  unsigned int score = 100;

  //Time bonuses
  if(timeToInput <= 250)
    score += 50;

  if(timeToInput <= 500)
    score += 25;

  if(timeToInput <= 1000)
    score += 25;
   
  return score;    
}

/* Maps input, as returned by disp_getInput or disp_getTimedInput, 
 * to the value of a Symbol in the Arrow Symbol set such that the input appears reversed.
 * input - A byte as read from the lcd.
 * Returns - The value of a Symbol in the Arrow Symbol set.
 *           0 if the input byte cannot be mapped.
 */ 
char mirrorInputToArrowSymbol(byte input){
  char value;
    
  switch(input){
    case RIGHT_BUTTON:
      value = 'l';
      break;

    case LEFT_BUTTON:
      value = 'r';
      break;

    case DOWN_BUTTON:
      value = 'u';
      break;

    case UP_BUTTON:
      value = 'd';
      break;

    default:
      value = 0;
      break;
  }
  return value;  
}

/* Randomly increases the complexity of the given RoundProfile.
 * profile - A pointer to the RoundProfile whose complexity is to be increased.
 * roundNumber - The number of the round.
 */
void randomRoundComplexity(RoundProfile *profile, unsigned int roundNumber){
  const byte SEQUENCE_LENGTH = 0;
  const byte UNIQUE_SYMBOLS = 1;
  const byte DISPLAY_TIME = 2;
  const byte INPUT_TIME = 3;
  
  byte randomComplexity;
  bool complexityIncreased = false;
  bool complexityMaximised = false;
  
  if(!complexityMaximised && roundNumber > 1){
    randomComplexity = random(0, 4);
    complexityIncreased = false;
    
    while(!complexityIncreased){
      switch(randomComplexity){
        case SEQUENCE_LENGTH:
          if(profile->sequenceLength < ROUNDP_SEQUENCE_LENGTH_MAX){
            profile->sequenceLength++;
            complexityIncreased = true;

            #ifdef DEBUG
              if(DEBUG_ENABLED){
                Serial.print(F("Random complexity - sequence length increased to "));
                Serial.println(profile->sequenceLength);
              }
            #endif
          }
          else
            randomComplexity++;

          break;


        case UNIQUE_SYMBOLS:
          if(profile->uniqueSymbols < ROUNDP_UNIQUE_SYMBOLS_MAX){
            profile->uniqueSymbols++;
            complexityIncreased = true;
           
            #ifdef DEBUG
              if(DEBUG_ENABLED){
                Serial.print(F("Random complexity - unique symbols increased to "));
                Serial.println(profile->uniqueSymbols);
              }
            #endif
          }
          else
            randomComplexity++;

          break;


        case DISPLAY_TIME:
          if(profile->displayTime - 250 >= ROUNDP_DISPLAY_TIME_MIN){
            profile->displayTime -= 250;
            complexityIncreased = true;

            #ifdef DEBUG
              if(DEBUG_ENABLED){
                Serial.print(F("Random complexity - display time decreased to "));
                Serial.print(profile->displayTime);
                Serial.println("ms");
              }
            #endif
          }
          else{
            randomComplexity++;
            profile->displayTime = ROUNDP_DISPLAY_TIME_MIN;
          }
            

          break;


        case INPUT_TIME:
          if(profile->inputTime - 250 >= ROUNDP_INPUT_TIME_MIN){
            profile->inputTime -= 250;
            complexityIncreased = true;

            #ifdef DEBUG
              if(DEBUG_ENABLED){
                Serial.print(F("Random complexity - input time decreased to "));
                Serial.print(profile->inputTime);
                Serial.println("ms");
              }
            #endif
          }
          else{
            profile->inputTime = ROUNDP_INPUT_TIME_MIN;
            if(profile->sequenceLength < ROUNDP_SEQUENCE_LENGTH_MAX)
              randomComplexity = SEQUENCE_LENGTH;
            else if(profile->uniqueSymbols < ROUNDP_UNIQUE_SYMBOLS_MAX)
              randomComplexity = UNIQUE_SYMBOLS;
            else if(profile->displayTime > ROUNDP_DISPLAY_TIME_MIN)
              randomComplexity = DISPLAY_TIME;
            else{
              complexityMaximised = true;
              complexityIncreased = true;

              #ifdef DEBUG
                if(DEBUG_ENABLED)
                  Serial.println(F("Random complexity maximised."));
              #endif
            }
          }
          break;
      }
    }
  }
}

/* Initialises the fields in the GameProfile pointed to by destination for the Practice game mode.
 * destination - A pointer to the GameProfile which is to be initialised.
 */
void initPracticeGameProfile(GameProfile* destination){
  destination->rProfile.sequenceLength = ROUNDP_SEQUENCE_LENGTH_MIN;
  destination->rProfile.uniqueSymbols = ROUNDP_UNIQUE_SYMBOLS_MIN;
  arrow_createSymbolSet(destination->rProfile.symbols);
  destination->rProfile.inputToSymbolValue = &inputToArrowSymbol;
  destination->rProfile.displayTime = 1000;
  destination->rProfile.inputTime = 2000;  
  destination->rProfile.score = 0;

  destination->changeRoundComplexity = &randomRoundComplexity;
  destination->isComplexityChanging = false;
  destination->roundCount = 1;
  destination->lifeCount = 1;
  strcpy(destination->gameTitle, "Practice");
}

/* Increases the complexity of an easy round in accordance with a predetermined pattern.
 * profile - A pointer to the RoundProfile which is to be modified.
 * roundNumber - The number of the round, starting at 1.
 */
void easyRoundComplexity(RoundProfile *profile, unsigned int roundNumber){
  switch(roundNumber){
    case 1:
      break;
      
    case 2:
      break;
      
    case 3:
      profile->sequenceLength = 5;
      profile->displayTime = 950;
      break;
      
    case 4:
      profile->displayTime = 900;
      break;
      
    case 5:
      profile->displayTime = 850;
      profile->inputTime = 1500;
      break;
      
    case 6:
      profile->uniqueSymbols = 3;
      profile->displayTime = 800;
      break;
      
    case 7:
      profile->displayTime = 750;
      break;
      
    case 8:
      profile->displayTime = 700;
      break;
      
    case 9:
      profile->sequenceLength = 6;
      profile->displayTime = 650;
      break;
      
    case 10:
      profile->displayTime = 600;
      break;

    default:
      break;
  }
}

/* Initialises the fields in the GameProfile pointed to by destintion for the Easy game mode.
 * destination - A pointer to the GameProfile which is to be initialised. 
 */
void initEasyGameProfile(GameProfile* destination){
  destination->rProfile.sequenceLength = 4;
  destination->rProfile.uniqueSymbols = 2;
  arrow_createSymbolSet(destination->rProfile.symbols);
  destination->rProfile.inputToSymbolValue = &inputToArrowSymbol;
  destination->rProfile.displayTime = 1500;
  destination->rProfile.inputTime = 2000;
  destination->rProfile.score = 0;

  destination->changeRoundComplexity = &easyRoundComplexity;
  destination->isComplexityChanging = true;
  destination->roundCount = 10;
  destination->lifeCount = 3;
  strcpy(destination->gameTitle, "Easy");
}

/* Increases the complexity of a medium difficulty round in accordance with a predetermined pattern.
 * profile - A pointer to the RoundProfile which is to be modified.
 * roundNumber - The number of the round, starting at 1.
 */
void mediumRoundComplexity(RoundProfile *profile, unsigned int roundNumber){
  switch(roundNumber){
    case 1:
      break;
    case 2:
      break;
    case 3:
      profile->displayTime = 950;
      break;
    case 4:
      profile->displayTime = 900;
      break;
    case 5:
      profile->displayTime = 850;
      profile->sequenceLength = 6;
      break;
    case 6:
      profile->displayTime = 800;
      break;
    case 7:
      profile->displayTime = 750;
      profile->inputTime = 1000;
      break;
    case 8:
      profile->uniqueSymbols = 4;
      profile->displayTime = 700;
      break;
    case 9:
      profile->displayTime = 650;
      break;
    case 10:
      profile->displayTime = 600;
      profile->sequenceLength = 7;
      break;
    case 11:
      profile->displayTime = 550;
      break;
    case 12:
      profile->displayTime = 500;
      break;
    case 13:
      profile->sequenceLength = 8;
      break;
    case 14:
      break;
    case 15:
      break;
    default:
      break;
  }
}

/* Initialises the fields in the GameProfile pointed to by destintion for the Medium game mode.
 * destination - A pointer to the GameProfile which is to be initialised. 
 */
void initMediumGameProfile(GameProfile* destination){
  destination->rProfile.sequenceLength = 5;
  destination->rProfile.uniqueSymbols = 3;
  arrow_createSymbolSet(destination->rProfile.symbols);
  destination->rProfile.inputToSymbolValue = &inputToArrowSymbol;
  destination->rProfile.displayTime = 1000;
  destination->rProfile.inputTime = 1500;
  destination->rProfile.score = 0;

  destination->changeRoundComplexity = &mediumRoundComplexity;
  destination->isComplexityChanging = true;
  destination->roundCount = 15;
  destination->lifeCount = 3;
  strcpy(destination->gameTitle, "Medium");
}

/* Increases the complexity of a hard difficulty round in accordance with a predetermined pattern.
 * profile - A pointer to the RoundProfile which is to be modified.
 * roundNumber - The number of the round, starting at 1.
 */
void hardRoundComplexity(RoundProfile *profile, unsigned int roundNumber){
  switch(roundNumber){
    case 1:
      break;
    case 2:
      break;
    case 3:
      profile->displayTime = 750;
      profile->inputTime = 1400;
      break;
    case 4:
      break;
    case 5:
      profile->sequenceLength = 7;
      break;
    case 6:
      profile->displayTime = 700;
      profile->inputTime = 1300;
      break;
    case 7:
      break;
    case 8:
      break;
    case 9:
      profile->displayTime = 650;
      profile->inputTime = 1200;
      break;
    case 10:
      profile->sequenceLength = 8;
      break;
    case 11:
      break;
    case 12:
      profile->displayTime = 600;
      profile->inputTime = 1100;
      break;
    case 13:
      break;
    case 14:
      profile->sequenceLength = 9;
      break;
    case 15: 
      break;
    case 16:
      profile->displayTime = 500;
      profile->inputTime = 1000;
      break;
    case 17:
      break;
    case 18:
      profile->sequenceLength = 10;
      profile->inputTime = 800;
      break;
    case 19:
      break;
    case 20:
      break;
    default:
      break;
  }
}

/* Initialises the fields in the GameProfile pointed to by destintion for the Hard game mode.
 * destination - A pointer to the GameProfile which is to be initialised. 
 */
void initHardGameProfile(GameProfile* destination){
  destination->rProfile.sequenceLength = 6;
  destination->rProfile.uniqueSymbols = 4;
  arrow_createSymbolSet(destination->rProfile.symbols);
  destination->rProfile.inputToSymbolValue = &inputToArrowSymbol;
  destination->rProfile.displayTime = 800;
  destination->rProfile.inputTime = 1500;
  destination->rProfile.score = 0;

  destination->changeRoundComplexity = &hardRoundComplexity;
  destination->isComplexityChanging = true;
  destination->roundCount = 20;
  destination->lifeCount = 3;
  strcpy(destination->gameTitle, "Hard");
}

/* Initialises the fields in the GameProfile pointed to by destintion for the Mirror game mode.
 * destination - A pointer to the GameProfile which is to be initialised. 
 */
void initMirrorGameProfile(GameProfile *destination){
  destination->rProfile.sequenceLength = 4;
  destination->rProfile.uniqueSymbols = 2;
  arrow_createSymbolSet(destination->rProfile.symbols);
  destination->rProfile.inputToSymbolValue = &mirrorInputToArrowSymbol;
  destination->rProfile.displayTime = 1500;
  destination->rProfile.inputTime = 2000;
  destination->rProfile.score = 0;
    
  destination->changeRoundComplexity = &randomRoundComplexity;
  destination->isComplexityChanging = true;
  destination->roundCount = 15;
  destination->lifeCount = 3;
  strcpy(destination->gameTitle, "Mirror");  
}

/* Runs a menu from which the user can set parameters for the practice mode
 * as well as configure other parts of the system.
 * profile - A pointer to a GameProfile whose fields will be changed by the user.
 *           This GameProfile should be used to launch the practice mode.
 * lcd - An LCDShield to use as the UI.
 */
void runConfigurationMenu(GameProfile* profile, LCDShield lcd){
  const byte ROUNDS = 0;
  const byte LIVES = 1;
  const byte SEQUENCE_LENGTH = 2;
  const byte SYMBOLS = 3;
  const byte DISPLAY_TIME = 4;
  const byte INPUT_TIME = 5;
  const byte COMPLEXITY = 6;
  const byte RESET = 7;
  const byte BACK = 8;
  #ifdef DEBUG
  const byte DEBUG_CONFIG = 9;
  #endif
  const byte END = 10;

  const byte SELECT = 0b00000001;
  const byte RIGHT = 0b00000010;
  const byte LEFT = 0b00010000;
  const byte DOWN = 0b00000100;
  const byte UP = 0b00001000;

  Symbol upPrompt;
  Symbol downPrompt;
  Symbol biPrompt;

  arrow_createUpPromptSymbol(&upPrompt);
  arrow_createDownPromptSymbol(&downPrompt);
  arrow_createBiPromptSymbol(&biPrompt);
  
  byte state = ROUNDS;
  byte input;

  char bufferStr[16];

  while(state != END){
    switch(state){
      case ROUNDS:
        disp_printStr("Rounds", 0, RIGHT, WHITE, true, lcd);
        if(profile->roundCount == GAMEP_ROUND_COUNT_INF)
          sprintf(bufferStr, "<%c>", 0b11110011);
        else
          sprintf(bufferStr, "<%hu>", profile->roundCount);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&downPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            
            if(profile->roundCount == GAMEP_ROUND_COUNT_INF)
              profile->roundCount = GAMEP_ROUND_COUNT_MAX;
            else if(profile->roundCount > GAMEP_ROUND_COUNT_MIN) 
              profile->roundCount--;
            else
              profile->roundCount = GAMEP_ROUND_COUNT_INF;
            
            break;

          case RIGHT:
            if(profile->roundCount == GAMEP_ROUND_COUNT_INF)
              profile->roundCount = GAMEP_ROUND_COUNT_MIN;
            else if(profile->roundCount < GAMEP_ROUND_COUNT_MAX)
              profile->roundCount++;
            else
              profile->roundCount = GAMEP_ROUND_COUNT_INF;
            break;

          case DOWN:
            state = LIVES;
            break;
        }
        break;
    
      case LIVES:
        disp_printStr("Lives", 0, RIGHT, WHITE, true, lcd);
        sprintf(bufferStr, "<%hu>", profile->lifeCount);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            if(profile->lifeCount > GAMEP_LIFE_COUNT_MIN) profile->lifeCount--;
            break;

          case RIGHT:
            if(profile->lifeCount < GAMEP_LIFE_COUNT_MAX) profile->lifeCount++;
            break;

          case UP:
            state = ROUNDS;
            break;

          case DOWN:
            state = SEQUENCE_LENGTH;
            break;
        }
        break;

      case SEQUENCE_LENGTH:
        disp_printStr("Sequence Len.", 0, RIGHT, WHITE, true, lcd);
        sprintf(bufferStr, "<%hu>", profile->rProfile.sequenceLength);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            if(profile->rProfile.sequenceLength > ROUNDP_SEQUENCE_LENGTH_MIN) profile->rProfile.sequenceLength--;
            break;

          case RIGHT:
            if(profile->rProfile.sequenceLength < ROUNDP_SEQUENCE_LENGTH_MAX) profile->rProfile.sequenceLength++;
            break;

          case UP:
            state = LIVES;
            break;

          case DOWN:
            state = SYMBOLS;
            break;
        }
        break;

      case SYMBOLS:
        disp_printStr("Unique Symbols", 0, RIGHT, WHITE, true, lcd);
        sprintf(bufferStr, "<%hu>", profile->rProfile.uniqueSymbols);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            if(profile->rProfile.uniqueSymbols > ROUNDP_UNIQUE_SYMBOLS_MIN) profile->rProfile.uniqueSymbols--;
            break;

          case RIGHT:
            if(profile->rProfile.uniqueSymbols < ROUNDP_UNIQUE_SYMBOLS_MAX) profile->rProfile.uniqueSymbols++;
            break;

          case UP:
            state = SEQUENCE_LENGTH;
            break;

          case DOWN:
            state = DISPLAY_TIME;
            break;
        }
        break;

      case DISPLAY_TIME:
        disp_printStr("Display Time", 0, RIGHT, WHITE, true, lcd);
        sprintf(bufferStr, "<%u>", profile->rProfile.displayTime);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            if(profile->rProfile.displayTime > ROUNDP_DISPLAY_TIME_MIN) profile->rProfile.displayTime -= 100;
            break;

          case RIGHT:
            if(profile->rProfile.displayTime < ROUNDP_DISPLAY_TIME_MAX) profile->rProfile.displayTime += 100;
            break;

          case UP:
            state = SYMBOLS;
            break;

          case DOWN:
            state = INPUT_TIME;
            break;
        }
        break;

        
      case INPUT_TIME:
        disp_printStr("Input Time", 0, RIGHT, WHITE, true, lcd);
        sprintf(bufferStr, "<%u>", profile->rProfile.inputTime);
        disp_printStr(bufferStr, 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            if(profile->rProfile.inputTime > ROUNDP_INPUT_TIME_MIN) profile->rProfile.inputTime -= 100;
            break;

          case RIGHT:
            if(profile->rProfile.inputTime < ROUNDP_INPUT_TIME_MAX) profile->rProfile.inputTime += 100;
            break;

          case UP:
            state = DISPLAY_TIME;
            break;

          case DOWN:
            state = COMPLEXITY;
            break;
        }
        break;

      case COMPLEXITY:
        disp_printStr("Complexity", 0, RIGHT, WHITE, true, lcd);
        
        if(profile->isComplexityChanging)
          disp_printStr("< Increasing", DISP_BOTTOM, RIGHT, CURRENT, false, lcd);
        else
          disp_printStr("Unchanging >", DISP_BOTTOM, RIGHT, CURRENT, false, lcd);
          
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            profile->isComplexityChanging = false;
            break;

          case RIGHT:
            profile->isComplexityChanging = true;
            break;

          case UP:
            state = INPUT_TIME;
            break;

          case DOWN:
            state = RESET;
            break;
          }
        break;

      case RESET:
        disp_printStr("Reset", 0, RIGHT, WHITE, true, lcd);
        disp_printStr("System", 1, RIGHT, WHITE, false, lcd);
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case SELECT:
            eep_initialiseHighScoreTable();
            eep_writeSeed(0UL);
            disp_printStr("Done!", 0, CENTRE, WHITE, true, lcd);
            delayByPoll(2000);
            break;

          case UP:
            state = COMPLEXITY;
            break;

          case DOWN:
          #ifdef DEBUG
            state = DEBUG_CONFIG;
          #else
            state = BACK;
          #endif
            break;
        }
        break;

      #ifdef DEBUG
      case DEBUG_CONFIG:
        disp_printStr("Debug", 0, RIGHT, WHITE, true, lcd);
        if(DEBUG_ENABLED)
          disp_printStr("< Enabled", 1, RIGHT, CURRENT, false, lcd);
        else
          disp_printStr("Disabled >", 1, RIGHT, CURRENT, false, lcd);
        
        disp_symbol(&biPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case LEFT:
            DEBUG_ENABLED = false;
            break;

          case RIGHT:
            DEBUG_ENABLED = true;
            break;

          case UP:
            state = RESET;
            break;

          case DOWN:
            state = BACK;
            break;

          default:
            break;
        }
        
        break;
      #endif
        

      case BACK:
        disp_printStr("Back", 0, RIGHT, WHITE, true, lcd);
        disp_symbol(&upPrompt, 0, lcd);
        input = disp_buttonPress(lcd);

        switch(input){
          case SELECT:
            state = END;
            break;

          case UP:
          #ifdef DEBUG
            state = DEBUG_CONFIG;
          #else
            state = RESET;
          #endif
            break;
        }
        break;     
    }
  }
}

/* Runs an entire game of the memory game, using the contents of the GameProfile as pointed
 * to by profile to control the game play.
 * profile - A pointer to a GameProfile whose contents are used to control the game play of 
 *           the game. See the definition of struct gameProfile for a complete explanation.
 * lcd - An LCDShield to use as the UI.
 */
void runGame(GameProfile* profile, LCDShield lcd){
  bool gameLost = false;
  unsigned short remainingLives = profile->lifeCount;
  char strBuffer[16];

  //Display something sensible if the user has chosen infinite rounds.
  if(profile->roundCount == GAMEP_ROUND_COUNT_INF)
    sprintf(strBuffer, "%c rounds", 0b11110011);
  else
    sprintf(strBuffer, "%hu rounds", profile->roundCount);
    
  disp_title(profile->gameTitle, strBuffer, WHITE, lcd);

  for(int i = 0; i < profile->roundCount && !gameLost; i++){
    bool roundLost;
    
    #ifdef DEBUG 
    if(DEBUG_ENABLED)
      Serial.print(F("---Round starting---\n"));
    #endif

    if(profile->isComplexityChanging)
      profile->changeRoundComplexity(&profile->rProfile, i + 1);
    
    disp_roundCount(i+1, profile->roundCount, lcd);
    
    roundLost = !runRound(&profile->rProfile, lcd);

    if(roundLost){
      remainingLives--;
      sprintf(strBuffer, "%hu lives", remainingLives);
      disp_printStr(strBuffer, 0, CENTRE, RED, true, lcd);
      disp_printStr("Remaining", 1, CENTRE, RED, false, lcd);
      delayByPoll(1000);
    }

    if(remainingLives < 1)
      gameLost = true;
  }

  //If the player won the game, award additional points.
  if(!gameLost){
    unsigned int bonusPoints = 100 * profile->roundCount;
    profile->rProfile.score += bonusPoints;
    disp_gameWon(profile->rProfile.score, bonusPoints, lcd);
  }
  else{
    disp_gameLost(profile->rProfile.score, lcd);
  }

  //Get high score and add to high score table.
  if(eep_isHighScore(profile->rProfile.score)){
    HighScore newHS;
    newHS.score = profile->rProfile.score;
    disp_newHighscore(profile->rProfile.score, lcd);
    getAlias(newHS.alias, lcd);
    eep_addHighScore(&newHS);
  }

  //Randomise the random seed.
  unsigned long nextSeed = random(0, pow(2, sizeof(long)));
  #ifdef debug
    if(DEBUG_ENABLED)
      Serial.println(F("Next seed: " + nextSeed));
  #endif
  
  randomSeed(nextSeed);
  eep_writeSeed(nextSeed);
}

/* Allows the user to browse the high score table as stored in EEPROM.
 * lcd - The LCDShield to use as the UI.
 */
void runLeaderboard(LCDShield lcd){
  HighScore leaderboard[EEP_HIGH_SCORE_TABLE_SIZE];
  bool goBack = false;
  byte index = 0;
  byte input;
  char bufferStr[11];
  
  Symbol upPrompt;
  Symbol downPrompt;
  Symbol biPrompt;

  arrow_createUpPromptSymbol(&upPrompt);
  arrow_createDownPromptSymbol(&downPrompt);
  arrow_createBiPromptSymbol(&biPrompt);
  
  eep_readHighScoreTable(leaderboard);

  while(!goBack){
    if(index >= 0 && index < EEP_HIGH_SCORE_TABLE_SIZE){
      strcpy(bufferStr, leaderboard[index].alias);
      disp_printStr(bufferStr, 0, CENTRE, WHITE, true, lcd);
      sprintf(bufferStr, "%lu", leaderboard[index].score);
      disp_printStr(bufferStr, 1, CENTRE, WHITE, false, lcd);
     
      if(index == 0){
        disp_symbol(&downPrompt, 0, lcd); 
        
        if((input = disp_buttonPress(lcd)) == DOWN_BUTTON)
          index++;
      }
      else{
        disp_symbol(&biPrompt, 0, lcd);
        
        if((input = disp_buttonPress(lcd)) == UP_BUTTON)
          index--;
        else if(input == DOWN_BUTTON)
          index++;
      }
    }
      
    else if(index == EEP_HIGH_SCORE_TABLE_SIZE){
      disp_printStr("Back", 0, CENTRE, WHITE, true, lcd);
      disp_symbol(&upPrompt, 0, lcd);
      
      if((input = disp_buttonPress(lcd)) == UP_BUTTON)
        index--;
      else if(input == SELECT_BUTTON)
        goBack = true;
    }
  }  
}

/* Runs the main menu from which the user can perform all the different system functionalities.
 * lcd - An LCDShield to use as the interface. 
 */
void runMenu(LCDShield lcd){
  //Menu states.
  const byte PRACTICE = 0;
  const byte STORY = 1;
  const byte SCORE = 2;
  const byte OPTIONS = 3;

  //Story mode difficulties.
  const byte EASY = 0;
  const byte MEDIUM = 1;
  const byte HARD = 2;
  const byte MIRROR = 3;

  byte state = PRACTICE;
  byte difficulty = EASY;
  char difficultyStr[11];
  byte input = 0;

  GameProfile configProfile;
  GameProfile runningProfile;

  initPracticeGameProfile(&configProfile);
  
  Symbol upPrompt;
  Symbol downPrompt;
  Symbol biPrompt;
  
  arrow_createUpPromptSymbol(&upPrompt);
  arrow_createDownPromptSymbol(&downPrompt);
  arrow_createBiPromptSymbol(&biPrompt); 

  while(true){
    switch(state){
      case PRACTICE:
        disp_printStr("Practice Mode", 0, RIGHT, WHITE, true, lcd);
        disp_symbol(&downPrompt, 0, lcd);
        
        if((input = disp_buttonPress(lcd)) & DOWN_BUTTON)
          state = STORY;
        else if(input == SELECT_BUTTON){
          runningProfile = configProfile;
          runGame(&runningProfile, lcd);  
        }
          
        break;


      case STORY:
        switch(difficulty){
          case EASY:
            strcpy(difficultyStr, "Easy >");
            break;

          case MEDIUM:
            strcpy(difficultyStr, "< Medium >");
            break;

          case HARD:
            strcpy(difficultyStr, "< Hard >");
            break;

          case MIRROR:
            strcpy(difficultyStr, "< Mirror");
            break;
        }
        
        disp_printStr("Story Mode", 0, RIGHT, WHITE, true, lcd); 
        disp_printStr(difficultyStr, 1, RIGHT, WHITE, false, lcd);       
        disp_symbol(&biPrompt, 0, lcd);
        
        if((input = disp_buttonPress(lcd)) == UP_BUTTON)
          state = PRACTICE;          
        else if(input == DOWN_BUTTON)
          state = SCORE;
        else if(input == LEFT_BUTTON){
          if(difficulty == MEDIUM)
            difficulty = EASY;
          else if(difficulty == HARD)
            difficulty = MEDIUM;
          else if(difficulty == MIRROR)
            difficulty = HARD;
        }
        else if(input == RIGHT_BUTTON){
          if(difficulty == EASY)
            difficulty = MEDIUM;
          else if(difficulty == MEDIUM)
            difficulty = HARD;
          else if(difficulty == HARD)
            difficulty = MIRROR;
        }
        else if(input == SELECT_BUTTON){
          if(difficulty == EASY)
            initEasyGameProfile(&runningProfile);
          else if(difficulty == MEDIUM)
            initMediumGameProfile(&runningProfile);
          else if(difficulty == HARD)
            initHardGameProfile(&runningProfile);
          else if(difficulty == MIRROR)
            initMirrorGameProfile(&runningProfile);

          runGame(&runningProfile, lcd);
        } 
        break;


      case SCORE:
        disp_printStr("High Scores", 0, RIGHT, WHITE, true, lcd);
        disp_symbol(&biPrompt, 0, lcd);

        if((input = disp_buttonPress(lcd)) == UP_BUTTON)
          state = STORY;
        else if (input == DOWN_BUTTON)
          state = OPTIONS;
        else if (input == SELECT_BUTTON)
          runLeaderboard(lcd);
                    
        break;

      case OPTIONS:
        disp_printStr("Configure", 0, RIGHT, WHITE, true, lcd);
        disp_symbol(&upPrompt, 0, lcd);

        if((input = disp_buttonPress(lcd)) == UP_BUTTON)
          state = SCORE;
        else if(input == SELECT_BUTTON){
          runConfigurationMenu(&configProfile, lcd);
        }
          
        break;
    }
  }  
}

/* Runs a round of the memory game using the fields in profile to effect the gameplay.
 * profile - The fields of the struct pointed to by this parameter define various aspects
 *           of the round's gameplay, such as sequence length, how long the sequence appears on scren etc.
 *           See the definition for struct roundProfile for a complete explanation.
 * lcd - An LCDShield to use as the UI.
 * Returns - true if the player won the round. false otherwise.
 */
bool runRound(RoundProfile* profile, Adafruit_RGBLCDShield lcd){
  // States for the state machine.
  const byte END = 0;
  const byte START = 1;
  const byte INITIALISING = 2;
  const byte DISPLAYING_SEQUENCE = 3;
  const byte GETTING_INPUT = 4;
  const byte TIMED_OUT = 5;
  const byte COMPARING_INPUT = 6;
  const byte CORRECT_INPUT = 7;
  const byte INCORRECT_INPUT = 8;
  const byte ROUND_WON = 9;
  const byte ROUND_LOST = 10;
 
  byte state = START; 
  //References to the Symbols which will be used in this round's sequence.
  Symbol *roundSymbols[profile->uniqueSymbols];
  Symbol *sequence[profile->sequenceLength];
  byte input;
  char inputAsSymbolValue;
  unsigned short inputCount = 0;
  unsigned int timeToInput;
  unsigned int totalInputTime = 0;
  unsigned int scoreIncrement = 0;
  bool roundWon = false;

  //State machine implementation of the round.
  while(state != END){
    switch(state){
      case START:
        state = INITIALISING;
        break;


      case INITIALISING:
        //Randomly selects the symbols to use in this round and then generates the sequence.
        chooseRandomSymbols(profile->symbols, roundSymbols, ROUNDP_SYMBOLS_LENGTH, profile->uniqueSymbols);
        generateSymbolSequence(roundSymbols, sequence, profile->uniqueSymbols, profile->sequenceLength);
        
        state = DISPLAYING_SEQUENCE;

        #ifdef DEBUG
          if(DEBUG_ENABLED){
            Serial.print(F("Round Symbols:\n"));
            for(int i = 0; i < profile->uniqueSymbols; i++){
              Serial.print(roundSymbols[i]->value);
              Serial.write(' ');
            }
            Serial.write('\n');
            Serial.print(F("Sequence:\n"));
            for(int i = 0; i < profile->sequenceLength; i++){
              Serial.print(sequence[i]->value);
              Serial.write(' ');
            }
            Serial.write('\n');
          }
        #endif

        break;


      case DISPLAYING_SEQUENCE:        
        disp_countdown("MEMORISE", lcd);
        disp_sequence(sequence, profile->sequenceLength, profile->displayTime, lcd);
        disp_countdown("REPEAT", lcd);
        disp_score(profile->score, 0, lcd);

        state = GETTING_INPUT;
        break;


      case GETTING_INPUT:
        input = disp_timedButtonPress(profile->inputTime, &timeToInput, lcd);
        totalInputTime += timeToInput;
        inputCount++;

        //Time out if the player does not make an input in time.
        if(input == NONE_BUTTON)
          state = TIMED_OUT;
        else
          state = COMPARING_INPUT;        
        break;


      case TIMED_OUT:        
        disp_timeout(lcd);
        disp_incorrectInput(sequence[inputCount - 1], lcd);
        state = ROUND_LOST;
        break;


      case COMPARING_INPUT:
        inputAsSymbolValue = (*(profile->inputToSymbolValue))(input);
        
        if(inputAsSymbolValue == sequence[inputCount - 1]->value)
          state = CORRECT_INPUT;
        else
          state = INCORRECT_INPUT;
        
        break;


      case CORRECT_INPUT:
        //Award bonus points based on speed of player response.
        scoreIncrement = inputTimeToScore(timeToInput);
        profile->score += scoreIncrement;
        disp_score(profile->score, scoreIncrement, lcd);
        disp_symbol(sequence[inputCount - 1], 7, lcd);
        
        if(inputCount < profile->sequenceLength){
          state = GETTING_INPUT;
        }
        else
          state = ROUND_WON;
  
        break;


      case INCORRECT_INPUT:
        disp_incorrectInput(sequence[inputCount - 1], lcd);        
        state = ROUND_LOST;
        
        break;


      case ROUND_WON:
        roundWon = true;
        //If each symbol was recalled in less than half a second.
        if(totalInputTime < 500 * profile->sequenceLength){
           unsigned int increment = 75 * profile->sequenceLength;
           profile->score += increment;
           disp_roundWon("FAST", increment, lcd);
        }
        else{
          unsigned int increment = 50 * profile->sequenceLength;
          profile->score += increment;
          disp_roundWon("GOOD", increment, lcd);
        }

        #ifdef DEBUG
          if(DEBUG_ENABLED){
            Serial.print(F("Total input time: "));
            Serial.print(totalInputTime);
            Serial.println(F("ms"));
          }
        #endif
                
        state = END;
        break;

     case ROUND_LOST:
       state = END;
       break;
    }
  }

  return roundWon;
}
