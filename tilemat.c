#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#define MAX(A, B) (A > B ? A : B)
#define MIN(A, B) (A < B ? A : B)
#define log(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
struct {
	SDL_Renderer *render;
	SDL_Window *window;
	int width, height;
} view;
struct {
	uint8_t *map;
	size_t cols, rows, total;
	int x, y;  // player position (pixel)
} tilemap;
struct {
	SDL_Texture *texture;
	int size, x, y;
} tileset;
struct {
	SDL_Texture *texture;
	int col, row, width, height;  // we *have* to store x,y since it can't be guessed from world-wrapped coord
} layers[1];
struct {
	SDL_Texture *texture;
	int size, dir, step, speed;
} player;

SDL_Texture *CreateTextureFromBMP(SDL_Renderer *rdr, const char *path) {
	SDL_Surface *bmp = SDL_LoadBMP(path);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(rdr, bmp);
	SDL_FreeSurface(bmp);
	return texture;
}
// inspired by https://github.com/vivi168/tilemap_demo/blob/master/map_utils.asm
int map_init_at(int x, int y) {
	SDL_SetRenderTarget(view.render, layers[0].texture);
	for (int map_col = 0; map_col < tilemap.cols / tileset.size; map_col++) {
		for (int map_row = 0; map_row < tilemap.rows / tileset.size; map_row++) {
			int tile = map_col + (map_row * tilemap.cols);
			int layer_col = map_col;
			int layer_row = map_row;
			SDL_RenderCopy(view.render, tileset.texture,
			               &(SDL_Rect){0, tilemap.map[tile % tilemap.total] * tileset.size, tileset.size, tileset.size},
			               &(SDL_Rect){layer_col * tileset.size, layer_row * tileset.size, tileset.size, tileset.size});
		}
	}
	SDL_SetRenderTarget(view.render, NULL);
}
int map_move_right(int off_x) {
	SDL_SetRenderTarget(view.render, layers[0].texture);
	// how many cols need to be updated
	for (int map_col = tilemap.x / tileset.size; map_col < (tilemap.x + off_x) / tileset.size; map_col++) {
		// layer col update on the side we are moving
		int layer_col = (map_col + (view.width / tileset.size)) % (layers[0].width / tileset.size);
		for (int layer_row = 0; layer_row < layers[0].height / tileset.size; layer_row++) {
			int map_row = tilemap.y / tileset.size + layer_row;
			int tile = map_col + (map_row * tilemap.cols);
			log("update map_col %i in layer_col %i / map_row %i in layer_row %i\n", map_col, layer_col, map_row, layer_row);
			SDL_RenderCopy(view.render, tileset.texture,
			               &(SDL_Rect){0, tilemap.map[tile % tilemap.total] * tileset.size, tileset.size, tileset.size},
			               &(SDL_Rect){layer_col * tileset.size, layer_row * tileset.size, tileset.size, tileset.size});
		}
	}
	SDL_SetRenderTarget(view.render, NULL);
	tilemap.x = (tilemap.x + off_x) % (tilemap.cols * tileset.size);
	return 1;
}
int map_move_x(int off_x) {  // TODO: BUG when going backward ??
	SDL_SetRenderTarget(view.render, layers[0].texture);
	// the range of rows to update (map + off) or (map - off) depending if going up or down => use MAX()
	for (int map_col = (tilemap.x + MIN(0, off_x)) / tileset.size; map_col < (tilemap.x + MAX(0, off_x)) / tileset.size; map_col++) {
		int layer_col = map_col % (layers[0].width / tileset.size);
		for (int layer_row = 0; layer_row < layers[0].height / tileset.size; layer_row++) {
			int map_row = tilemap.y / tileset.size + layer_row;
			int tile = map_col + (map_row * tilemap.cols);
			log("update map_col %i in layer_col %i / map_row %i in layer_row %i\n", map_col, layer_col, map_row, layer_row);
			SDL_RenderCopy(view.render, tileset.texture,
			               &(SDL_Rect){0, tilemap.map[tile % tilemap.total] * tileset.size, tileset.size, tileset.size},
			               &(SDL_Rect){layer_col * tileset.size, layer_row * tileset.size, tileset.size, tileset.size});
		}
	}
	SDL_SetRenderTarget(view.render, NULL);
	tilemap.x = (tilemap.x + off_x) % (tilemap.cols * tileset.size);
	return 1;
}
int map_move_y(int off_y) {  // TODO: BUG when going backward ??
	SDL_SetRenderTarget(view.render, layers[0].texture);
	// the range of rows to update (map + off) or (map - off) depending if going up or down => use MAX()
	for (int map_row = (tilemap.y + MIN(0, off_y)) / tileset.size; map_row < (tilemap.y + MAX(0, off_y)) / tileset.size; map_row++) {
		int layer_row = map_row % (layers[0].height / tileset.size);
		for (int layer_col = 0; layer_col < layers[0].width / tileset.size; layer_col++) {
			int map_col = (tilemap.x / tileset.size + layer_col) % tilemap.cols;
			int tile = map_col + (map_row * tilemap.cols);
			log("update map_row %i in layer_row %i / map_col %i in layer_col %i\n", map_row, layer_row, map_col, layer_col);
			SDL_RenderCopy(view.render, tileset.texture,
			               &(SDL_Rect){0, tilemap.map[tile % tilemap.total] * tileset.size, tileset.size, tileset.size},
			               &(SDL_Rect){layer_col * tileset.size, layer_row * tileset.size, tileset.size, tileset.size});
		}
	}
	SDL_SetRenderTarget(view.render, NULL);
	tilemap.y = (tilemap.y + off_y) % (tilemap.rows * tileset.size);
	return 1;
}

