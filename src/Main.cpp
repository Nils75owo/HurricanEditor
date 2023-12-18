#include <string>

#include "DX8Graphics.hpp"
#include "DX8Texture.hpp"
#include "Logdatei.hpp"
#include "Tileengine.hpp"
#include "Timer.hpp"
#include "ObjectList.hpp"

bool GameRunning = true;
std::string g_storage_ext = "/home/mia/code/Editor";

Logdatei Protokoll("logdatei.txt");
DirectGraphicsClass DirectGraphics;
TexturesystemClass Textures;
TimerClass Timer;
TileEngineClass TileEngine;
ObjectListClass ObjectList;

const Uint8 *KeyBuffer;
int NumberOfKeys;
#define KeyDown(key) (KeyBuffer[SDL_GetScancodeFromKey(key)] > 0)

int main() {
  DirectGraphics.Init(640, 480, 32, false);
  KeyBuffer = SDL_GetKeyboardState(&NumberOfKeys);

  TileEngine.LoadLevel("/home/mia/code/Editor/data/levels/jungle.map");

  Timer.SetMaxFPS(60);
  Timer.update();

  //ObjectList.LoadAllGraphics();

  while (GameRunning) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
        case SDL_QUIT:
          GameRunning = false;
          break;
        case SDL_WINDOWEVENT:
          if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
            DirectGraphics.ResizeToWindow();
          }
        default:
          break;
      }
    }

    if (KeyDown(SDLK_LEFT)) {
      TileEngine.ScrolltoX -= 20;
    }
    if (KeyDown(SDLK_RIGHT)) {
      TileEngine.ScrolltoX += 20;
    }
    if (KeyDown(SDLK_UP)) {
      TileEngine.ScrolltoY -= 20;
    }
    if (KeyDown(SDLK_DOWN)) {
      TileEngine.ScrolltoY += 20;
    }

    Timer.update();

    TileEngine.SpeedX = 500.0f;
    TileEngine.SpeedY = 500.0f;
    TileEngine.Zustand = TileStateEnum::SCROLLTO;
    TileEngine.UpdateLevel();

    DirectGraphics.ClearBackBuffer();

    DirectGraphics.SetColorKeyMode();

    TileEngine.CalcRenderRange();
    TileEngine.DrawBackground();

    TileEngine.DrawBackLevel();
    TileEngine.DrawFrontLevel();

    ObjectList.DrawAllObjects(TileEngine);

    DirectGraphics.SetColorKeyMode();

    TileEngine.DrawWater();
    TileEngine.DrawBackLevelOverlay();
    TileEngine.DrawOverlayLevel();
    TileEngine.DrawShadow();

    DirectGraphics.DisplayBuffer();

    Timer.wait();
  }
}
