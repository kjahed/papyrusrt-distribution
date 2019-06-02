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

#include "flatbuffers/flatbuffers.h"
#include "fbschemas/deployment_generated.h"
#include "fbschemas/signal_generated.h"
#include "fbschemas/message_generated.h"

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
	std::string deploymentStr(deploymentJson);
	std::vector<uint8_t> deploymentData(deploymentStr.begin(), deploymentStr.end());

	flatbuffers::FlatBufferBuilder builder(1024);

	auto sender = builder.CreateString(localhost->name);
	auto receiver = builder.CreateString(host->name);
	auto deployment = builder.CreateVector(deploymentData);

	FBSchema::MessageBuilder msgBuilder(builder);
	msgBuilder.add_sender(sender);
	msgBuilder.add_receiver(receiver);
	msgBuilder.add_type(FBSchema::Type_DEPLOYMENT);
	msgBuilder.add_deployment(deployment);

	auto messageObj = msgBuilder.Finish();    
    builder.Finish( messageObj );

    uint8_t* buffer = builder.GetBufferPointer( );
    int bufferSize = builder.GetSize( );

	if ( nn_send (host->socket, buffer, bufferSize, 0) < 0 )
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

	auto message = FBSchema::GetMessage(buffer);
	auto sender = message->sender();
	auto type = message->type();
	auto deployment = message->deployment();

	if(	!sender 
		|| type != FBSchema::Type_DEPLOYMENT 
		|| !deployment )
		FATAL("Unexpected message received");

	int deploymentBuffSize = deployment->Length();
	uint8_t * deploymentBuff = (uint8_t*) malloc(deploymentBuffSize);
	for(int i=0; i<deploymentBuffSize; i++)
		deploymentBuff[i] = deployment->Get(i);

	UMLRTDeploymentMap::decode( (const char*) deploymentBuff );
	return sender->c_str();

	//printf("Got deployment");
}

void UMLRTCommunicator::sendReadySignal ( UMLRTHost * host )
{
	flatbuffers::FlatBufferBuilder builder(1024);

	auto sender = builder.CreateString(localhost->name);
	auto receiver = builder.CreateString(host->name);

	FBSchema::MessageBuilder msgBuilder(builder);
	msgBuilder.add_sender(sender);
	msgBuilder.add_receiver(receiver);
	msgBuilder.add_type(FBSchema::Type_READY_SIGNAL);

	auto messageObj = msgBuilder.Finish();    
    builder.Finish( messageObj );

    uint8_t* buffer = builder.GetBufferPointer( );
    int bufferSize = builder.GetSize( );

	if ( nn_send (host->socket, buffer, bufferSize, 0) < 0 )
		FATAL("Error sending ready signal to host  %s\n", host->name);
	
	host->signaled = true;

	//printf("Sent ready signal to host @ %s\n", host->address);
}

void UMLRTCommunicator::sendGoSignal ( UMLRTHost * host )
{
	flatbuffers::FlatBufferBuilder builder(1024);

	auto sender = builder.CreateString(localhost->name);
	auto receiver = builder.CreateString(host->name);

	FBSchema::MessageBuilder msgBuilder(builder);
	msgBuilder.add_sender(sender);
	msgBuilder.add_receiver(receiver);
	msgBuilder.add_type(FBSchema::Type_GO_SIGNAL);

	auto messageObj = msgBuilder.Finish();
    builder.Finish( messageObj );

    uint8_t* buffer = builder.GetBufferPointer( );
    int bufferSize = builder.GetSize( );

	if ( nn_send (host->socket, buffer, bufferSize, 0) < 0 )
		FATAL("Error sending go signal to host  %s\n", host->name);

	host->goSignaled = true;

	//printf("Sent ready signal to host @ %s\n", host->address);
}

void UMLRTCommunicator::waitForReadySignal ( UMLRTHost * host )
{
	uint8_t* buffer;
	int recv = nn_recv ( localsock, &buffer, NN_MSG, 0 );
	if(recv < 0)
		FATAL("Error receiving ready signal");

	auto message = FBSchema::GetMessage(buffer);
	auto sender = message->sender();
	auto type = message->type();

	if(	!sender 
		|| type != FBSchema::Type_READY_SIGNAL)
		FATAL("Unexpected message received");
	
	host->gotack = true;

	//printf("Got ready signal from %s\n", (char*) buffer);
}

void UMLRTCommunicator::waitForGoSignal ( UMLRTHost * host )
{
	uint8_t* buffer;
	int recv = nn_recv ( localsock, &buffer, NN_MSG, 0 );
	if(recv < 0)
		FATAL("Error receiving go signal");

	auto message = FBSchema::GetMessage(buffer);
	auto sender = message->sender();
	auto type = message->type();

	if(	!sender
		|| type != FBSchema::Type_GO_SIGNAL)
		FATAL("Unexpected message received");
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