int main(int argc, char **argv) {
	if (argc < 4)
		return log("USAGE: player sprite.bmp tileset.bmp tilemap.bin tilemap_width");
	SDL_Init(SDL_INIT_EVERYTHING);
	view.width = 640;
	view.height = 480;
	view.window = SDL_CreateWindow("tilemap demo", 2000, 0, view.width, view.height, SDL_WINDOW_ALWAYS_ON_TOP);
	view.render = SDL_CreateRenderer(view.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	tileset.texture = CreateTextureFromBMP(view.render, argv[2]);  // convert -depth 8 -size 16x544 rgba:zelda.rgba rgba.png
	SDL_QueryTexture(tileset.texture, NULL, NULL, &tileset.size, NULL);
	layers[0].width = view.width + (view.width % tileset.size) + tileset.size;
	layers[0].height = view.height + (view.height % tileset.size) + tileset.size;
	layers[0].texture = SDL_CreateTexture(view.render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, layers[0].width, layers[0].height);
	struct stat sb;
	stat(argv[3], &sb);
	tilemap.map = malloc(sb.st_size);
	FILE *m = fopen(argv[3], "rb");
	fread(tilemap.map, sb.st_size, 1, m);
	//	tilemap.map = (uint8_t *)SDL_LoadFile(argv[2], &tilemap.rows);
	tilemap.cols = atoi(argv[4]);
	tilemap.rows = sb.st_size / tilemap.cols;
	tilemap.total = tilemap.rows * tilemap.cols;
	player.texture = CreateTextureFromBMP(view.render, argv[1]);
	SDL_QueryTexture(player.texture, NULL, NULL, &player.size, NULL);

	//	map_move(view.width, view.height);
	//	map_move(-view.width, -view.height);
	//	map_move(100, 0);
	map_init_at(0,0);
	for (int dead = 0, debug = 0; !dead;) {            // mainloop (shall SDL_Delay(1000/60) ? )
		for (SDL_Event event; SDL_PollEvent(&event);) {  // event loop
			dead |= event.type == SDL_QUIT;                // closed from OS
			if (event.type != SDL_KEYDOWN) break;
			dead |= event.key.keysym.scancode == SDL_SCANCODE_ESCAPE;
			player.speed = event.key.keysym.mod & KMOD_SHIFT ? 40 : 1;
			if (event.key.keysym.scancode == SDL_SCANCODE_D)
				debug ^= 1;
			if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
				player.dir = 0, player.step = player.step ^ !!map_move_y(+player.speed);
			if (event.key.keysym.scancode == SDL_SCANCODE_UP)
				player.dir = 1, player.step = player.step ^ !!map_move_y(-player.speed);
			if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
				player.dir = 2, player.step = player.step ^ !!map_move_x(-player.speed);
			if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
				player.dir = 3, player.step = player.step ^ !!map_move_right(+player.speed);
			if (event.key.keysym.scancode == SDL_SCANCODE_R)
				player.dir = 0, player.step = player.step ^ !!(map_move_x(-tilemap.x) | map_move_y(-tilemap.y));
			if (event.key.keysym.scancode == SDL_SCANCODE_F)
				SDL_SetWindowFullscreen(view.window, (SDL_GetWindowFlags(view.window) & SDL_WINDOW_FULLSCREEN) ^ SDL_WINDOW_FULLSCREEN);
		}
		// SDL_RenderClear(view.render);
		/*
		draw our tilemap using 4 surfaces to do H+V wraping on the layers buffer
		each surface has it static anchor point: #
		the variable point in the @ which is [tilemap.x % view.width, tilemap.y % view.height]
		layerX:       screen:
		#_________#   #_________#
		|  A  | B |   |  A  | B |
		|_____$___|   |_____@___|
		|  C  | D |   |  C  | D |
		#_____|___#   #_____|___#
		*/
		SDL_Point var = {
		    view.width - (tilemap.x % view.width),
		    view.height - (tilemap.y % view.height),
		};
		SDL_Rect coord[] = {
		    // surface A: anchor to top-left corner [0,0, var.x,var.y]
		    {(tilemap.x) % view.width, (tilemap.y % view.height), var.x, var.y},
		    {0, 0, var.x, var.y},
		    // surface B: anchor to top-right corner [var.x,0, view.width-var.x,var.y]
		    {(tilemap.x + var.x) % view.width, tilemap.y % view.height, view.width - var.x, var.y},
		    {var.x, 0, view.width - var.x, var.y},
		    // surface C: anchor to bottom-left [0,var.y, var.x,view.height-var.y]
		    {(tilemap.x) % view.width, (tilemap.y + var.y) % view.height, var.x, view.height - var.y},
		    {0, var.y, var.x, view.height - var.y},
		    // surface D: anchor to bottom-right [var.x,var.y ,view.width,view.height]
		    {(tilemap.x + var.x) % view.width, (tilemap.y + var.y) % view.height, view.width - var.x, view.height - var.y},
		    {var.x, var.y, view.width - var.x, view.height - var.y},
		};
		if (debug) {
			SDL_RenderCopy(view.render, layers[0].texture, NULL, NULL);
			SDL_SetRenderDrawColor(view.render, 255, 0, 0, 128);
			for (int i = 0; i < sizeof(coord) / sizeof(*coord); i += 2) {
				SDL_RenderDrawRect(view.render, &coord[i]);
			}
			SDL_RenderPresent(view.render);
			continue;
		}
		//        log("x:%i y:%i => %i %i\n",tilemap.x, tilemap.y, screen_x, screen_y);
		for (int i = 0; i < sizeof(coord) / sizeof(*coord); i += 2) {
			SDL_RenderCopy(view.render, layers[0].texture, &coord[i], &coord[i + 1]);
			// log("src x:%3i y:%3i w:%3i h:%3i\n", coord[i + 0].x, coord[i + 0].y, coord[i + 0].w, coord[i + 0].h);
			// log("dst x:%3i y:%3i w:%3i h:%3i\n", coord[i + 1].x, coord[i + 1].y, coord[i + 1].w, coord[i + 1].h);
		}
		// log("\n");
		//   render player in the center => TODO not in center if at border of screen
		SDL_RenderCopy(view.render, player.texture,
		               &(SDL_Rect){0, (player.dir * 2 + player.step) * player.size, player.size, player.size},
		               &(SDL_Rect){(view.width - player.size) / 2, (view.height - player.size) / 2, 16, 16});
		SDL_RenderPresent(view.render);
	}
	SDL_DestroyTexture(layers[0].texture);
	SDL_DestroyTexture(player.texture);
	SDL_DestroyRenderer(view.render);
	SDL_DestroyWindow(view.window);
	SDL_Quit();
	return log("Bye bye !\n");
}