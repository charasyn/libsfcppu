#include <assert.h>
#include <stdint.h>
#include <SDL.h>
#include <iostream>
#include <fstream>

typedef int8_t int8;
typedef uint8_t uint8;
typedef uint16_t uint16;

#include "data-interop.hpp"

const int ImgW = 512;
const int ImgH = 480;
const int WindowScale = 1;

extern "C" {
    bool libsfcppu_init();
    uint16 const * libsfcppu_drawFrame(SnesFrameData const * ppuState);
}

void drawFrame(byte* buffer, int pitch, const SnesFrameData* d) {
    assert(ImgW == 512);
    assert(ImgH == 480);
    assert(pitch == ImgW * 4);
    ushort const * rawdata = libsfcppu_drawFrame(d);
    for (int y = 0; y < ImgH; y += 1) {
        byte* pxptr = buffer + pitch * y;
        for (int x = 0; x < ImgW; x += 1) {
            ushort px = rawdata[y * 512 + x];
            /* R */ pxptr[0] = ((px >>  0) & 31) << 3;
            /* G */ pxptr[1] = ((px >>  5) & 31) << 3;
            /* B */ pxptr[2] = ((px >> 10) & 31) << 3;
            pxptr += 4;
        }
    }
}

extern "C" int SDL_main(int argc, char ** argv) {
    std::ofstream logger("log.txt", std::ios::trunc);
    logger << "Hello world!" << std::endl;
    if(!libsfcppu_init()) {
        logger << "Error initializing DrawFrame!" << std::endl;
        return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        logger << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    const int windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    SDL_Window* appWin = SDL_CreateWindow(
        "SDL Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        ImgW * WindowScale,
        ImgH * WindowScale,
        windowFlags
    );
    if(!appWin) {
        logger << "Error creating SDL window: " << SDL_GetError() << std::endl;
        return 1;
    }

    const int rendererFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer* renderer = SDL_CreateRenderer(
        appWin, -1, rendererFlags
    );
    if(!renderer) {
        logger << "Error creating SDL renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Texture* drawTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_BGR888,
        SDL_TEXTUREACCESS_STREAMING,
        ImgW,
        ImgH
    );
    if(!drawTexture) {
        logger << "Error creating SDL texture: " << SDL_GetError() << std::endl;
        return 1;
    }

    bool run = true, pause = false;
    SnesFrameData frameData = {};
    frameData.INIDISP = 0x0f;
    frameData.BGMODE = 1;
    frameData.MOSAIC = 0;
    frameData.TM = 0;
    frameData.TS = 0;
    frameData.TMW = 0;
    frameData.TSW = 0;
    frameData.cgram[0] = 0x1fff;
    {
        int i;
        for (i = 0; i < 16; i += 1) {
            frameData.hdmaData[i].vcounter = i + 10;
            frameData.hdmaData[i].addr = 0x00;
            frameData.hdmaData[i].value = 15 - i;
        }
        for (; i < 32; i += 1) {
            frameData.hdmaData[i].vcounter = i + 10;
            frameData.hdmaData[i].addr = 0x00;
            frameData.hdmaData[i].value = i - 16;
        }
        frameData.numHdmaWrites = i;
    }
    
    logger << "Beginning SDL loop..." << std::endl;

    int lastTime;
    while(run) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                run = false;
            }

            if (event.type == SDL_KEYDOWN) {
                pause = !pause;
            }
        }

        lastTime = SDL_GetTicks();

        byte* drawBuffer;
        int drawPitch;
        SDL_LockTexture(drawTexture, NULL, (void**)&drawBuffer, &drawPitch);
        drawFrame(drawBuffer, drawPitch, &frameData);
        SDL_UnlockTexture(drawTexture);

        SDL_SetRenderDrawColor(renderer, 120, 140, 230, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, drawTexture, NULL, NULL);
        SDL_RenderPresent(renderer);

        int drawTime = SDL_GetTicks() - lastTime;
        if(drawTime < 16) {
            SDL_Delay(16 - drawTime);
        }
    }

    // Close and destroy the texture
    if(drawTexture) SDL_DestroyTexture(drawTexture);
    // Close and destroy the renderer
    if(renderer) SDL_DestroyRenderer(renderer);
    // Close and destroy the window
    if(appWin) SDL_DestroyWindow(appWin);
    SDL_Quit();

    return 0;
}
