#ifndef GLOBAL_H
#define GLOBAL_H

#include <LNamespaces.h>

using namespace Louvre;

class Compositor;
class Output;
class Scene;
class LayerView;
class Assets;
class Systemd;
class Seat;

enum ID {
  backgroundType = 1,
};
/* This is just a utility for quick access to the Compositor members and
 * to prevent type casting all the time */
class G {
 public:
  enum ViewHint { SSDEdge = 1 };

  static Compositor *compositor() noexcept {
    return (Compositor *)Louvre::compositor();
  }

  static Seat *seat() noexcept { return (Seat *)Louvre::seat(); }

  static Scene &scene() noexcept;
  static LayerView *layers() noexcept;
  static Assets *assets() noexcept;
  static Systemd *systemd() noexcept;

  // Textures
  struct Textures {
    LTexture *background{nullptr};
  };
  static Textures *textures();
  static void loadTextures();
  static void setTexViewConf(LTextureView *view, UInt32 index);
};

#endif  // GLOBAL_H
