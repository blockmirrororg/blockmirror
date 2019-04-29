#ifndef HANDLER_MANAGER_H
#define HANDLER_MANAGER_H

#include <boost/unordered_map.hpp>
#include "MessageHandler.h"
#include "RequestAckHandler.h"
#include "Connection.h"

class HandlerManager
{
public:
	void register_message_handler(unsigned char remote_type, unsigned short msg_type, MessageHandler* message_handler);
	void register_async_request(unsigned char remote_type, unsigned short msg_type, App_Request_Handler&);
	void register_async_ack(unsigned char remote_type, unsigned short msg_type, App_Async_Ack_Handler&);

	static HandlerManager &get();
	void dispatch_message_handler(Connection &connection);

private:
	unsigned int make_id(unsigned char remote_type, unsigned short msg_type);
	
private:
	boost::unordered_map<unsigned int, MessageHandler*> message_handlers_;
};

#endif