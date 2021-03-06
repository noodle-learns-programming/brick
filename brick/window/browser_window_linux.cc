
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <include/base/cef_logging.h>

#include "../window_util.h"
#include "browser_window.h"
#include "../brick_app.h"

bool
BrowserWindow::WrapNative(CefWindowHandle window) {
  if (window_handler_) {
    LOG(ERROR) << "Browser window already have native window. Can't wrap one more window.";
    return false;
  }

  XFlush(cef_get_xdisplay());
  window_handler_ = gdk_window_foreign_new(window);
  if (!window_handler_) {
    LOG(ERROR) << "Browser window has been destroyed. Can't wrap.";
    return false;
  }

  AddRef();
  g_object_set_data(G_OBJECT(window_handler_), "wrapper", this);
  gdk_window_set_icon_list(window_handler_, window_util::GetDefaultIcons());
  gdk_window_set_events(window_handler_, (GdkEventMask) (
          GDK_STRUCTURE_MASK|GDK_VISIBILITY_NOTIFY_MASK
  ));
  return true;
}

void
BrowserWindow::Resize(int width, int height) {
  bool need_froze_size = !resizable_;
  if (need_froze_size)
    FrozeSize(0, 0);

  gdk_window_resize(window_handler_, width, height);

  if (need_froze_size)
    FrozeSize(width, height);
}

void
BrowserWindow::SetMinSize(int width, int height) {
  GdkGeometry hints;
  hints.min_width = width;
  hints.min_height = height;
  gdk_window_set_geometry_hints(window_handler_, &hints, GDK_HINT_MIN_SIZE);
}

void
BrowserWindow::FrozeSize(int width, int height) {
  GdkGeometry hints = {0};
  if (width && height) {
    resizable_ = false;
    hints.min_width = width;
    hints.min_height = height;
    hints.max_width = width;
    hints.max_height = height;
  } else {
    resizable_ = true;
  }

  gdk_window_set_geometry_hints(window_handler_, &hints, (GdkWindowHints) (GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}

void
BrowserWindow::SetTitle(std::string title) {
  gdk_window_set_title(window_handler_, title.c_str());
}

void
BrowserWindow::Present() {
  Show();
  if (!hided_) {
      // If window mapped - activate it immediately
      SetActive();
  }
}

void
BrowserWindow::Show() {
  bool handled = OnShow();
  if (!handled) {
    gdk_window_show(window_handler_);
    SaveRestorePosition(false);
  }
}

void
BrowserWindow::Hide() {
  bool handled = OnHide();
  if (!handled) {
    SaveRestorePosition(true);
    gdk_window_hide(window_handler_);
  }
}

void
BrowserWindow::Close() {
  gdk_window_destroy(window_handler_);
}

void
BrowserWindow::Popupping() {
  gdk_window_set_type_hint(window_handler_, GDK_WINDOW_TYPE_HINT_DIALOG);
}

void
BrowserWindow::OnNativeEvent(BrowserWindowNativeEvent event) {
  // ToDo: implement native window event handlers
  switch (event->type) {
    case GDK_DESTROY:
      // Release reference when wrapped window destroy
      Release();
      break;
    case GDK_MAP:
      hided_ = false;
      SetActive();
      break;
    case GDK_UNMAP:
      hided_ = true;
      break;
    case GDK_VISIBILITY_NOTIFY:
      visible_ = (event->visibility.state != GDK_VISIBILITY_FULLY_OBSCURED);
      break;
    default:
//      LOG(INFO) << "window: " << window_handler_ << ", event: " << event->type;
      break;
  }

}

void
BrowserWindow::ToggleVisibility() {
  bool is_hided = (
     hided_
     || !visible_
     || (gdk_window_get_state(window_handler_) & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN))
  );

  if (is_hided) {
    Present();
  } else {
    Hide();
  }
}

void
BrowserWindow::SetActive() {
  gdk_window_focus(window_handler_,
    gdk_x11_display_get_user_time(gdk_window_get_display(window_handler_))
  );
}

void
BrowserWindow::SaveRestorePosition(bool save) {
  if (save) {
    GdkRectangle extents;
    gdk_window_get_frame_extents(window_handler_, &extents);
    last_x_ = extents.x;
    last_y_ = extents.y;
  } else if (
     last_x_ >= 0 && last_x_ < (gdk_screen_width() - 60)
     && last_y_ >= 0 && last_y_ < (gdk_screen_height() - 60)
     ) {

    gdk_window_move(window_handler_, last_x_, last_y_);
  }
}

void
BrowserWindow::FlushChanges() {
  gdk_flush();
}

void
BrowserWindow::MoveResize(Position position, int width, int height) {
  int x, y;

  switch(position) {
    case Position::NORTH_WEST:
      x = BROWSER_WINDOW_PADDING;
      y = BROWSER_WINDOW_PADDING;
      break;
    case Position::NORTH:
      x = (gdk_screen_width() - width) / 2;
      y = BROWSER_WINDOW_PADDING;
      break;
    case Position::NORTH_EAST:
      x = gdk_screen_width() - width - BROWSER_WINDOW_PADDING;
      y = BROWSER_WINDOW_PADDING;
      break;
    case Position::WEST:
      x = BROWSER_WINDOW_PADDING;
      y = (gdk_screen_height() - height) / 2;
      break;
    case Position::CENTER:
      x = (gdk_screen_width() - width) / 2;
      y = (gdk_screen_height() - height) / 2;
      break;
    case Position::EAST:
      x = gdk_screen_width() - width - BROWSER_WINDOW_PADDING;
      y = (gdk_screen_height() - height) / 2;
      break;
    case Position::SOUTH_WEST:
      x = BROWSER_WINDOW_PADDING;
      y = gdk_screen_height() - height - BROWSER_WINDOW_PADDING;
      break;
    case Position::SOUTH:
      x = (gdk_screen_width() - width) / 2;
      y = gdk_screen_height() - height - BROWSER_WINDOW_PADDING;
      break;
    case Position::SOUTH_EAST:
      x = gdk_screen_width() - width - BROWSER_WINDOW_PADDING;
      y = gdk_screen_height() - height - BROWSER_WINDOW_PADDING;
      break;
    default:
      x = BROWSER_WINDOW_PADDING;
      y = BROWSER_WINDOW_PADDING;
      LOG(WARNING) << "Unknown window position: " << position << ", used NORTH_WEST";
      break;
  }

  gdk_window_move_resize(window_handler_, x, y, width, height);
}

void
BrowserWindow::SetKeepAbove(bool above) {
  gdk_window_set_keep_above(window_handler_, above);
}

void
BrowserWindow::SetKeepBelow(bool below) {
  gdk_window_set_keep_below(window_handler_, below);
}