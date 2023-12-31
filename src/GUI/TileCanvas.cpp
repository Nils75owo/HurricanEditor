#include "TileCanvas.hpp"

#include <epoxy/gl.h>
#include <wx/event.h>
#include <wx/glcanvas.h>
#include <wx/window.h>
#include <wx/wx.h>

#include "DX8Graphics.hpp"
#include "DX8Sprite.hpp"
#include "GUI/App.hpp"
#include "ObjectList.hpp"
#include "Tileengine.hpp"
#include "Timer.hpp"

BEGIN_EVENT_TABLE(TileCanvas, wxGLCanvas)
EVT_PAINT(TileCanvas::PaintIt)
END_EVENT_TABLE()

TileCanvas::TileCanvas(wxWindow* parent)
    : wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, 0,
                 wxT("GLCanvas")) {
  DirectGraphics.Init();

  context = new wxGLContext(this);
  context->SetCurrent(*this);

  if (!DirectGraphics.SetDeviceInfo()) {
    Protokoll << "\n-> SetDeviceInfo failed!\n" << std::endl;
    GameRunning = false;
  }

  Protokoll << "\n-> OpenGL init successful!\n" << std::endl;

  TileEngine.LoadSprites();
  TileEngine.LoadLevel(g_storage_ext + "/data/levels/jungle.map");

  editMode = EDIT_MODE_VIEW;

  Bind(wxEVT_SIZE, [&](wxSizeEvent& evt) {
    auto size = this->GetSize();
    DirectGraphics.ResizeToWindow(size.GetWidth(), size.GetHeight());
    evt.Skip();
  });

  mouseLeft = false;
  mouseRight = false;
  Bind(wxEVT_LEFT_DOWN, [&](wxMouseEvent& evt) {
    mouseLeft = true;
    if (evt.AltDown()) {
      TryRemove();
    } else {
      TryPlace();
    }
    evt.Skip();
  });
  Bind(wxEVT_LEFT_UP, [&](wxMouseEvent& evt) {
    mouseLeft = false;
    evt.Skip();
  });
  Bind(wxEVT_RIGHT_DOWN, [&](wxMouseEvent& evt) {
    mouseRight = true;
    evt.Skip();
  });
  Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& evt) {
    mouseRight = false;
    evt.Skip();
  });

  Bind(wxEVT_MOTION, [&](wxMouseEvent& evt) {
    if (evt.Dragging() && mouseRight) {
      auto delta = mousePos - evt.GetPosition();

      TileEngine.XOffset += delta.x;
      TileEngine.YOffset += delta.y;
    }

    if (mouseLeft) {
      if (evt.AltDown()) {
        TryRemove();
      } else {
        TryPlace();
      }
    }

    mousePos = evt.GetPosition();
    evt.Skip();
  });
  Bind(wxEVT_MOUSEWHEEL, [&](wxMouseEvent& evt) {
    if (evt.GetWheelRotation() > 0) {
      TileEngine.ZoomBy(0.1);
    } else {
      TileEngine.ZoomBy(-0.1);
    }
    evt.Skip();
  });
}

void TileCanvas::PlaceBlock(wxPoint pos, LevelTileStruct tile) {
  TileEngine.Tiles[pos.x][pos.y] = tile;
}

void TileCanvas::PlaceTileFront(wxPoint pos, unsigned char art,
                                unsigned char tileSet, uint32_t flags) {
  TileEngine.Tiles[pos.x][pos.y].FrontArt = art;
  TileEngine.Tiles[pos.x][pos.y].TileSetFront = tileSet;
  TileEngine.Tiles[pos.x][pos.y].Block = flags;
}
void TileCanvas::PlaceTileBack(wxPoint pos, unsigned char art,
                               unsigned char tileSet, uint32_t flags) {
  TileEngine.Tiles[pos.x][pos.y].BackArt = art;
  TileEngine.Tiles[pos.x][pos.y].TileSetBack = tileSet;
  TileEngine.Tiles[pos.x][pos.y].Block = flags;
}

void TileCanvas::RemoveTileFront(wxPoint pos) {
  TileEngine.Tiles[pos.x][pos.y].FrontArt = 0;
  TileEngine.Tiles[pos.x][pos.y].TileSetFront = 0;
  if (TileEngine.Tiles[pos.x][pos.y].BackArt == 0) {
    TileEngine.Tiles[pos.x][pos.y].Block = 0;
  } else {
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_VERDECKEN;
  }
}
void TileCanvas::RemoveTileBack(wxPoint pos) {
  TileEngine.Tiles[pos.x][pos.y].BackArt = 0;
  TileEngine.Tiles[pos.x][pos.y].TileSetBack = 0;
  if (TileEngine.Tiles[pos.x][pos.y].FrontArt == 0) {
    TileEngine.Tiles[pos.x][pos.y].Block = 0;
  } else {
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_WAND;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_EIS;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_SUMPF;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_LIQUID;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_WASSER;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_PLATTFORM;
    TileEngine.Tiles[pos.x][pos.y].Block &= ~BLOCKWERT_DESTRUCTIBLE;
  }
}

