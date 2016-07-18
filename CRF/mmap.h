//
//  CRF++ -- Yet Another CRF toolkit
//
//  $Id: mmap.h 1588 2007-02-12 09:03:39Z taku $;
//
//  Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//
#ifndef CRFPP_MMAP_H__
#define CRFPP_MMAP_H__

#include <errno.h>
#include <string>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#else

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif
}

#include "common.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
namespace {
int open__(const char* name, int flag) { return open(name, flag); }
int close__(int fd) { return close(fd); }
}
#endif

namespace CRFPP {

template <class T> class Mmap {
 private:
  T            *text;
  size_t       length;
  std::string  fileName;
  whatlog what_;

#if defined(_WIN32) && !defined(__CYGWIN__)
  HANDLE hFile;
  HANDLE hMap;
#else
  int    fd;
  int    flag;
#endif

 public:
  T&       operator[](size_t n)       { return *(text + n); }
  const T& operator[](size_t n) const { return *(text + n); }
  T*       begin()           { return text; }
  const T* begin()    const  { return text; }
  T*       end()           { return text + size(); }
  const T* end()    const  { return text + length/sizeof(T); }
  size_t size()               { return length/sizeof(T); }
  const char *what()          { return what_.str(); }
  const char *file_name()     { return fileName.c_str(); }
  size_t file_size()          { return length; }
  bool empty()                { return(length == 0); }

  // This code is imported from sufary, develoved by
  //  TATUO Yamashita <yto@nais.to> Thanks!

  bool open(const char *filename, const char *mode = "r") {
    this->close();
    struct stat st;
    fileName = std::string(filename);

    if      (std::strcmp(mode, "r") == 0)
      flag = O_RDONLY;
    else if (std::strcmp(mode, "r+") == 0)
      flag = O_RDWR;
    else
      CHECK_CLOSE_FALSE(false) << "unknown open mode: " << filename;

    CHECK_CLOSE_FALSE((fd = open__(filename, flag | O_BINARY)) >= 0)
        << "open failed: " << filename;

    CHECK_CLOSE_FALSE(fstat(fd, &st) >= 0)
        << "failed to get file size: " << filename;

    length = st.st_size;

#ifdef HAVE_MMAP
    int prot = PROT_READ;
    if (flag == O_RDWR) prot |= PROT_WRITE;
    char *p;
    CHECK_CLOSE_FALSE((p = reinterpret_cast<char *>
                       (mmap(0, length, prot, MAP_SHARED, fd, 0)))
                      != MAP_FAILED)
        << "mmap() failed: " << filename;

    text = reinterpret_cast<T *>(p);
#else
    text = new T[length];
    CHECK_CLOSE_FALSE(read(fd, text, length) >= 0)
        << "read() failed: " << filename;
#endif
    close__(fd);
    fd = -1;

    return true;
  }

  void close() {
    if (fd >= 0) {
      close__(fd);
      fd = -1;
    }

    if (text) {
#ifdef HAVE_MMAP
      munmap(reinterpret_cast<char *>(text), length);
      text = 0;
#else
      if (flag == O_RDWR) {
        int fd2;
        if ((fd2 = open__(fileName.c_str(), O_RDWR)) >= 0) {
          write(fd2, text, length);
          close__(fd2);
        }
      }
      delete [] text;
#endif
    }

    text = 0;
  }

  Mmap(): text(0), fd(-1) {}
#endif

  virtual ~Mmap() { this->close(); }
};
}
