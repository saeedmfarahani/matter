#include "Output.h"

#include <LAnimation.h>
#include <LScreenshotRequest.h>
#include <LSessionLockRole.h>

#include "Compositor.h"
#include "LOutputMode.h"
#include "Output.h"
#include "Surface.h"
#include "roles/ToplevelRole.h"
#include "utils/Global.h"

Output::Output(const void *params) noexcept : LOutput(params) {
  background.enableDstSize(true);
  background.enableSrcRect(compositor()->graphicBackendId() ==
                           LGraphicBackendWayland);
  background.enablePointerEvents(true);
  background.enableBlockPointer(true);
  background.setTranslucentRegion(&LRegion::EmptyRegion());
  background.setUserData(backgroundType);
}
void Output::initializeGL() {
  background.setParent(&G::compositor()->scene.layers[0]);
  backgroundUpdate();

  G::scene().handleInitializeGL(this);

  /* Fade-in animation example */

  LWeak<Output> weakRef{this};
  fadeInView.insertAfter(&G::layers()[LLayerOverlay]);
  fadeInView.setOpacity(0.f);

  LAnimation::oneShot(
      1000,                        // Ms
      [weakRef](LAnimation *anim)  // On Update
      {
        // Oops the output was probably unplugged!
        if (!weakRef) {
          anim->stop();
          return;
        }

        weakRef->fadeInView.setPos(weakRef->pos());
        weakRef->fadeInView.setSize(weakRef->size());
        weakRef->fadeInView.setOpacity(1.f - powf(anim->value(), 5.f));
        weakRef->repaint();
      },
      [weakRef](LAnimation *)  // On Finish
      {
        // Remove it from the scene
        if (weakRef) weakRef->fadeInView.setParent(nullptr);
      });
}

void Output::paintGL() {
  Surface *fullscreenSurface{searchFullscreenSurface()};

  if (fullscreenSurface) {
    /*
     * Content Type
     *
     * This hint can be passed to outputs to optimize content display.
     * For example, if the content type is "Game", a TV plugged in via HDMI may
     * try to reduce latency.
     */
    setContentType(fullscreenSurface->contentType());

    /*
     * V-SYNC
     *
     * Clients can suggest enabling or disabling v-sync for specific surfaces
     * using LSurface::preferVSync(). For instance, you may choose to enable or
     * disable it when displaying a toplevel surface in fullscreen mode. Refer
     * to LOutput::enableVSync() for more information.
     */
    enableVSync(fullscreenSurface->preferVSync());

    /*
     * Oversampling:
     *
     * When you assign a fractional scale to an output, oversampling is enabled
     * by default to mitigate aliasing artifacts, but this may impact
     * performance. For instance, you might want to switch off oversampling when
     * displaying a fullscreen surface. Refer to enableFractionalOversampling().
     *
     * Note: Oversampling is unnecessary and always disabled when using integer
     * scales. Therefore, it's recommended to stick with integer scales and
     * select an appropriate LOutputMode that suits your requirements.
     */
    enableFractionalOversampling(false);

    /*
     * Direct Scanout
     *
     * Directly scanning fullscreen surfaces reduces GPU consumption and
     * latency. However, there are several conditions to consider before doing
     * so, such as ensuring there is no overlay content like subsurfaces,
     * popups, notifications (as these won't be displayed), or screenshot
     * requests which are always denied. Refer to
     * LOutput::setCustomScanoutBuffer() for more information.
     */
    if (tryDirectScanout(fullscreenSurface))
      return;  // On success, avoid doing any rendering.
  } else {
    setContentType(LContentTypeNone);
    enableFractionalOversampling(true);
  }

  /* Let our scene do its magic */
  G::scene().handlePaintGL(this);

  /* Screen capture requests for this single frame */
  for (LScreenshotRequest *req : screenshotRequests()) req->accept(true);
}

void Output::moveGL() {
  backgroundUpdate();
  G::scene().handleMoveGL(this);
}

void Output::resizeGL() {
  backgroundUpdate();
  G::scene().handleResizeGL(this);
}

void Output::uninitializeGL() {
  background.setParent(nullptr);
  backgroundScaled.reset();
  G::scene().handleUninitializeGL(this);
}

