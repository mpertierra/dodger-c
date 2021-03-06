#include <stdio.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#define MAX_NUM_OF_OBSTACLES 25
#define INIT_OBSTACLE_GEN_PROBABILITY 0.001
#define MIN_OBSTACLE_GEN_PROBABILITY 0.001
#define MAX_OBSTACLE_GEN_PROBABILITY 0.05
#define OBSTACLE_GEN_PROB_DECR_MULT 0.50
#define OBSTACLE_GEN_PROB_INCR_MULT 1.50

#define MIN_OBSTACLE_SIZE 0.05
#define MAX_OBSTACLE_SIZE 0.1
#define MIN_OBSTACLE_SPEED 0.001
#define MAX_OBSTACLE_SPEED 0.01

typedef SDL_Texture* Texture;

SDL_Window *window = NULL;

const unsigned window_x = 800;
const unsigned window_y = 600;

const unsigned window_start_x = 200;
const unsigned window_start_y = 100;

const char* window_name = "6.179 Final Project - Dodger";
int game_over = 0;
int score = 0;
int top_score = 0;

// Player position, size, and speed
double player_x = 0.5;
double player_y = 1.0;
const double player_height = 0.05;
const double player_width  = 0.05;
const double player_speed = 0.01;

// Player direction
int player_up = 0;
int player_down = 0;
int player_left = 0;
int player_right = 0;

// Obstacle definition
typedef struct obstacle_t {
   double x;
   double y;
   double size;
   double speed;
} Obstacle;

// Obstacle storage and generation-probability
int obstacle_count = 0;
double obstacle_gen_prob = INIT_OBSTACLE_GEN_PROBABILITY;
Obstacle * obstacles[MAX_NUM_OF_OBSTACLES] = {NULL};

SDL_Renderer *renderer;

Texture background_image, player_image, obstacle_image;
Mix_Music *background_music = NULL;
Mix_Chunk *collision_sound_effect = NULL;
TTF_Font *font = NULL;

