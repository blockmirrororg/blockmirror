
#include "HandlerManager.h"
#include "RequestAck.h"

void HandlerManager::register_message_handler(unsigned char remote_type, unsigned short msg_type, MessageHandler* message_handler)
{
	message_handlers_.insert(std::pair<unsigned int, MessageHandler*>(make_id(remote_type, msg_type), message_handler));
}

void HandlerManager::register_async_request(unsigned char remote_type, unsigned short msg_type, App_Request_Handler& app_handler)
{
	message_handlers_.insert(std::pair<unsigned int, MessageHandler*>(make_id(remote_type, msg_type), new RequestHandler(app_handler)));
}

void HandlerManager::register_async_ack(unsigned char remote_type, unsigned short msg_type, App_Async_Ack_Handler& app_handler)
{
	message_handlers_.insert(std::pair<unsigned int, MessageHandler*>(make_id(remote_type, msg_type), new AsyncAckHandler(app_handler)));
}

unsigned int HandlerManager::make_id(unsigned char remote_type, unsigned short msg_type)
{
	int id = remote_type << 16;
	id += msg_type;

	return id;
}

HandlerManager& HandlerManager::get()
{
	static HandlerManager handler_manager;
	return handler_manager;
}

void HandlerManager::dispatch_message_handler(Connection &connection)
{
	Message& message = connection.get_message();
	unsigned short msg_type;
	message >> msg_type;
	boost::unordered_map<unsigned int, MessageHandler*>::iterator pos = message_handlers_.find(make_id(connection.remote_type(), msg_type));
	if (pos != message_handlers_.end())
	{
		pos->second->handle_message();
	}
}