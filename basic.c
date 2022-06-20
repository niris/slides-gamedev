#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>

void main(int argc, char **argv)
{
    if(argc<2)return printf("USAGE: %s sprite.bmp", argv[0]);
    SDL_Init(SDL_INIT_EVERYTHING); // SDL_WINDOWPOS_CENTERED
    SDL_Window *win = SDL_CreateWindow("tilemap demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 272, SDL_WINDOW_ALWAYS_ON_TOP);
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *player = SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP(argv[1]));
    SDL_SetRenderDrawColor(renderer, 0, 80, 40, 0);
    int player_x = 64, player_y = 64, player_direction = 0, player_frame = 0; // start position
    for (int dead = 0; !dead;)        // main loop
    {
        for (SDL_Event event; SDL_PollEvent(&event);) // event loop
        {
            dead |= event.type == SDL_QUIT; // closed from taskbar/window
            if (event.type != SDL_KEYDOWN)
                break;
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                dead |= 1;

            player_frame = player_frame == 0 ? 16 : 0;
            if (event.key.keysym.scancode == SDL_SCANCODE_DOWN){
                player_direction = 0;
                player_y += 2;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_UP){
                player_direction = 32;
                player_y -= 2;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_LEFT){
                player_direction = 64;
                player_x -= 2;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT){
                player_direction = 96;
                player_x += 2;
            }
			if (event.key.keysym.scancode == SDL_SCANCODE_F)
				SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN) ^ SDL_WINDOW_FULLSCREEN);
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, player,
                       &(SDL_Rect){0, player_direction + player_frame, 16, 16},                // position in BMP
                       &(SDL_Rect){player_x, player_y, 16, 16}); // position on screen
        SDL_RenderPresent(renderer);
    }
}