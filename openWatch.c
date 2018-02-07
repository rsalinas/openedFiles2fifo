/**
 * Watches open()s and sends them to a previously created FIFO, without ever
 * blocking.
 *
 * By Raúl Salinas-Monteagudo <rausalinas@gmail.com>
 *
 * Based on
 * https://rafalcieslak.wordpress.com/2013/04/02/dynamic-linker-tricks-using-ld_preload-to-cheat-inject-features-and-investigate-programs/
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*open_func_t)(const char *pathname, int flags, ...);
typedef int (*open64_func_t)(const char *pathname, int flags, ...);
typedef int (*chdir_func_t)(const char *path);
typedef int (*close_func_t)(int fd);

static open_func_t orig_open;
static open64_func_t orig_open64;
static chdir_func_t orig_chdir;
static close_func_t orig_close;

static int fifo = -1;
static const char *filename;
static const char *cwd = NULL;

#define MAX_OPEN 1024
static const char *openFiles[MAX_OPEN];

__attribute__((constructor)) void open_watch_initialize(void) {
  memset(openFiles, 0, sizeof openFiles);
  orig_open64 = (open64_func_t)dlsym(RTLD_NEXT, "open64");
  orig_open = (open_func_t)dlsym(RTLD_NEXT, "open");
  orig_chdir = (chdir_func_t)dlsym(RTLD_NEXT, "chdir");
  orig_close = (close_func_t)dlsym(RTLD_NEXT, "close");
  cwd = get_current_dir_name();

  filename = getenv("OPEN_WATCH_FIFO");
  if (!filename) {
    filename = "/tmp/open_watch_fifo";
    fprintf(stderr, "OPEN_WATCH_FIFO not defined\n");
  }
  fprintf(stderr, "Fifo: %s\n", filename);
}

__attribute__((destructor)) void open_watch_deinitialize(void) {
  free((void *)cwd);
  orig_close(fifo);
}

void unregisterOpenFile(int fd) {
  if (fd < 0 || fd >= MAX_OPEN)
    return;
  if (openFiles[fd]) {
    free((void *)openFiles[fd]);
    openFiles[fd] = NULL;
  }
}

void registerOpenFile(int fd, const char *pathname) {
  fprintf(stderr, "open %d %s\n", fd, pathname);
  if (fd < 0 || fd >= MAX_OPEN)
    return;
  unregisterOpenFile(fd);
  openFiles[fd] = strdup(pathname);
}

void logOpenAccess(const char *pathname, int fd) {

  if (fifo == -1) {
    fifo = orig_open(filename, O_NONBLOCK | O_RDWR, 0);
  }
  char buf[4096];
  int len;
  if (*pathname == '/') {
    len = snprintf(buf, sizeof buf, "%s\n", pathname);
  } else {
    len = snprintf(buf, sizeof buf, "%s/%s\n", cwd, pathname);
  }

  registerOpenFile(fd, buf);
  if (fifo != -1) {
    int res = write(fifo, buf, len);
    if (res != len) {
      // The pipe was full? May this happen?
    }
  } else {
    // The fifo does not exist.
    fprintf(stderr, "Cannot open FIFO: %s\n", strerror(errno));
  }
}

int open(const char *pathname, int flags, ...) {
  if (!orig_open) {
    errno = ENOSYS;
    return -1;
  }
  va_list args;
  va_start(args, flags);
  int ret = orig_open(pathname, flags, flags & O_CREAT ? va_arg(args, int) : 0);

  va_end(args);
  logOpenAccess(pathname, ret);

  return ret;
}

int open64(const char *pathname, int flags, ...) {
  if (!orig_open64) {
    errno = ENOSYS;
    return -1;
  }
  va_list args;
  va_start(args, flags);
  int ret = orig_open64(pathname, flags, args,
                        flags & O_CREAT ? va_arg(args, int) : 0);
  va_end(args);
  logOpenAccess(pathname, ret);
  return ret;
}

int chdir(const char *path) {
  if (!orig_chdir) {
    errno = ENOSYS;
    return -1;
  }
  int ret = orig_chdir(path);
  if (cwd)
    free((void *)cwd);
  cwd = get_current_dir_name();
  return ret;
}

int close(int fd) {
  if (!orig_close) {
    errno = ENOSYS;
    return -1;
  }
  if (fd >= 0 && fd <= MAX_OPEN && openFiles[fd]) {
    fprintf(stderr, "close %d %s\n", fd, openFiles[fd]);
  } else {
    fprintf(stderr, "Wrong close %d\n", fd);
  }
  unregisterOpenFile(fd);
  return orig_close(fd);
}
