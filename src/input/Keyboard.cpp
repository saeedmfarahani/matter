#include "Keyboard.h"

#include <LKeyboardKeyEvent.h>
#include <LLauncher.h>
#include <LSessionLockManager.h>

#include "../scene/Scene.h"
#include "../utils/Global.h"
#include "../utils/Settings.h"
#include "LCompositor.h"

Keyboard::Keyboard(const void *params) noexcept : LKeyboard(params) {
  /* Key press repeat rate */
  setRepeatInfo(32, 500);

  /* Keymap sent to clients and used by the compositor, check the LKeyboard
   * class or XKB doc */
  setKeymap(getenv("MATTER_KEY_RULES"),     // Rules
            getenv("MATTER_KEY_MODEL"),     // Model
            getenv("MATTER_KEY_LAYOUT"),    // Layout
            getenv("MATTER_KEY_OPTIONS"));  // Options
}

void Keyboard::keyEvent(const LKeyboardKeyEvent &event) {
  /* The AuxFunc flag adds the Ctrl + Shift + ESC shortcut to quit, ensure
   * to add a way to exit if you remove it */
  const bool L_CTRL{isKeyCodePressed(KEY_LEFTCTRL)};
  const bool R_CTRL{isKeyCodePressed(KEY_RIGHTCTRL)};
  const bool L_SHIFT{isKeyCodePressed(KEY_LEFTSHIFT)};
  const bool L_ALT{isKeyCodePressed(KEY_LEFTALT)};
  const bool mods{L_ALT || L_SHIFT || L_CTRL || R_CTRL};

  G::scene().handleKeyboardKeyEvent(event, SETTINGS_SCENE_EVENT_OPTIONS);
  if (event.state() == LKeyboardKeyEvent::Released) {
    if (event.keyCode() == KEY_F1 && !mods) LLauncher::launch("ptyxis");
    if (event.keyCode() == KEY_ESC && L_CTRL && L_SHIFT) {
      compositor()->finish();
      return;
    }
  }
}
