/*#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

void SDLWindow(){
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("SDL_Error: %s\n", SDL_GetError());
      return;
  }
  int width = 800;
  int height = 800;
  SDL_Window *window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
  SDL_Surface *surface = SDL_getWindowSurface(window);
  SDL_UpdateWindowSurface(window);

  int keep_window_open = 1;
  SDL_Rect rect;
  rect.x = width/2;
  rect.y = height/2;
  rect.width = width/4;
  rect.height = height/4;

  while(keep_window_open){
    SDL_Event e;
    while(SDL_PollEvent(&e) > 0){
      switch(e.type){
        case SDL_QUIT:
          keep_window_open = 0;
          break;
      }
      SDL_UpdateWindowSurface(window);
    }
  }
}
*/
