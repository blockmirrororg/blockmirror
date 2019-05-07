#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

class MessageHandler
{
public:
	virtual ~MessageHandler() {}

public:
	virtual void handle_message() = 0;
};

#endif