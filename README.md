# Mesmerize
A simon says style memory game for Arduino systems equipped with an Adafruit RGB LCD Shield.

This project was developed as coursework for an embedded systems programming course at Loughborough University. It was awarded 99% which made it the highest scoring submission that year. The coursework specification has not been included to avoid copyright infringement, but my submission's readme file, which gives details of the task and program, has been included below. 

# Running the Program
This program was originally developed for an Arduino Uno (from around 2019) equipped with an Adafruit RGB LCD Shield. Although untested, the program should be compatible with other Arduino systems so long as they can connect to the Adafruit shield (although the game's timing may differ).

To run the program, download mesmerize.ino and the required libraries, and then use the Arduino IDE to install mesmerize on your Arduino.

Required libraries:
* Adafruit_RGBLCDShield.h
* Adafruit_MCP23017.h
* EEPROM.h
* Wire.h

# Original Readme
What follows is the readme file which was included with mesmerize.ino as part of my coursework submission. 

## INS: Instructions


First Time Playing?


If you have just installed Mesmerize on your Arduino then make sure to select the 'Reset System' option in the 'Configure' menu. This will initialise the Arduino's EEPROM. See Navigating Configure for more details.


Navigating the Main Menu:


The first thing you'll see when you boot the system is a title screen. The second thing you'll see is the main menu. To navigate the menu, use the up and down directional buttons. The arrows to the left of the screen indicate that there are menu entries above (up arrow) and below (down arrow) the current menu entry. To select a menu entry, use the select button. The following explains the functionalities of each menu entry:


* Practice Mode - Play the memory game in practice mode. The gameplay of a practice mode game is determined by modifiable values found in the Configure menu. The complexity of rounds in practice mode games increases (if enabled) in a random way, unlike Story Mode games.

* Story Mode - Play the memory game in one of four difficulty modes: easy, medium, hard, and mirror. Browse the difficulty settings using the left and right directional buttons. The higher difficulty modes last longer and contain more complex sequences. Mirror mode requires that you input the 'mirror' sequence to the one shown during each round (see How To Play for more information). The increase in complexity for the easy, medium, and hard difficulties is predetermined. The increase in complexity for the mirror mode is random, much like practice mode.

* High Scores - Displays a table containing the highest scores and aliases of those who achieved them. The table is in descending score order. Navigate the table with the up and down directional buttons. To return to the main menu, navigate to 'Back' and press select.

* Configure - Allows you to modify the gameplay of practice mode games. Also contains options for resetting the system and enabling debug features. See 'Navigating Configure' for more. 


Navigating Configure:


The 'Configure' menu is navigated the same way as the main menu. Navigate to 'Back' and press select to return to the main menu. Applicable options, signalled by '<' and/or '>' surrounding their values, can be modified using the left and right directional buttons. The following describes the meaning of each option in 'Configure':


* Rounds - The number of rounds in a practice mode game.

* Lives - The number of lives you start with in a practice mode game.

* Sequence Len. - The length of the starting sequence in a practice mode game.

* Unique Symbols - The number of different symbols in the starting sequence of a practice mode game.

* Display Time - The length of time, in milliseconds, for which each symbol in a sequence appears on screen in the starting sequence of a practice mode game.

* Input Time - The length of time, in milliseconds, that the player has to enter each symbol in the starting round of a practice mode game.

* Reset System - Initialises EEPROM such that the high score table is set to its starting value and the random seed is set to zero.

* Complexity - If set to 'Increasing' then each round in a practice mode game will be more difficult than the previous. The way in which complexity increases is randomly determined. Complexity will not increase between rounds if set to 'Unchanging'.

* Debug - When set to enabled, debug messages will be written to the Serial Monitor. Note that Debug messages may have a mild impact on performance.


How to Play the Memory Game:


A memory game consists of a series of rounds. If you pass all of the rounds without running out of lives, then you win the game. The game gets progressively more complex as you complete more rounds. The way in which round complexity increases is determined by the game mode (practice, easy, medium, hard, or mirror). As you play, you will earn points which will increase your score. Try to maximise your score to make it into the high scores table! For more details on acquiring points, see 'Scoring'. A round of the memory game is played as follows:


1. A brief countdown starts the round. Once the countdown is over, a sequence of arrows will appear on the screen. You must do your best to memorise this sequence!

2. Once the sequence has finished displaying, another brief countdown will appear. Once this countdown finishes, you must repeat the sequence you just saw using the directional buttons. Each arrow corresponds to a matching directional button, e.g. the up arrow corresponds with the up button, the left arrow with the left button etc. You will have limited time to make each input, so act fast! Depending on how well you play, several things can happen:

* If you input the sequence correctly, you will pass the round and be awarded points based on the sequence length.

* If you make an incorrect input, you will lose a life and fail the round.

* If you run out of time while making an input, you will also lose a life and fail the round.

3. Whether or not you pass a round, you will move onto the next round if you have lives remaining. If there are no more rounds to complete, you win the game and are awarded additional points. If you have no more lives remaining, you lose the game.

4. You will notice several things on the screen as you repeat the sequence:

* To the left of the screen is your total score (top) and the most recent addition to your score (bottom).

* In the middle of the screen is an arrow representing your last input.

* To the right of the screen (top) is the remaining time you have to make an input.

* The screen will flash different colours based on how quickly you are repeating the sequence. Violet is fastest, teal is fast, green is average, and white is slow. These colours also correspond to differing amounts of additional points for speed.

5. Whether or not you win the game, your score will be considered for the high score table. If your score is greater than any score on the high score table, then you will be prompted to enter your three letter alias (navigate the characters with the left and right directional buttons and use the select button to select a character) and your score will be added to the high score table which you can view from the menu.


Scoring:


Note: The first four conditions stack. E.g. if a correct input is made in a quarter of a second then you are awarded 100 + 50 + 25 + 25 = 200 points.

* Make the correct input: +100 points. 

* Make the correct input in a quarter of a second or less: +50 points.

* Make the correct input in half a second or less: +25 points.

* Make the correct input in a second or less: +25 points.

* Win a round with an average speed less than an input every half second: 75 * sequence length.

* Win a round with an average speed of an input every half second or more: 50 * sequence length.

* Win a game: 100 * round count.



## MFS: Features of the minimal feature set


Indicate by deleting YES or NO to the questions below.

If partially implemented, describe this by deleting *YES/NO* and

writing what does work.  Do not adjust any other text in this section.


* Sequences are displayed on the screen as specified: *YES*

* Backlight flashes (only when correct): *YES*

* Incorrect answer leads to game over: *YES*

* Game supports *N* up to 10: *YES*

* Game supports *M* of size 4: *YES*

* A menu is present: *YES*

* Size of *M* can be changed: *YES*

* *N* can be changed: *YES*

* Message when wrong printed: *YES*


## OAF: Optional and additional features


* Games that last a predetermined number of rounds, R.

* R is configurable for practice mode.

* A lives system which gives the player a number of rounds to fail before losing the game.

* The number of lives in a practice game is configurable.

* The length of a sequence can be increased up to 15.

* The duration each symbol in the sequence appears on screen, D, is affected by game complexity.

* D is configurable for practice mode.

* The duration the user has to enter each symbol in the sequence, T, is affected by game complexity.

* T is configurable for practice mode.

* Practice mode difficulty progression is configurable.

* A high score table of the ten highest scores, and the aliases of the players which achieved them, is stored in EEPROM and can be viewed from the menu.

* The system contains a debug mode which supports testing the system by use of the serial interface.

* Symbols are drawn as a 2x2 matrix of custom characters.

* The player gains points based on how quickly they recall the sequence.

* The player gains points based on the length of the sequence they recall.

* The system contains four story mode difficulties: easy, medium, hard, and mirror.

* In the easy, medium, and hard story mode difficulties the player must play a game of predetermined length where the difficulty increases in a predetermined way.

* In the mirror story mode difficulty, the player must recall the sequence with the symbols mirrored, e.g. left as right, up as down, etc. where the difficulty increases randomly.


## TPL: Test plan


1. Open the Serial Monitor and reset the Arduino. Confirm that '--Mesmerize Serial Debugger--' appears on screen.

2. From the main menu, press down three times to navigate to the configure menu. Press select to enter the menu.

3. From the configure menu, press down seven times to navigate to the 'Reset System' option.

4. Press select and observe 'Done!' The system is now ready to play games.

5. Press down once to navigate to the 'Debug' option. Ensure it is set to 'Enabled'. If not, adjust this setting with the left and right directional buttons. The system will now print debugging messages over the Serial Monitor.

6. Navigate to 'Back' by pressing down one more time. Press select. You will return to the main menu.

7. Navigate to 'Practice Mode' by pressing up three times. 

8. Press select to begin a practice game.

9. Observe the LCD. Observe that the sequence is displayed as specified using a 2x2 matrix of custom characters.

10. Enter the sequence as shown using the directional buttons. Observe that the backlight flashes and the screen displays an appropriate message to indicate the correctness of the input.

11. You will be returned to the menu. Press select again to play another practice game.

12. Make an incorrect input when repeating the sequence and observe that the correct input is displayed, an appropriate loss message is displayed, the player loses a life, and the game ends by returning to the main menu.

13. From the main menu, navigate again to the configure menu.

14. In the configure menu, use the directional buttons to set the following values:

* Rounds - 3

* Lives - 2

* Sequence Len. - 15

* Unique Symbols - 4

* Display Time - 700

* Input Time - 3000

* Complexity - Increasing

15. Return to the main menu and navigate to practice mode.

16. Begin another game of the practice mode. Observe that the changes made in the configure menu have affected the sequence, the time for which each symbol is displayed, and the time the player has to make an input.

17. Do not make any inputs. Observe that inputs are timed, the timer is displayed to the right of the screen, and that failure to make an input in this time leads to losing a life.

19. Allow the system to continue to the next round. Observe that the sequence is displayed in a textual form on the Serial monitor. When appropriate, use this textual representation of the sequence to make the correct inputs. 

20. Observe that progression to the next round increases game complexity and the way in which complexity

is increased is displayed on the Serial Monitor.

21. Again, correctly enter the sequence using the Serial Monitor as a reference. 

22. Observe that a high score has been achieved.

23. Enter a three character alias. Use the left and right directional buttons to navigate the characters and press the select button to confirm a character.

24. Press the reset button on the Arduino.

25. From the main menu, navigate to 'High Scores' and press the select button.

26. Navigate the high score table using the up and down directional buttons. Observe that your high score is appropriately ordered amongst the other high scores.

27. Navigate to 'Back' and press the select button.

28. From the main menu, navigate to Story Mode. Navigate the difficulty modes with the left and right directional buttons.

29. Navigate to the easy difficulty setting and press the select button to start a game.

30. Use the first few rounds of the game to experiment with input timing. Observe that different timings lead to different point increments and backlight colours. Reference the 'Scoring' section under Instructions for a list of timings, increments, and colours. 

31. Observe that the length of the sequence and overall speed affects bonus points awarded at the end of the round. Again, reference 'Scoring'.

32. Once satisfied with testing the scoring mechanisms, press the 'Reset' button on the Arduino.

33. From the main menu, navigate to Story Mode and set the difficulty to 'Mirror'.

34. Play several rounds of the mirror game, noting that inputs are only considered correct if they 'mirror' the symbol in the sequence, e.g. left mirrors right, right mirrors left, up mirrors down, and down mirrors up.

35. Observe on the Serial monitor that the game complexity increases in a random fashion between rounds.

36. Allow the system to enter a game over. This concludes the Test Plan.
