/**
 * Watches open()s and sends them to a previously created FIFO, without ever blocking.
 * 
 * By Raúl Salinas-Monteagudo <rausalinas@gmail.com>
 * 
 * Based on https://rafalcieslak.wordpress.com/2013/04/02/dynamic-linker-tricks-using-ld_preload-to-cheat-inject-features-and-investigate-programs/
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

typedef int (*open_func_t)(const char *pathname, ...);
typedef int (*open64_func_t)(const char *pathname, ...);

static open_func_t orig_open;
static open64_func_t orig_open64;

static int fifo = -1;
static const char *filename;

__attribute__((constructor)) void open_watch_initialize(void) {
  orig_open64 = (open64_func_t)dlsym(RTLD_NEXT, "open64");
  orig_open = (open_func_t)dlsym(RTLD_NEXT, "open");
  
  filename = getenv("OPEN_WATCH_FIFO");
  if (!filename) {
	filename = "/tmp/open_watch_fifo";
	fprintf(stderr, "OPEN_WATCH_FIFO not defined\n");
  }  
  fprintf(stderr, "Fifo: %s\n", filename);
}

__attribute__((destructor)) void open_watch_deinitialize(void) { 
	close(fifo);
}

void logOpenAccess(const char *pathname) {
  if (fifo == -1) {
    fifo = orig_open(filename, O_NONBLOCK | O_RDWR, 0);
  }
  char buf[4096];
  int len = snprintf(buf, sizeof buf, "%s\n", pathname);
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
  logOpenAccess(pathname);
  
  va_list args;
  va_start(args, flags);
  int ret = orig_open(pathname, flags, args);
  va_end(args);
  return ret;
}

int open64(const char *pathname, int flags, ...) {
  logOpenAccess(pathname);
  
  va_list args;
  va_start(args, flags);
  int ret = orig_open64(pathname, flags, args);
  va_end(args);
  return ret;
}