void TileCanvas::TryPlace() {
  auto pos = GetTileCordsUnderCursor();
  auto& tileSet = frame->editMenu->tileSet;

  switch (editMode) {
    case EDIT_MODE_FRONT:
      PlaceTileFront(                                       // foreground
          pos,                                              // tile position
          tileSet->GetSelectedTileID() + INCLUDE_ZEROTILE,  // art
          tileSet->GetSelectedTileSetID(),                  // tile set
          frame->editMenu->getBlockFlags()                  // flags
      );
      break;
    case EDIT_MODE_BACK:
      PlaceTileBack(                                        // background
          pos,                                              // tile position
          tileSet->GetSelectedTileID() + INCLUDE_ZEROTILE,  // art
          tileSet->GetSelectedTileSetID(),                  // tile set
          frame->editMenu->getBlockFlags()                  // flags
      );
      break;
    case EDIT_MODE_OBJECTS:
    case EDIT_MODE_VIEW:
      break;
  }
}

void TileCanvas::TryRemove() {
  auto pos = GetTileCordsUnderCursor();

  switch (editMode) {
    case EDIT_MODE_FRONT:
      RemoveTileFront(pos);
      break;
    case EDIT_MODE_BACK:
      RemoveTileBack(pos);
      break;
    case EDIT_MODE_OBJECTS:
    case EDIT_MODE_VIEW:
      break;
  }
}

wxPoint TileCanvas::GetTileCordsUnderCursor() {
  const float scaledOffsetX = TileEngine.XOffset / TileEngine.TileSizeX;
  const float scaledOffsetY = TileEngine.YOffset / TileEngine.TileSizeY;

  const float scaledMouseX = mousePos.x / TileEngine.TileSizeX;
  const float scaledMouseY = mousePos.y / TileEngine.TileSizeY;

  const float relativeX = scaledMouseX + scaledOffsetX;
  const float relativeY = scaledMouseY + scaledOffsetY;

  return wxPoint(relativeX, relativeY);
}

void TileCanvas::Update() {
  Timer.update();
  TileEngine.UpdateLevel();
  TileEngine.CalcRenderRange();
}

void TileCanvas::Render() {
  context->SetCurrent(*this);

  DirectGraphics.ClearBackBuffer();
  DirectGraphics.SetColorKeyMode();

  TileEngine.DrawBackground();

  switch (editMode) {
    case EDIT_MODE_FRONT: {
      TileEngine.DrawFrontLevel();
      ObjectList.DrawAllObjects(TileEngine.XOffset, TileEngine.YOffset,
                                TileEngine.Scale);
      TileEngine.DrawOverlayLevel();
    } break;
    case EDIT_MODE_BACK: {
      TileEngine.DrawBackLevel();
      ObjectList.DrawAllObjects(TileEngine.XOffset, TileEngine.YOffset,
                                TileEngine.Scale);
      TileEngine.DrawWater();
      TileEngine.DrawBackLevelOverlay();
    } break;
    case EDIT_MODE_OBJECTS: {
      TileEngine.DrawBackLevel();
      TileEngine.DrawFrontLevel();
      ObjectList.DrawAllObjects(TileEngine.XOffset, TileEngine.YOffset,
                                TileEngine.Scale);
      TileEngine.DrawBackLevelOverlay();
      TileEngine.DrawWater();
    } break;
    case EDIT_MODE_VIEW: {
      TileEngine.DrawBackLevel();
      TileEngine.DrawFrontLevel();

      ObjectList.DrawAllObjects(TileEngine.XOffset, TileEngine.YOffset,
                                TileEngine.Scale);

      TileEngine.DrawWater();
      TileEngine.DrawBackLevelOverlay();
      TileEngine.DrawOverlayLevel();
      // TileEngine.DrawShadow();
    } break;

    default:
      Protokoll << "ERROR" << std::endl;
      break;
  }

  DrawGrid();

  glFlush();
  SwapBuffers();
}

void TileCanvas::DrawGrid() {
  const int tilesX = DirectGraphics.RenderWidth / TileEngine.TileSizeX;
  const int tilesY = DirectGraphics.RenderHeight / TileEngine.TileSizeY;

  const float squareSizeX = TileEngine.TileSizeX / 15.0f;
  const float squareSizeY = TileEngine.TileSizeY / 15.0f;

  for (int x = 1; x < tilesX + 2; x++) {
    for (int y = 1; y < tilesY + 2; y++) {
      const float posX = (TileEngine.TileSizeX * x) - fmod(TileEngine.XOffset, TileEngine.TileSizeX);
      const float posY = (TileEngine.TileSizeY * y) - fmod(TileEngine.YOffset, TileEngine.TileSizeY);
      RenderRect(posX, posY, squareSizeX, squareSizeY, 0xff0000ff);
    }
  }
}