void Output::backgroundUpdate() noexcept {
  background.setPos(pos());
  background.setDstSize(size());

  if (!G::textures()->background || size().area() == 0) return;

  if (compositor()->graphicBackendId() == LGraphicBackendDRM) {
    LSize bufferSize;

    if (is90Transform(transform())) {
      bufferSize.setW(currentMode()->sizeB().h());
      bufferSize.setH(currentMode()->sizeB().w());
    } else
      bufferSize = currentMode()->sizeB();

    if (backgroundScaled && backgroundScaled->sizeB() == bufferSize) return;

    LRect srcB;
    const Float32 w{
        Float32(bufferSize.w() * G::textures()->background->sizeB().h()) /
        Float32(bufferSize.h())};

    /* Clip and scale the wallpaper texture */

    if (w >= G::textures()->background->sizeB().w()) {
      srcB.setX(0);
      srcB.setW(G::textures()->background->sizeB().w());
      srcB.setH((G::textures()->background->sizeB().w() * bufferSize.h()) /
                bufferSize.w());
      srcB.setY((G::textures()->background->sizeB().h() - srcB.h()) / 2);
    } else {
      srcB.setY(0);
      srcB.setH(G::textures()->background->sizeB().h());
      srcB.setW((G::textures()->background->sizeB().h() * bufferSize.w()) /
                bufferSize.h());
      srcB.setX((G::textures()->background->sizeB().w() - srcB.w()) / 2);
    }
    backgroundScaled.reset(G::textures()->background->copy(bufferSize, srcB));
    background.setTexture(backgroundScaled.get());
  } else {
    background.setTexture(G::textures()->background);
    const LSize &texSize{background.texture()->sizeB()};

    const Int32 outputScaledHeight{(texSize.w() * size().h()) / size().w()};

    if (outputScaledHeight >= G::textures()->background->sizeB().h()) {
      const Int32 outputScaledWidth{(texSize.h() * size().w()) / size().h()};
      background.setSrcRect(LRectF((texSize.w() - outputScaledWidth) / 2.f, 0.f,
                                   outputScaledWidth, texSize.h()));
    } else
      background.setSrcRect(LRectF(0.f,
                                   (texSize.h() - outputScaledHeight) / 2.f,
                                   texSize.w(), outputScaledHeight));
  }
}

void Output::setGammaRequest(LClient *client, const LGammaTable *gamma) {
  L_UNUSED(client)
  setGamma(gamma);
}

void Output::availableGeometryChanged() {
  /* Refer to the default implementation in the documentation. */
  LOutput::availableGeometryChanged();
}

Surface *Output::searchFullscreenSurface() const noexcept {
  /*
   * When the session is locked the locking client creates a fullscreen
   * surface for each initialized output
   */
  if (sessionLockRole() && sessionLockRole()->surface()->mapped())
    return static_cast<Surface *>(sessionLockRole()->surface());

  /*
   * Louvre moves fullscreen toplevels to the top layer.
   * The default implementation of LToplevelRole (which is partially used by our
   * ToplevelRole) stores the output in its exclusiveOutput() property
   */
  for (auto it = compositor()->layer(LLayerTop).rbegin();
       it != compositor()->layer(LLayerTop).rend(); it++)
    if ((*it)->mapped() && (*it)->toplevel() &&
        (*it)->toplevel()->fullscreen() &&
        (*it)->toplevel()->exclusiveOutput() == this)
      return static_cast<Surface *>(*it);

  // No fullscreen surfaces on this output
  return nullptr;
}

bool Output::tryDirectScanout(Surface *surface) noexcept {
  if (!surface || !surface->mapped() ||

      /* Ensure there is no overlay content.
       * TODO: Check if there are overlay layer-shell surfaces
       * such as notifications */
      surface->hasMappedChildSurface() ||

      /* Screenshot requests are denied when there is a custom scanout buffer */
      !screenshotRequests().empty() ||

      /* Hardware cursor planes can still be displayed on top */
      (cursor()->visible() && !cursor()->hwCompositingEnabled(this)))
    return false;

  const bool ret{setCustomScanoutBuffer(surface->texture())};

  /* Ask the surface to continue updating */
  if (ret) surface->requestNextFrame();

  return ret;
}
