#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "doomkeys.h"
#include "m_argv.h"
#include "i_video.h"
#include "m_config.h"
#include "game.h"
#include "host.h"

struct dg_file_t {
  uint8_t *buffer;
  uint32_t alloc;
  uint32_t size;
  uint32_t pos;
  char *name;
  int dynamic, wr;
};

struct dg_dir_t {
  // not used
  int fd;
};

static int first = 1;
static int finish = 0;
static int variant = 0;
static int width, height;
static uint64_t t0;
static char *argv[16];

#define MAX_PATH 256

static char config[MAX_PATH];
static char extraConfig[MAX_PATH];
static char savedir[MAX_PATH];

#define KEYQUEUE_SIZE 16

static uint16_t KeyQueue[KEYQUEUE_SIZE];
static uint32_t KeyQueueWriteIndex;
static uint32_t KeyQueueReadIndex;

static uint32_t *DG_ScreenBuffer;

static dg_file_t wad = {0}, extraWad = {0};

extern void D_DoomMain(void);
extern int D_RunFrame(void);

extern void console_log(char *buf);
extern void get_now(struct timespec *tp);
extern int file_size(char *name);
extern void file_read(char *name, uint8_t *buf);
extern void file_write(char *name, uint8_t *buf, uint32_t n);
extern void file_remove(char *name);
extern void file_rename(char *oldname, char *newname);

uint32_t DG_color32(uint32_t r, uint32_t g, uint32_t b) {
  // 32 bits color ABGR (A is MSB)
  return 0xFF000000 | (b << 16) | (g << 8) | r;
}

static int64_t gettime(void) {
  struct timespec tp;
  int64_t ts;

  get_now(&tp);
  ts = ((int64_t)tp.tv_sec) * 1000000 + ((int64_t)tp.tv_nsec) / 1000;

  return ts;
}

static void DG_Init(void) {
  t0 = gettime();
  KeyQueueWriteIndex = 0;
  KeyQueueReadIndex = 0;
  DG_ScreenBuffer = mymalloc(SCREENWIDTH * 2 * SCREENHEIGHT * 2 * 4);
}

static void DG_Finish(void) {
  if (extraWad.buffer) myfree(extraWad.buffer);
  if (wad.buffer) myfree(wad.buffer);
  myfree(DG_ScreenBuffer);
}

static uint8_t convertToDoomKey(int key) {
  if (key >= 'A' && key <= 'Z') {
    key += 32;
  } else {
    switch (key) {
      case 38: key = 173; break;
      case 40: key = 175; break;
      case 37: key = 172; break;
      case 39: key = 174; break;
      case 17: key = 157; break;
    }
  }

  return key;
}

