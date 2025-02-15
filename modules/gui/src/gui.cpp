#include "gui.h"

int he(){
    printf("HI\n");
    return 0;
}

int8_t create_win(const int SCREEN_WIDTH, const int SCREEN_HEIGHT){


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Создание окна
    SDL_Window* window = SDL_CreateWindow("FFmpeg + SDL2",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Ошибка создания окна: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Создание рендерера
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Ошибка создания рендерера: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Создание текстуры для видео (YUV420)
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!texture) {
        std::cerr << "Ошибка создания текстуры: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Создание буфера кадра FFmpeg (пример)
    AVFrame* frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = SCREEN_WIDTH;
    frame->height = SCREEN_HEIGHT;
    av_frame_get_buffer(frame, 32);  // Выделяем буфер

    // Генерация тестового изображения (градиент)
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            frame->data[0][y * frame->linesize[0] + x] = x % 256;  // Y-плоскость (яркость)
        }
    }

    // Обновление текстуры данными из AVFrame
    SDL_UpdateYUVTexture(texture, nullptr,
                         frame->data[0], frame->linesize[0],  // Y-плоскость
                         frame->data[1], frame->linesize[1],  // U-плоскость
                         frame->data[2], frame->linesize[2]); // V-плоскость

    // Основной цикл рендеринга
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // Задержка для 60 FPS
    }

    // Освобождение ресурсов
    av_frame_free(&frame);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;


    // // Инициализация SDL
    // if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    //     std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
    //     return -1;
    // }

    // // Создание окна
    // SDL_Window* window = SDL_CreateWindow("Пустое окно SDL2",
    //                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    //                                       800, 600, SDL_WINDOW_SHOWN);
    // if (!window) {
    //     std::cerr << "Ошибка создания окна: " << SDL_GetError() << std::endl;
    //     SDL_Quit();
    //     return -1;
    // }

    // // Основной цикл обработки событий
    // bool running = true;
    // SDL_Event event;
    // while (running) {
    //     while (SDL_PollEvent(&event)) {
    //         if (event.type == SDL_QUIT) { // Закрытие окна
    //             running = false;
    //         }
    //     }
    //     SDL_Delay(16); // 60 FPS (примерное значение)
    // }

    // // Очистка ресурсов
    // SDL_DestroyWindow(window);
    // SDL_Quit();
    // return 0;
}