const SDL_MessageBoxButtonData buttons[] = {
   { /* .flags, .buttonid, .text */        0, 0, "No" },
   { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" }
};

SDL_MessageBoxData messageboxdata = {
   SDL_MESSAGEBOX_INFORMATION, /* .flags */
   NULL, /* .window */
   "Game over :(", /* .title */
   "Game over :(", /* .message */
   SDL_arraysize(buttons), /* .numbuttons */
   buttons, /* .buttons */
   NULL /* .colorScheme */
};

double genRandDouble(double min, double max) {
   // assert (min <= max);
   double rand_double = (double)rand() / (double)RAND_MAX;
   return min + rand_double*(max - min);
}

void movePlayer() {
   player_x+= (player_left - player_right) * player_speed;
   if (player_x > 1.0 - player_width) {
      player_x = 1.0 - player_width;
   } else if (player_x < 0) {
      player_x = 0;
   }
   
   player_y+= (player_up - player_down) * player_speed;
   if (player_y > 1.0 - player_height) {
      player_y = 1.0 - player_height;
   } else if (player_y < 0) {
      player_y = 0;
   }
}

void moveObstacles() {
   int i;
   for (i=0; i < MAX_NUM_OF_OBSTACLES; i++) {
      Obstacle* obs = obstacles[i];
      if (obs == NULL) {
         continue;
      }
      obs->y += obs->speed;
      if (obs->y > 1.0 - obs->size) {
         free(obs);
         obstacles[i] = NULL;
         obstacle_count -= 1;
         score += 1;
      }
   }
   return;
}

void createObstacle() {
   int i;
   for (i=0; i < MAX_NUM_OF_OBSTACLES; i++) {
      Obstacle* obs = obstacles[i];
      if (obs != NULL) {
         continue;
      }
      obs = malloc(sizeof(*obs));
      obs->x = genRandDouble(0.0, 1.0);
      obs->y = 0;
      obs->size = genRandDouble(MIN_OBSTACLE_SIZE, MAX_OBSTACLE_SIZE);
      obs->speed = genRandDouble(MIN_OBSTACLE_SPEED, MAX_OBSTACLE_SPEED);
      obstacles[i] = obs;
      obstacle_count += 1;
      return;
   }
}

void tryCreateObstacle() {
   if (obstacle_count < MAX_NUM_OF_OBSTACLES && genRandDouble(0.0, 1.0) < obstacle_gen_prob) {
      createObstacle();
      obstacle_gen_prob *= OBSTACLE_GEN_PROB_DECR_MULT;
      if (obstacle_gen_prob < MIN_OBSTACLE_GEN_PROBABILITY) {
         obstacle_gen_prob = MIN_OBSTACLE_GEN_PROBABILITY;
      }
   } else {
      obstacle_gen_prob *= OBSTACLE_GEN_PROB_INCR_MULT;
      if (obstacle_gen_prob > MAX_OBSTACLE_GEN_PROBABILITY) {
         obstacle_gen_prob = MAX_OBSTACLE_GEN_PROBABILITY;
      }
   }
}

void freeObstacles() {
   int i;
   for (i=0; i<MAX_NUM_OF_OBSTACLES; i++) {
      if (obstacles[i] != NULL) {
         free(obstacles[i]);
         obstacles[i] = NULL;
      }
   }
}

int pairCollision(int topA, int bottomA, int leftA, int rightA, int topB, int bottomB, int leftB, int rightB) {
   if (bottomA <= topB) {
      return 0;
   }
   if (topA >= bottomB) {
      return 0;
   }
   if (rightA <= leftB) {
      return 0;
   }
   if (leftA >= rightB) {
      return 0;
   }
   return 1;
}


int checkCollision(){
   int top = (int) (player_y*window_y);
   int bottom = (int) (player_y*window_y + player_height*window_y);
   int left = (int) (player_x*window_x);
   int right = (int) (player_x*window_x + player_width*window_x);
   int i;
   for (i=0; i<MAX_NUM_OF_OBSTACLES; i++) {
      Obstacle* obs = obstacles[i];
      if (obs == NULL) {
         continue;
      }
      int x = (int) ((obs->x)*window_x);
      int y = (int) ((obs->y)*window_y);
      int size = (int) ((obs->size)*((window_x+window_y)/2.0));
      if (pairCollision(top, bottom, left, right, y, y + size, x, x + size)) {
         return 1;
      }
   }
   return 0;
}

Texture loadImage(char* image){
   SDL_Surface *loadedImage = IMG_Load(image);
   if(!loadedImage) {
      printf("Failed to load image: %s\n", SDL_GetError() );
      SDL_Quit();
      exit(1);
   }
   Texture texture = SDL_CreateTextureFromSurface(renderer, loadedImage);
   SDL_FreeSurface(loadedImage);
   if(!texture) {
      printf("Failed to create texture: %s\n", SDL_GetError() );
      SDL_Quit();
      exit(1);
   }
   return texture;
}

void displayTexture(Texture t, unsigned x, unsigned y, unsigned width, unsigned height, SDL_RendererFlip flip){
   SDL_Rect tex_size;
   tex_size.x = 0;
   tex_size.y = 0;
   SDL_Rect toplace;
   toplace.x = x;
   toplace.y = y;
   toplace.w = width;
   toplace.h = height;
   SDL_QueryTexture(t, NULL, NULL, &tex_size.w, &tex_size.h);
   SDL_RenderCopyEx(renderer, t, &tex_size, &toplace, 0, NULL, flip);
}

void displayPlayer() {
   displayTexture(player_image, (int)(player_x*window_x), (int)(player_y*window_y), (int)(player_width*window_x), (int)(player_height*window_y), SDL_FLIP_NONE);
}

void displayObstacle(Obstacle* obs) {
   int x = (int) ((obs->x)*window_x);
   int y = (int) ((obs->y)*window_y);
   int size = (int) ((obs->size)*((window_x+window_y)/2.0));
   displayTexture(obstacle_image, x, y, size, size, SDL_FLIP_NONE);
}

void displayObstacles() {
   int i;
   for (i=0; i < MAX_NUM_OF_OBSTACLES; i++) {
      Obstacle* obs = obstacles[i];
      if (obs == NULL) {
         continue;
      }
      displayObstacle(obs);
   }
}

void displayScore() {
   if (font == NULL) {
      font = TTF_OpenFont("fonts/OpenSans-Regular.ttf", 24);
   }
   char score_text[128];
   snprintf(score_text, sizeof(score_text), "Score: %d  Top Score: %d", score, top_score);
   SDL_Color color = {0, 0, 0};
   SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, score_text, color);
   SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
   SDL_FreeSurface(surfaceMessage);
   displayTexture(Message, 10, 10, 200, 100, SDL_FLIP_NONE);
}

