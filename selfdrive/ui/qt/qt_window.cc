#include "selfdrive/ui/qt/qt_window.h"

#include <QKeyEvent>
#include <QShortcut>

// 全屏切换快捷键
void setFullScreenShortcut(QWidget *w) {
  QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F11), w);
  QObject::connect(shortcut, &QShortcut::activated, [w]() {
    if (w->isFullScreen()) {
      w->showNormal();
    } else {
      w->showFullScreen();
    }
  });
}

void setMainWindow(QWidget *w) {
  const float scale = util::getenv("SCALE", 1.0f);

  if (Hardware::PC()) {
    w->showFullScreen();
    // 添加F11快捷键切换全屏
    setFullScreenShortcut(w);
  } else {
    w->setFixedSize(DEVICE_SCREEN_SIZE * scale);
    w->show();
  }

#ifdef QCOM2
  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  wl_surface *s = reinterpret_cast<wl_surface*>(native->nativeResourceForWindow("surface", w->windowHandle()));
  wl_surface_set_buffer_transform(s, WL_OUTPUT_TRANSFORM_270);
  wl_surface_commit(s);

  w->setWindowState(Qt::WindowFullScreen);
  w->setVisible(true);

  // ensure we have a valid eglDisplay, otherwise the ui will silently fail
  void *egl = native->nativeResourceForWindow("egldisplay", w->windowHandle());
  assert(egl != nullptr);
#endif
}


extern "C" {
  void set_main_window(void *w) {
    setMainWindow((QWidget*)w);
  }
}
