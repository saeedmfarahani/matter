#include "Global.h"

#include <LOpenGL.h>

#include "../Compositor.h"
#include "Global.h"
#include "LOutputMode.h"
#include "src/input/Seat.h"
Scene &G::scene() noexcept { return compositor()->scene; }

LayerView *G::layers() noexcept { return scene().layers; }

Assets *G::assets() noexcept { return compositor()->assets.get(); }

Systemd *G::systemd() noexcept { return compositor()->systemd.get(); }

static G::Textures _textures;

G::Textures *G::textures() { return &_textures; }

void G::loadTextures() {
  _textures.background = LOpenGL::loadTexture(
      std::filesystem::path(getenvString("HOME")) / ".config/background");
  if (_textures.background) {
    if (compositor()->graphicBackendId() == LGraphicBackendDRM) {
      LSize bestSize{0, 0};

      for (LOutput *output : seat()->outputs())
        if (output->currentMode()->sizeB().area() > bestSize.area() &&
            output->currentMode()->sizeB().area() <
                _textures.background->sizeB().area())
          bestSize = output->preferredMode()->sizeB();

      if (bestSize.area() != 0) {
        LRect srcRect = {
            0, 0,
            (bestSize.w() * _textures.background->sizeB().h()) / bestSize.h(),
            _textures.background->sizeB().h()};

        if (srcRect.w() <= _textures.background->sizeB().w()) {
          srcRect.setX((_textures.background->sizeB().w() - srcRect.w()) / 2);
        } else {
          srcRect.setW(_textures.background->sizeB().w());
          srcRect.setH((bestSize.h() * _textures.background->sizeB().w()) /
                       bestSize.w());
          srcRect.setY((_textures.background->sizeB().h() - srcRect.h()) / 2);
        }

        LTexture *tmp = _textures.background;
        _textures.background = tmp->copy(bestSize, srcRect);
        delete tmp;
      }
    }
  }
}