int gameOver() {
   char score_text[128];
   snprintf(score_text, sizeof(score_text), "Game Over! Your score is %d. Try again?", score);
   messageboxdata.message = score_text;

   int buttonid;
   if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0 || buttonid == -1 || buttonid == 0) {
      Mix_Quit();
      TTF_Quit();
      IMG_Quit();
      SDL_Quit();
      freeObstacles();
      return 1;
   }

   return 0;
}

void resetGame() {
   score = 0;
   player_x = 0.5;
   player_y = 1.0;
   freeObstacles();
   obstacle_count = 0;
   obstacle_gen_prob = INIT_OBSTACLE_GEN_PROBABILITY;
}

void playGame() {
   FILE *fp;
   fp = fopen("top_score", "r");
   fscanf(fp, "%d", &top_score);
   fclose(fp);

   //Our event structure
   SDL_Event e;
   int quit = 0;
   while (!quit){
      while (SDL_PollEvent(&e)){
         if (e.type == SDL_QUIT) quit = 1;
         else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            switch(e.key.keysym.sym){
               case SDLK_UP:
                  player_down = (e.type == SDL_KEYDOWN); break;
               case SDLK_DOWN:
                  player_up = (e.type == SDL_KEYDOWN); break;
               case SDLK_LEFT:
                  player_right = (e.type == SDL_KEYDOWN); break;
               case SDLK_RIGHT:
                  player_left = (e.type == SDL_KEYDOWN); break;
            }
         }
      }

      // Update player and obstacles
      movePlayer();
      tryCreateObstacle();
      moveObstacles();

      // Check for collisions
      if (checkCollision()) {
         Mix_PlayChannel( -1, collision_sound_effect, 0 );
         game_over = 1;
         break;
      }

      SDL_RenderClear(renderer);

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_Rect rectangle;
      rectangle.x = 0;
      rectangle.y = 0;
      rectangle.w = window_x;
      rectangle.h = window_y;
      SDL_RenderFillRect(renderer, &rectangle);

      displayTexture(background_image, 200, 200, 200, 200, SDL_FLIP_NONE);
      displayPlayer();
      displayObstacles();
      displayScore();
      SDL_RenderPresent(renderer);
   }

   // Save top score if necessary
   if (score > top_score) {
      top_score = score;
      fp = fopen("top_score", "w");
      fprintf(fp, "%d", top_score);
      fclose(fp);
   }

   if (game_over) {
      if (!gameOver()) {
         game_over = 0;
         resetGame();
         playGame();
      };
   } else {
      Mix_Quit();
      TTF_Quit();
      IMG_Quit();
      SDL_Quit();
      freeObstacles();
   }
}

int main(){
   if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
      printf("SDL_Init failed: %s\n", SDL_GetError());
      return 1;
   }

   window = SDL_CreateWindow(window_name,window_start_x,window_start_y, window_x,window_y,SDL_WINDOW_SHOWN);
   if (!window){
      printf("Window creation failed: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
   }

   renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
   if (!renderer){
      printf("Renderer creation failed: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
   }

   if (Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1) {
        printf("Audio creation failed %s\n", Mix_GetError());
        return 1;
   }

   background_music = Mix_LoadMUS("audio/background_music.wav");
   Mix_PlayMusic(background_music, -1);
   if(!background_music){
        printf("Could not open sound effect %s\n", Mix_GetError());
   }

   collision_sound_effect = Mix_LoadWAV("audio/collision_sound_effect.wav");
   if(!collision_sound_effect){
        printf("Could not open sound effect %s\n", Mix_GetError());
   }

   if (TTF_Init() == -1) {
      printf("TTF_Init: %s\n", TTF_GetError());
      return 1;
   }

   // Load Images for background, player and obstacles
   background_image = loadImage("images/background.png");
   player_image = loadImage("images/player.jpg");
   obstacle_image = loadImage("images/obstacle.jpg");

   playGame();

   return 0;
}
