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

#define ROOT "/"

struct dg_file_t {
  char *name;
  uint8_t *buffer;
  uint32_t size;
  uint32_t pos;
};

struct dg_dir_t {
  int fd;
};

static int first;
static uint64_t t0;
static int width, height;
static char *argv[16];

#define KEYQUEUE_SIZE 16

static uint16_t KeyQueue[KEYQUEUE_SIZE];
static uint32_t KeyQueueWriteIndex;
static uint32_t KeyQueueReadIndex;

static uint32_t *DG_ScreenBuffer;

static dg_file_t config0, extra0, wad;

extern void D_DoomMain(void);
extern void D_RunFrame(void);

extern void console_log(char *buf);
extern void getnow(struct timespec *tp);

uint32_t DG_color32(uint32_t r, uint32_t g, uint32_t b) {
  // 32 bits color ABGR (A is MSB)
  return 0xFF000000 | (b << 16) | (g << 8) | r;
}

static int64_t gettime(void) {
  struct timespec tp;
  int64_t ts;

  getnow(&tp);
  ts = ((int64_t)tp.tv_sec) * 1000000 + ((int64_t)tp.tv_nsec) / 1000;

  return ts;
}

static void DG_Init(void) {
  t0 = gettime();
  first = 1;
  KeyQueueWriteIndex = 0;
  KeyQueueReadIndex = 0;
  DG_ScreenBuffer = malloc(SCREENWIDTH * SCREENHEIGHT * 4);
}

static void DG_Finish(void) {
  free(DG_ScreenBuffer);
}

static uint8_t convertToDoomKey(int key) {
  if (key >= 'A' && key <= 'Z') {
    key += 32;
  }

  switch (key) {
    case 38: key = 173; break;
    case 40: key = 175; break;
    case 37: key = 172; break;
    case 39: key = 174; break;
    case 17: key = 157; break;
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

static char *getGameRoot(char *name) {
  char *rootdir;
  int len;

  len = strlen(ROOT) + strlen(name) + 1;
  rootdir = malloc(len);
  snprintf(rootdir, len, "%s%s", ROOT, name);

  return rootdir;
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
}

int DG_create(char *name) {
  DG_debug(1 ,"DG_create(\"%s\")", name);
  return -1;
}

dg_file_t *DG_open(char *name, int wr) {
  uint32_t len;

  DG_debug(1, "DG_open(\"%s\", %d)", name, wr);

  if (!strcmp(name, "config0.cfg")) {
    config0.name = strdup(name);
    config0.buffer = (uint8_t *)gameConfig(0);
    config0.size = strlen((char *)config0.buffer);
    config0.pos = 0;
    DG_debug(1, "returning internal %s", name);
    return &config0;
  }

  if (!strcmp(name, "extra0.cfg")) {
    extra0.name = strdup(name);
    extra0.buffer = (uint8_t *)gameExtraConfig(0);
    extra0.size = strlen((char *)extra0.buffer);
    extra0.pos = 0;
    DG_debug(1, "returning internal %s", name);
    return &extra0;
  }

  if (!strcmp(name, gameWad(0))) {
    wad.name = strdup(name);
    wad.pos = 0;
    DG_debug(1, "returning internal %s", name);
    return &wad;
  }

  return NULL;
}

void DG_close(dg_file_t *f) {
  if (f) {
    DG_debug(1, "DG_close \"%s\")", f->name);
    free(f->name);
    f->pos = 0;
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

  return r;
}

int DG_read(dg_file_t *f, void *buf, int n) {
  int r = -1;

  if (f && buf) {
    r = n < (f->size - f->pos) ? n : f->size - f->pos;
    memcpy(buf, f->buffer + f->pos, n);
    //DG_debug(1 ,"DG_read \"%s\" size=%d pos=%d n=%d: %d", f->name, f->size, f->pos, n, r);
    f->pos += n;
  }

  return r;
}

int DG_write(dg_file_t *f, void *buf, int n) {
  return -1;
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

  if (r) {
    return f->pos;
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
  int r = 0;

  DG_debug(1 ,"DG_mkdir(\"%s\")", name);

  return r;
}

int DG_remove(char *name) {
  DG_debug(1 ,"DG_remove(\"%s\")", name);
  return -1;
}

int DG_rename(char *oldname, char *newname) {
  DG_debug(1 ,"DG_rename(\"%s\", \"%s\")", oldname, newname);
  return -1;
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
        //DG_debug(1, "DG_fgets \"%s\" pos=%d i=%d eof", f->name, f->pos, i);
        break;
      }
      if (DG_read(f, &s[i], 1) != 1) {
        //DG_debug(1, "DG_fgets \"%s\" pos=%d i=%d error", f->name, f->pos, i);
        r = NULL;
        break;
      }
      i++;
      if (s[i-1] == '\n') {
        s[i] = 0;
        //DG_debug(1, "DG_fgets \"%s\" %d \"%s\")", f->name, i, i ? s : "");
        break;
      }
    }
  }

  return i ? r : NULL;
}

dg_dir_t *DG_opendir(char *name) {
  dg_dir_t *f;

  DG_debug(1 ,"DG_opendir(\"%s\")", name);
  if ((f = calloc(1, sizeof(dg_dir_t))) != NULL) {
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
    free(f);
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

  myargc = 0;
  argv[myargc++] = gameName();
  argv[myargc++] = "-iwad";
  argv[myargc++] = gameWad(0);
  argv[myargc++] = "-config";
  argv[myargc++] = "config0.cfg";
  argv[myargc++] = "-extraconfig";
  argv[myargc++] = "extra0.cfg";
  argv[myargc++] = "-savedir";
  argv[myargc++] = "savedir0";
  argv[myargc] = NULL;
  myargv = argv;

  M_FindResponseFile();
  DG_debug(1, "M_FindResponseFile()");
  M_SetExeDir();
  DG_debug(1, "M_SetExeDir()");
  DG_Init();
  DG_debug(1, "DG_Init()");

  D_DoomMain();
  DG_Finish();
}

void *DoomWad(int len) {
  wad.buffer = malloc(len);
  wad.size = len;
  return wad.buffer;
}

void *DoomStep(void) {
  if (first) {
    setIntVariable(gameMsgOn(), 0);
    setIntVariable("usegamma", 3);
    setPalette();
    first = 0;
  }

  D_RunFrame();
  return DG_ScreenBuffer;
}