void DoomKey(int down, int key) {
  uint16_t keyData = (down << 8) | convertToDoomKey(key);
  KeyQueue[KeyQueueWriteIndex] = keyData;
  KeyQueueWriteIndex++;
  KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

uint32_t *DG_GetScreenBuffer(void) {
  return DG_ScreenBuffer;
}

uint32_t DG_GetTicksMs(void) {
  uint64_t t = gettime();
  uint32_t dt = t - t0;
  return dt / 1000;
}

static void setIntVariable(char *name, int n) {
  char value[16];
  snprintf(value, sizeof(value), "%d", n);
  M_SetVariable(name, value);
}

int DG_SleepMs(uint32_t ms) {
  //DG_debug(1, "DG_SleepMs %d", ms);
  //usleep(ms * 1000);
  //DG_debug(1, "DG_SleepMs %d done", ms);
  return 0;
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
  uint16_t keyData;

  if (KeyQueueReadIndex == KeyQueueWriteIndex) {
    return 0;
  }

  keyData = KeyQueue[KeyQueueReadIndex];
  KeyQueueReadIndex++;
  KeyQueueReadIndex %= KEYQUEUE_SIZE;

  *pressed = keyData >> 8;
  *doomKey = keyData & 0xFF;

  return 1;
}

void DG_SetWindowTitle(const char *_title) {
}

void DG_Fatal(const char *filename, const char *function, int lineNo, char *msg) {
  DG_debug(1 ,"DG_Fatal(\"%s\", \"%s\", %d, \"%s\")", filename, function, lineNo, msg);
  finish = 1;
}

int DG_create(char *name) {
  DG_debug(1 ,"DG_create(\"%s\")", name);
  return -1;
}

dg_file_t *DG_open(char *name, int wr) {
  dg_file_t *f;
  int len;

  if (!strcmp(name, gameWad(variant))) {
    wad.dynamic = 0;
    wad.pos = 0;
    DG_debug(1, "DG_open internal \"%s\" reading %u bytes", name, wad.size);
    return &wad;
  }

  if (extraWad.buffer && !strcmp(name, "extra.wad")) {
    wad.dynamic = 0;
    extraWad.pos = 0;
    DG_debug(1, "DG_open internal \"%s\" reading %u bytes", name, extraWad.size);
    return &extraWad;
  }

  if (wr) {
    if ((f = mycalloc(1, sizeof(dg_file_t))) != NULL) {
      f->name = mystrdup(name);
      f->dynamic = 1;
      f->wr = 1;
      f->alloc = 65536;
      f->buffer = mymalloc(f->alloc);
    }
    DG_debug(1, "DG_open create file \"%s\"", f->name);
    return f;
  }

  len = file_size(name);
  if (len < 0) return NULL;
  DG_debug(1, "DG_open \"%s\" len=%d", name, len);

  if ((f = mycalloc(1, sizeof(dg_file_t))) != NULL) {
    f->name = mystrdup(name);
    f->alloc = (len + 1023) & 0xFFFFFC00;
    f->size = len;
    f->dynamic = 1;
    f->wr = 0;
    if (len > 0) {
      DG_debug(1, "DG_open malloc %u", f->alloc);
      if ((f->buffer = mymalloc(f->alloc)) == NULL) {
        myfree(f->name);
        myfree(f);
        return NULL;
      }
      DG_debug(1, "DG_open \"%s\" reading %u bytes", name, f->size);
      file_read(name, f->buffer);
    } else {
      DG_debug(1, "DG_open empty \"%s\"", name);
    }
  }

  return f;
}

void DG_close(dg_file_t *f) {
  if (f) {
    DG_debug(1, "DG_close");
    f->pos = 0;

    if (f->dynamic) {
      if (f->name && f->buffer && f->size && f->wr) {
        DG_debug(1, "DG_close writing %u bytes to \"%s\"", f->size, f->name);
        file_write(f->name, f->buffer, f->size);
      }
      if (f->name) myfree(f->name);
      if (f->buffer) myfree(f->buffer);
      myfree(f);
    }
  }
}

int DG_eof(dg_file_t *f) {
  int r = 1;

  if (f) {
    r = f->pos >= f->size;
  }

  return r;
}

int DG_isdir(char *name) {
  int r = 0;

  DG_debug(1, "DG_isdir(\"%s\")", name);

  return r;
}

int DG_read(dg_file_t *f, void *buf, int n) {
  int r = -1;

  if (f && buf) {
    r = n <= (f->size - f->pos) ? n : f->size - f->pos;
    memcpy(buf, f->buffer + f->pos, r);
    f->pos += r;
  }

  return r;
}

int DG_write(dg_file_t *f, void *buf, int n) {
  int r = -1;

  if (f && buf && n >= 0 && f->dynamic && f->wr) {
    if (n == 0) {
      r = 0;
    } else {
      if (f->buffer == NULL) {
        //DG_debug(1, "DG_write \"%s\" n=%u init", f->name, n);
        f->alloc = n < 1024 ? 1024 : n;
        f->buffer = mymalloc(f->alloc);
      } else if (f->size + n <= f->alloc) {
        //DG_debug(1, "DG_write \"%s\" n=%u append, size=%u, alloc=%u", f->name, n, f->size, f->alloc);
      } else {
        //DG_debug(1, "DG_write \"%s\" n=%u increase, size=%u, alloc=%u", f->name, n, f->size, f->alloc);
        f->alloc += n < 1024 ? 1024 : n;
        f->buffer = myrealloc(f->buffer, f->alloc);
      }
      memcpy(f->buffer + f->pos, buf, n);
      f->size += n;
      f->pos += n;
      r = n;
    }
  }

  return r;
}

int DG_seek(dg_file_t *f, int offset) {
  int r = -1;

  if (f) {
    if (offset < 0) offset = 0;
    if (offset >= f->size) offset = f->size;
    f->pos = offset;
    r = 0;
  }

  return r;
}

int DG_tell(dg_file_t *f) {
  int r = 0;

  if (f) {
    r = f->pos;
  }

  return r;
}

int DG_filesize(dg_file_t *f) {
  int r = 0;

  if (f) {
    r = f->size;
  }

  return r;
}

int DG_mkdir(char *name) {
  DG_debug(1 ,"DG_mkdir(\"%s\")", name);
  return 0;
}

int DG_remove(char *name) {
  DG_debug(1 ,"DG_remove(\"%s\")", name);
  file_remove(name);
  return 0;
}

int DG_rename(char *oldname, char *newname) {
  DG_debug(1 ,"DG_rename(\"%s\", \"%s\")", oldname, newname);
  file_rename(oldname, newname);
  return 0;
}

int DG_printf(dg_file_t *f, const char *fmt, ...) {
  va_list ap;
  char buf[256];
  int n = 0;

  if (f && fmt) {
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    DG_write(f, buf, strlen(buf));
  }

  return n;
}

int DG_vprintf(dg_file_t *f, const char *fmt, va_list ap) {
  char buf[256];
  int n = 0;

  if (f && fmt) {
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    DG_write(f, buf, strlen(buf));
  }

  return n;
}

char *DG_fgets(dg_file_t *f, char *s, int size) {
  char *r = s;
  int i = 0;

  if (f && s && size > 0) {
    for (; i < size-1;) {
      if (DG_eof(f)) {
        break;
      }
      if (DG_read(f, &s[i], 1) != 1) {
        r = NULL;
        break;
      }
      i++;
      if (s[i-1] == '\n') {
        s[i] = 0;
        break;
      }
    }
  }

  return i ? r : NULL;
}

dg_dir_t *DG_opendir(char *name) {
  dg_dir_t *f;

  DG_debug(1 ,"DG_opendir(\"%s\")", name);
  if ((f = mycalloc(1, sizeof(dg_dir_t))) != NULL) {
  }

  return f;
}

int DG_readdir(dg_dir_t *f, char *name, int len) {
  int r = -1;

  if (f && name && len > 0) {
  }

  return r;
}

void DG_closedir(dg_dir_t *f) {
  if (f) {
    myfree(f);
  }
}

void DG_debug_full(const char *file, const char *func, int line, int level, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  DG_debugva_full(file, func, line, level, fmt, ap);
  va_end(ap);
}

void DG_debugva_full(const char *file, const char *func, int line, int level, const char *fmt, va_list ap) {
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  console_log(buf);
}

void DoomInit(void) {
  DG_debug(1, "DoomInit");
  width = SCREENWIDTH;
  height = SCREENHEIGHT;

  snprintf(config, MAX_PATH-1, "/%s/config%d.cfg", gameName(), variant);
  snprintf(extraConfig, MAX_PATH-1, "/%s/extra%d.cfg", gameName(), variant);
  snprintf(savedir, MAX_PATH-1, "/%s/savedir%d", gameName(), variant);

  myargc = 0;
  argv[myargc++] = gameName();
  argv[myargc++] = "-iwad";
  argv[myargc++] = gameWad(variant);
  if (extraWad.buffer) {
    argv[myargc++] = "-file";
    argv[myargc++] = "extra.wad";
  }
  argv[myargc++] = "-config";
  argv[myargc++] = config;
  argv[myargc++] = "-extraconfig";
  argv[myargc++] = extraConfig;
  argv[myargc++] = "-savedir";
  argv[myargc++] = savedir;
  argv[myargc] = NULL;
  myargv = argv;

  M_FindResponseFile();
  M_SetExeDir();
  DG_Init();

  D_DoomMain();
}

char *DoomWadName(int _variant) {
  variant = _variant;
  if (variant < 0 || variant >= gameVariants()) variant = 0;
  return gameWad(variant);
}

void *DoomWadAlloc(int len) {
  if (wad.buffer == NULL) {
    wad.buffer = mymalloc(len);
    wad.size = len;
    return wad.buffer;
  }

  extraWad.buffer = mymalloc(len);
  extraWad.size = len;
  return extraWad.buffer;
}

char *DoomName() {
  return gameName();
}

void *DoomStep(void) {
  if (first) {
    setIntVariable(gameMsgOn(), 0);
    setIntVariable("usegamma", 1);
    setPalette();
    first = 0;
  }

  finish = D_RunFrame();

  if (finish) {
    DG_debug(1, "last step");
    DG_Finish();
    return NULL;
  }

  return DG_ScreenBuffer;
}

void *mymalloc_full(const char *file, const char *func, int line, size_t size) {
  if (size % 4) size += 4 - (size % 4);
  void *p = malloc(size);
  memset(p, 0, size);
  //DG_debug_full(file, func, line, 1,  "HEAP %s %s %d alloc %p %u", file, func, line, p, size);
  return p;
}

void *mycalloc_full(const char *file, const char *func, int line, size_t nmemb, size_t size) {
  size *= nmemb;
  if (size % 4) size += 4 - (size % 4);
  void *p = malloc(size);
  memset(p, 0, size);
  //DG_debug_full(file, func, line, 1,  "HEAP %s %s %d alloc %p %u", file, func, line, p, size);
  return p;
}

void *myrealloc_full(const char *file, const char *func, int line, void *ptr, size_t size) {
  if (size % 4) size += 4 - (size % 4);
  void *p = realloc(ptr, size);
  //DG_debug_full(file, func, line, 1, "HEAP %s %s %d free %p", ptr);
  //DG_debug_full(file, func, line, 1, "HEAP %s %s %d alloc %p %u", file, func, line, p, size);
  return p;
}

char *mystrdup_full(const char *file, const char *func, int line, const char *s) {
  char *r = NULL;
  if (s) {
    size_t len = strlen(s);
    size_t size = len + 1;
    if (size % 4) size += 4 - (size % 4);
    r = malloc(size);
    memset(r, 0, size);
    memcpy(r, s, len);
    //DG_debug_full(file, func, line, 1,  "HEAP %s %s %d alloc %p %u", file, func, line, r, size);
  }
  return r;
}

void myfree_full(const char *file, const char *func, int line, void *ptr) {
  //DG_debug_full(file, func, line, 1, "HEAP %s %s %d free %p", file, func, line, ptr);
  if (ptr) free(ptr);
}
