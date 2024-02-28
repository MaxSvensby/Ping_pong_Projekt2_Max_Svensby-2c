/*
File: pong.ino
Author: Max Svensby
Date: 2024-02-28
Description: This is a game where you play as a paddle, with two players. There is a ball that bounces around and your goal is to block the ball from surpassing your paddle.
The two players score is displayed on the top side of the screen. If one of the players score is 8 then they win and the game ends.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definition of the Player class
class Player {
public:
// Constructor to initialize player properties
  Player(int analogPinX, int analogPinY, int startX, int startY, uint8_t height) : analogPinX(analogPinX), analogPinY(analogPinY), x(startX), y(startY), height(height) {}
// Method to update the player's vertical position based on the joystick input
  void updatePosition() {
     // Move up if joystick reading is below a threshold
    if (analogRead(analogPinX) < 475) {
      y -= 1;
    }
     // Move down if joystick reading is above a threshold
    if (analogRead(analogPinX) > 550) {
      y += 1;
    }
     // Ensure player stays within the screen bounds
    if (y < 1) y = 1;
    if (y + height > 63) y = 63 - height;
  }

 // Method to activate a wall ability with certain conditions
  void wallAbility() {
    Serial.println(millis() - ability_last_used); // print cooldown left
    // Check if enough time has passed since the last use of the ability
    if ((millis() - ability_last_used) >= 10000){
      // Check joystick y input to activate the wall ability
      if (analogRead(analogPinY) < 475) {
        // Activate wall ability by setting the height to the whole screen and the y value to the top so it doesn't start from the middle or something
        height = 62;
        y = 1;
        // Record the time the ability is used
        if (time_ability_is_used == 0){
          time_ability_is_used = millis();
        }
        // Deactivate wall ability after a certain duration, which is 3 seconds
        if (millis() - time_ability_is_used >= 3000) {
          ability_last_used = millis(); // reset the time since last ability was used to now
          time_ability_is_used = 0; // reset the time it has been used
          height = 12; // set height to normal
          y = 30; // Set y value to the middle
        }
      }
      else {
         // Deactivate wall ability if not in use
        if (height == 62){ // Check so it only deactivates and gets placed in the middle if the wall ability has just been used, so the height is 62
          height = 12;
          y = 30;
        }
      }
    }
  }
  
// Method to draw the player on the OLED display
  void drawPlayer(Adafruit_SSD1306& display, uint8_t xPosition, uint8_t color) {
    display.drawFastVLine(xPosition, y, height, color);
  }

 // Getter method for the player's y-coordinate, so i can use the real time y value in the other methods
  uint8_t getY() const {
    return y;
  }

 // Getter method for the player's height, so i can use the real time height in the other methods
  uint8_t getHeight() const {
    return height;
  }

private:
// Player variables
  int analogPinX, analogPinY;
  uint8_t x, y, height;
  unsigned long time_ability_is_used = 0;
  unsigned long ability_last_used = millis();
};

// Constants and variables for the game
const int SW_pin = 4;
const unsigned long PADDLE_RATE = 0; // unsigned long is basically a extremly big positive number. It can be atleast
const unsigned long BALL_RATE = 0;
uint8_t paddleHeight = 12;
int player1Score = 0;
int player2Score = 0;
int maxScore = 8;
int BEEPER = 12;
bool resetBall = false;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define RESET_BUTTON 3
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

uint8_t ball_x = 64, ball_y = 32;
uint8_t ball_dir_x = 1, ball_dir_y = 1;
unsigned long ball_update;
unsigned long paddle_update;
unsigned long time_ability_is_used = 0;
const uint8_t PLAYER2_X = 22;
const uint8_t PLAYER_X = 105;

// Create player objects
Player player1(A1, A2, PLAYER_X, 26, paddleHeight);
Player player2(A0, A3, PLAYER2_X, 26, paddleHeight);

// Setup function
void setup() {
   // Initialize serial communication, display, pins, and draw the initial game state
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(BEEPER, OUTPUT);
  pinMode(SW_pin, INPUT);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  digitalWrite(SW_pin, HIGH);
  display.clearDisplay();
  drawCourt();
  drawScore();

 // Initialize variables for ball and paddle updates
  ball_update = millis();
  paddle_update = ball_update;

 // Draw the initial game state
  drawCourt();
  drawScore();
}

// Main game loop
void loop() {
  // Variables to track whether an update is needed and current time
  bool update = false;
  unsigned long time = millis();

 // Update ball and paddle movement
  handleBallMovement(time);
  handlePaddleMovement(time);

 // Set update flag to true
  update = true;

 // Display the updated game state
  if (update) {
    drawScore();
    display.display();
    
    // Check for a joystick is press to restart the game
    if (digitalRead(SW_pin) == 0) {
      gameOver();
    }
  }
}

// Function to handle ball movement and collisions
void handleBallMovement(int ballTime) {
  // Check if the ball is reseted
  if (resetBall) {
     // Check if any of the players has won the game
    if (player1Score == maxScore || player2Score == maxScore) {
      gameOver();
    } else {
       // Initialize ball properties and direction
      display.fillScreen(BLACK);
      drawScore();
      drawCourt();
      ball_x = random(62, 66);
      ball_y = random(23, 33);
      do {
        ball_dir_x = random(-1, 2);
      } while (ball_dir_x == 0);
      do {
        ball_dir_y = random(-1, 2);
      } while (ball_dir_y == 0);

      // Reset the ball flag
      resetBall = false;
    }
  }
  
  // Update ball position if enough time has passed
  if (ballTime > ball_update) {
    uint8_t new_x = ball_x + ball_dir_x;
    uint8_t new_y = ball_y + ball_dir_y;

     // Handle wall collisions
    if (new_x == 0 || new_x == 127) {
      handleWallCollision(new_x);
    }

    // Handle top and bottom collisions, and update ball direction
    if (new_y == 0 || new_y == 63) {
      soundBounce();
      ball_dir_y = -ball_dir_y;
      new_y += ball_dir_y + ball_dir_y;
    }

    // Handle paddle collisions for player 2
    if (new_x == PLAYER2_X && new_y >= player2.getY() && new_y <= player2.getY() + player2.getHeight()) {
      handlePaddleCollision(-ball_dir_x, new_x);
       // Adjust ball direction based on the part of the paddle hit
      if (new_y <= (player2.getY() + (player2.getHeight()/3))) {
        ball_dir_y = -1;
      } else if (new_y <= (player2.getY() + (player2.getHeight()/3) * 2)) {
        ball_dir_y = 0;
      } else if (new_y <= (player2.getY() + player2.getHeight())) {
        ball_dir_y = 1;
      }
    }

    // Handle paddle collisions for player 1
    if (new_x == PLAYER_X && new_y >= player1.getY() && new_y <= player1.getY() + player1.getHeight()) {
      handlePaddleCollision(-ball_dir_x, new_x);
       // Adjust ball direction based on the part of the paddle hit
      if (new_y <= (player1.getY() + (player1.getHeight()/3))) {
        ball_dir_y = -1;
      } else if (new_y <= (player1.getY() + (player1.getHeight()/3) * 2)) {
        ball_dir_y = 0;
      } else if (new_y <= (player1.getY() + player1.getHeight())) {
        ball_dir_y = 1;
      }
    }

    // Erase the old ball position and draw the new one
    display.drawPixel(ball_x, ball_y, BLACK);
    display.drawPixel(new_x, new_y, WHITE);
    ball_x = new_x;
    ball_y = new_y;

    // Update the ball movement time
    ball_update += BALL_RATE;
  }
}

// Function to handle paddle movement
void handlePaddleMovement(int paddleTime) {
  // Update paddle positions at regular intervals
  if (paddleTime > paddle_update) {
    paddle_update += PADDLE_RATE;

    // Erase the old paddle positions
    player2.drawPlayer(display, PLAYER2_X, BLACK);
    player1.drawPlayer(display, PLAYER_X, BLACK);

    // Update player positions
    player2.updatePosition();
    player1.updatePosition();

    // Activate the wall ability for players
    player2.wallAbility();
    player1.wallAbility();

    // Draw the updated paddle positions
    player2.drawPlayer(display, PLAYER2_X, WHITE);
    player1.drawPlayer(display, PLAYER_X, WHITE);

  }
}

// Function to handle wall collisions and update scores
void handleWallCollision(uint8_t& new_x) {
  // Check if the ball hit the left wall
  if (new_x == 0) {
    // Increase player 2's score
    player1Score += 1;

    // Clear the display, play a sound, and reset the ball
    display.fillScreen(BLACK);
    soundPoint();
    resetBall = true;
    
    // Check if the ball hit the right wall
  } else if (new_x == 127) {
    // Increase player 1's score
    player2Score += 1;

    // Clear the display, play a sound, and reset the ball
    display.fillScreen(BLACK);
    soundPoint();
    resetBall = true;
  }

  // Reverse the ball's direction along the x-axis
  ball_dir_x = -ball_dir_x;
  new_x += ball_dir_x + ball_dir_x;
}

// Function to handle paddle collisions and update ball direction
void handlePaddleCollision(int dir, uint8_t& new_x) {
  // Play a sound for paddle collision
  soundBounce();

  // Update the ball direction along the x-axis
  ball_dir_x = dir;
  new_x += ball_dir_x + ball_dir_x;
}

// Function to draw the game court on the OLED display
void drawCourt() {
  // Draw a rectangular border on the display
  display.drawRect(0, 0, 128, 64, WHITE);
}

// Function to draw the player scores on the OLED display
void drawScore() {
  // Set the text size and color for the scores
  display.setTextSize(2);
  display.setTextColor(WHITE);

  // Draw player 2's score
  display.setCursor(45, 0);
  display.println(player2Score);

  // Draw player 1's score
  display.setCursor(75, 0);
  display.println(player1Score);
}

// Function to handle the game-over condition
void gameOver() {
  // Clear the display and display the winner
  display.fillScreen(BLACK);
  
   // Check which player won
  if (player1Score > player2Score) {
    display.setCursor(20, 15);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.print("Player 1");
    display.setCursor(40, 35);
    display.print("won");
  } else {
    display.setCursor(20, 15);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.print("Player 2");
    display.setCursor(40, 35);
    display.print("won");
  }
   // Delay for visual effect
  delay(100);

  // Display the result for 2 seconds
  display.display();
  delay(2000);

  // Reset scores and update times
  player2Score = player1Score = 0;

  unsigned long start = millis();
  while (millis() - start < 2000);
  ball_update = millis();
  paddle_update = ball_update;

  // Set the flag to reset the ball
  resetBall = true;
}

// Function to play a sound on ball bounce
void soundBounce() {
  tone(BEEPER, 500, 50);
}

// Function to play a sound on scoring a point
void soundPoint() {
  tone(BEEPER, 100, 50);
}
