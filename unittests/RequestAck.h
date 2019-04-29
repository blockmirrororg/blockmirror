#ifndef REQUEST_ACK_H
#define REQUEST_ACK_H

#include "RequestAckHandler.h"
#include "MessageHandler.h"

class RequestHandler :public MessageHandler
{
public:
	RequestHandler(App_Request_Handler&);
	virtual void handle_message();

private:
	App_Request_Handler& app_handler_;
};

// ------------------------------------------------------------------------------------
class AsyncAckHandler :public MessageHandler
{
public:
	AsyncAckHandler(App_Async_Ack_Handler&);
	virtual void handle_message();
	void handle_timeout(const Message& request);

private:
	App_Async_Ack_Handler &app_handler_;
};

#endif