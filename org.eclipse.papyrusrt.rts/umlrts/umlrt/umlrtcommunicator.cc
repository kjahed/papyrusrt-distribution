// umlrtcommunicator.cc

/*******************************************************************************

*******************************************************************************/

// UMLRTCommunicator manages all external communication.

#include "basedebug.hh"
#include "basefatal.hh"
#include "umlrtcommunicator.hh"
#include "umlrthost.hh"
#include "nanomsg/nn.h"
#include "nanomsg/pair.h"
#include "nanomsg/reqrep.h"
#include <unistd.h>
#include <stdio.h>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>
using namespace rapidjson;

UMLRTCommunicator::UMLRTCommunicator ( const char * _localaddr )
	: localaddr(_localaddr), abort(false)
{
	localsock = nn_socket(AF_SP, NN_REP);
	if( nn_bind (localsock, localaddr) < 0 )
		FATAL("Cannot bind to address %s", localaddr);

	// int timeout = COMMUNICATOR_TIMEOUT;
	// if( nn_setsockopt (localsock, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout)) < 0 )
	// 	FATAL("Error setting socket options");
}   

void UMLRTCommunicator::setLocalHost( UMLRTHost * host )
{
	localhost = host;
}

void UMLRTCommunicator::sendDeployment ( UMLRTHost * host )
{
	const char * deploymentJson = UMLRTDeploymentMap::encode( );

	Document document;
	document.SetObject();
	Document::AllocatorType& allocator = document.GetAllocator();

	Value sender, receiver, deployment;
	sender.SetString(StringRef(localhost->name));
	receiver.SetString(StringRef(host->name));
	deployment.SetString(StringRef(deploymentJson));

	document.AddMember("sender", sender, allocator);
	document.AddMember("receiver", receiver, allocator);
	document.AddMember("deployment", deployment, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	document.Accept(writer);

	if ( nn_send (host->socket, buffer.GetString(), buffer.GetSize(), 0) < 0 )
		FATAL("Error sending deployment to host  %s\n", host->name);
	
	host->deployed = true;

	//printf("Sent deployment to host @ %s\n", host->address);
}


const char * UMLRTCommunicator::waitForDeployment ( )
{
	uint8_t* buffer;
	int recv = nn_recv ( localsock, &buffer, NN_MSG, 0 );
	if(recv < 0)
		FATAL("Error receiving deployment plan");

	StringStream stream((const char*)buffer);
	Document document;
	document.ParseStream(stream);

	const char * sender = document["sender"].GetString();
	const char * receiver = document["receiver"].GetString();
	const char * deployment = document["deployment"].GetString();

	UMLRTDeploymentMap::decode( deployment );
	return sender;

	//printf("Got deployment");
}

void UMLRTCommunicator::sendReadySignal ( UMLRTHost * host )
{
	Document document;
	document.SetObject();
	Document::AllocatorType& allocator = document.GetAllocator();

	Value sender, receiver, ready(true);
	sender.SetString(StringRef(localhost->name));
	receiver.SetString(StringRef(host->name));

	document.AddMember("sender", sender, allocator);
	document.AddMember("receiver", receiver, allocator);
	document.AddMember("ready", ready, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	document.Accept(writer);

	if ( nn_send (host->socket, buffer.GetString(), buffer.GetSize(), 0) < 0 )
		FATAL("Error sending ready signal to host  %s\n", host->name);
	
	host->signaled = true;

	//printf("Sent ready signal to host @ %s\n", host->address);
}

void UMLRTCommunicator::sendGoSignal ( UMLRTHost * host )
{
	Document document;
	document.SetObject();
	Document::AllocatorType& allocator = document.GetAllocator();

	Value sender, receiver, go(true);
	sender.SetString(StringRef(localhost->name));
	receiver.SetString(StringRef(host->name));

	document.AddMember("sender", sender, allocator);
	document.AddMember("receiver", receiver, allocator);
	document.AddMember("go", go, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	document.Accept(writer);

	if ( nn_send (host->socket, buffer.GetString(), buffer.GetSize(), 0) < 0 )
		FATAL("Error sending ready signal to host  %s\n", host->name);

	host->goSignaled = true;

	//printf("Sent ready signal to host @ %s\n", host->address);
}

void UMLRTCommunicator::waitForReadySignal ( UMLRTHost * host )
{
	uint8_t* buffer;
	int recv = nn_recv ( localsock, &buffer, NN_MSG, 0 );
	if(recv < 0)
		FATAL("Error receiving ready signal");

	StringStream stream((const char*)buffer);
	Document document;
	document.ParseStream(stream);

	const char * sender;
	const char * receiver;
	bool ready;

	sender = document["sender"].GetString();
	receiver = document["receiver"].GetString();
	ready = document["ready"].GetBool();
	if(ready)
		host->gotack = true;

	//printf("Got ready signal from %s\n", (char*) buffer);
}

void UMLRTCommunicator::waitForGoSignal ( UMLRTHost * host )
{
	uint8_t* buffer;
	int recv = nn_recv ( localsock, &buffer, NN_MSG, 0 );
	if(recv < 0)
		FATAL("Error receiving go signal");

	StringStream stream((const char*)buffer);
	Document document;
	document.ParseStream(stream);

	const char * sender;
	const char * receiver;
	bool go;

	sender = document["sender"].GetString();
	receiver = document["receiver"].GetString();
	go = document["go"].GetBool();
}

void UMLRTCommunicator::connect ( UMLRTHost * host )
{

	host->socket = nn_socket(AF_SP, NN_REQ);
	if( nn_connect(host->socket, host->address) < 0 )
		FATAL("Unable to connect to host @ %s\n", host->address);

	// int timeout = COMMUNICATOR_TIMEOUT;
	// if( nn_setsockopt (host->socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout)) < 0 )
	// 	FATAL("Error setting socket options");

	host->connected = true;

	//printf("Connected to host @ %s\n", host->address);
}

void UMLRTCommunicator::disconnect ( UMLRTHost * host )
{
	if(host->socket != 0)
		nn_shutdown(host->socket, 0);

	host->connected = false;
}

void UMLRTCommunicator::queueMessage ( UMLRTCommunicator::Message* msg )
{
	messageQueue.enqueue(msg);
}

UMLRTCommunicator::Message* UMLRTCommunicator::sendrecv()
{
	static char metadata[256];
	static char separator[] = {',','\0'};

	UMLRTCommunicator::Message* msg = (UMLRTCommunicator::Message*) messageQueue.dequeue();
	if(msg != NULL)
	{
		sprintf(metadata, "%s,%d,%d,%s,%d,%d,%s,%s,%lu,",
			msg->destSlot
			, msg->destPort
			, msg->destInternal
			, msg->srcSlot
			, msg->srcPort
			, msg->srcInternal
			, msg->protocolName
			, msg->signalName
			, msg->payloadSize);

		int metadataLen = strlen(metadata);
		int bufferSize = metadataLen+msg->payloadSize;

		char* buffer = (char*) malloc(bufferSize);
		memcpy(buffer, metadata, metadataLen);
		memcpy(buffer+metadataLen, msg->payload, msg->payloadSize);

    	if ( nn_send (msg->destHost->socket, buffer, bufferSize, 0) < 0 )
			FATAL("Error sending message %s to host @ %s\n", buffer, msg->destHost->name);
		
		//printf("Sent message %s to host @ %s\n", buffer, msg->destHost->name);

		free(buffer);
		msg->release();
		delete msg;
	}

	char* buffer;
	int recv = nn_recv (localsock, &buffer, NN_MSG, NN_DONTWAIT);
	if(recv > 0)
	{

        UMLRTCommunicator::Message* msg = new UMLRTCommunicator::Message;
        char* token = strtok(buffer, separator);
        if(token == NULL)
        	return NULL;

        msg->destSlot = token;

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->destPort = atoi(token);

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->destInternal = atoi(token);

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->srcSlot = token;

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

	 	msg->srcPort = atoi(token);

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->srcInternal = atoi(token);

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->protocolName = token;

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->signalName = token;

        token = strtok(NULL, separator);
        if(token == NULL)
        	return NULL;

        msg->payloadSize = atoi(token);
        msg->payload =  buffer+recv-msg->payloadSize;

        //printf("Received message %s,%d,%s,%d,%s,%s,%lu,%s\n", msg->destSlot, msg->destPort, msg->srcSlot, msg->srcPort, msg->protocolName, msg->signalName, msg->payloadSize, msg->payload);

		return msg;
	}

	return NULL;
}
