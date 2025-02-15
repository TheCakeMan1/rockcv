#include "gui.h"

int8_t create_win(const int SCREEN_WIDTH, const int SCREEN_HEIGHT){
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Ошибка инициализации SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Создание окна
    SDL_Window* window = SDL_CreateWindow("FFmpeg + SDL2",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Ошибка создания окна: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Создание рендерера
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Ошибка создания рендерера: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Создание текстуры для видео (YUV420)
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!texture) {
        printf("Ошибка создания текстуры: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
}