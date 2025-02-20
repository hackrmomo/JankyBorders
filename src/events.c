#include "events.h"
#include "windows.h"
#include "border.h"
#include "misc/window.h"

extern struct table g_windows;
extern pid_t g_pid;

static void dump_event(void* data, size_t data_length) {
  for (int i = 0; i < data_length; i++) {
    printf("%02x ", *((unsigned char*)data + i));
  }
  printf("\n");
}

static void event_watcher(uint32_t event, void* data, size_t data_length, void* context) {
  static int count = 0;
  printf("(%d) Event: %d; Payload:\n", ++count, event);
  dump_event(data, data_length);
}

struct window_spawn_data {
  uint64_t sid;
  uint32_t wid;
};

static void window_spawn_handler(uint32_t event, struct window_spawn_data* data, size_t _, int cid) {
  struct table* windows = &g_windows;
  uint32_t wid = data->wid;
  uint64_t sid = data->sid;

  if (!wid || !sid) return;

  if (event == EVENT_WINDOW_CREATE) {
    if (windows_window_create(windows, wid, sid)) {
      debug("Window Created: %d %d\n", wid, sid);
      windows_determine_and_focus_active_window(windows);
    }
  } else if (event == EVENT_WINDOW_DESTROY) {
    if (windows_window_destroy(windows, wid, sid)) {
      debug("Window Destroyed: %d %d\n", wid, sid);
    }
    windows_determine_and_focus_active_window(windows);
  }
}

static void window_modify_handler(uint32_t event, uint32_t* window_id, size_t _, int cid) {
  uint32_t wid = *window_id;
  struct table* windows = &g_windows;

  if (event == EVENT_WINDOW_MOVE) {
    debug("Window Move: %d\n", wid);
    windows_window_move(windows, wid);
  } else if (event == EVENT_WINDOW_RESIZE) {
    debug("Window Resize: %d\n", wid);
    windows_window_update(windows, wid);
  } else if (event == EVENT_WINDOW_REORDER) {
    debug("Window Reorder (and focus): %d\n", wid);
    windows_window_update(windows, wid);

    // The update of the front window might not have taken place yet...
    usleep(10000);
    windows_determine_and_focus_active_window(windows);
  } else if (event == EVENT_WINDOW_LEVEL) {
    debug("Window Level: %d\n", wid);
    windows_window_update(windows, wid);
  } else if (event == EVENT_WINDOW_TITLE || event == EVENT_WINDOW_UPDATE) {
    debug("Window Focus\n");
    windows_determine_and_focus_active_window(windows);
  } else if (event == EVENT_WINDOW_UNHIDE) {
    debug("Window Unhide: %d\n", wid);
    windows_window_unhide(windows, wid);
  } else if (event == EVENT_WINDOW_HIDE) {
    debug("Window Hide: %d\n", wid);
    windows_window_hide(windows, wid);
  }
}

static void front_app_handler() {
  windows_determine_and_focus_active_window(&g_windows);
}

static void space_handler() {
  // Not all native-fullscreen windows have yet updated their space id...
  usleep(20000);
  windows_draw_borders_on_current_spaces(&g_windows);
}

void events_register(int cid) {
  void* cid_ctx = (void*)(intptr_t)cid;

  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_MOVE, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_RESIZE, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_LEVEL, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_UNHIDE, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_HIDE, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_TITLE, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_REORDER, cid_ctx);
  SLSRegisterNotifyProc(window_modify_handler, EVENT_WINDOW_UPDATE, cid_ctx);
  SLSRegisterNotifyProc(window_spawn_handler, EVENT_WINDOW_CREATE, cid_ctx);
  SLSRegisterNotifyProc(window_spawn_handler, EVENT_WINDOW_DESTROY, cid_ctx);

  SLSRegisterNotifyProc(space_handler, EVENT_SPACE_CHANGE, cid_ctx);

  SLSRegisterNotifyProc(front_app_handler, EVENT_FRONT_CHANGE, cid_ctx);

#ifdef DEBUG
  for (int i = 0; i < 2000; i++) {
    SLSRegisterNotifyProc(event_watcher, i, NULL);
  }
#endif
}
