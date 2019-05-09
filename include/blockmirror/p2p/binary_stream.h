#pragma once

#include <string.h>
#include <boost/noncopyable.hpp>

namespace blockmirror {
namespace p2p {

class Message {
 public:
  char* get_write_ptr();
  void move_write_pos(int offset);
  int get_body_len();
  void move_read_pos(int offset);
  void reset();
  char* get_read_ptr();
  int get_readable();

  template <typename T>
  Message& operator<<(const T&);
  template <typename T>
  Message& operator>>(T&);

 private:
  char buf_[8192];
  int read_pos_;
  int write_pos_;
  size_t size_;
};

template <typename T>
Message& Message::operator<<(const T& t) {
  int size = sizeof(t);
  memcpy(buf_ + write_pos_, (char*)&t, size);
  write_pos_ += size;

  return *this;
}

template <typename T>
Message& Message::operator>>(T& t) {
  int size = sizeof(t);
  memcpy((char*)&t, buf_ + read_pos_, t);
  read_pos_ += size;

  return *this;
}

}  // namespace p2p
}  // namespace blockmirror