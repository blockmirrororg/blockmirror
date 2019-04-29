
#include "Message.h"

char *Message::get_write_ptr()
{
	return buf_ + write_pos_;
}

void Message::move_write_pos(int offset)
{
	write_pos_ += offset;
}

int Message::get_body_len()
{
	return 0;
}

void Message::move_read_pos(int offset)
{
	read_pos_ += offset;
}

void Message::reset()
{
	read_pos_ = 0;
	write_pos_ = 0;
	size_ = 0;
}

char* Message::get_read_ptr()
{
	return buf_ + read_pos_;
}

int Message::get_readable()
{
	return write_pos_ - read_pos_;
}