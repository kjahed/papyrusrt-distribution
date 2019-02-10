// umlrtexecutiondirector.cc

#include "umlrtexecutiondirector.hh"
#include "umlrtdeploymentmap.hh"
#include "umlrtframeservice.hh"
#include "umlrtcommsport.hh"
#include "umlrtcommsportfarend.hh"
#include "umlrtcapsule.hh"
#include "umlrtcapsulepart.hh"
#include "umlrtcontroller.hh"
#include "umlrtoutsignal.hh"
#include "umlrtslot.hh"
#include "umlrtsignalregistry.hh"
#include "umlrthost.hh"
#include "umlrtprotocol.hh"
#include "basedebug.hh"
#include "basefatal.hh"
#include <unistd.h>
#include <string.h>

/*static*/ UMLRTExecutionDirector* UMLRTExecutionDirector::instance = NULL;

/*static*/ void UMLRTExecutionDirector::init ( const char * localaddr )
{
	UMLRTExecutionDirector::instance = new UMLRTExecutionDirector(localaddr);
}

UMLRTExecutionDirector::UMLRTExecutionDirector ( const char * _localaddr )
	: UMLRTBasicThread("ExecutionDirector"), localaddr(_localaddr), abort(false)
{
    slotsCount = UMLRTDeploymentMap::getDefaultSlotList(&slots);
}    

/*static*/ bool UMLRTExecutionDirector::sendSignal ( const UMLRTCommsPort * destPort, const UMLRTSignal &signal, size_t srcPortIndex )
{
    if (instance->communicator == NULL)
        FATAL("Destination slot %s is remote but no communicator instance found.", destPort->slot);

    UMLRTCommunicator::Message* msg = new UMLRTCommunicator::Message;
    msg->destHost = destPort->slot->host;

    msg->destSlot = destPort->slot->name;
    msg->srcSlot = signal.getSrcPort()->slot->name;

    msg->destPort = destPort->roleIndex;
    msg->srcPort = signal.getSrcPort()->roleIndex;
    msg->destInternal = destPort->border ? 0 : 1;
    msg->srcInternal = signal.getSrcPort()->border ? 0 : 1;

    msg->protocolName = signal.getSrcPort()->role()->protocol;
    msg->signalName = strdup(signal.getName());

    //msg->payload = (uint8_t*) malloc(signal.getPayloadSize());
    //memcpy(msg->payload, signal.getPayload(), signal.getPayloadSize());
    std::string json = UMLRTSignalRegistry::getRegistry().toJSON(signal);
    msg->payload = strdup(json.c_str());
    msg->payloadSize = json.length()+1;

    instance->communicator->queueMessage( msg );    
    return true;
}

/*static*/void UMLRTExecutionDirector::spawn ( )
{
	instance->spawnThread();
}


/*static*/void  UMLRTExecutionDirector::join ( )
{
	instance->joinThread();
}

void  UMLRTExecutionDirector::spawnThread ( )
{
	start(NULL);
}

void  UMLRTExecutionDirector::joinThread ( )
{
	UMLRTBasicThread::join();
}

void  UMLRTExecutionDirector::deploy ( )
{
    for( int i=0; i<slotsCount; i++ )
    {
        // [FIXME] we should only init capsules of non-remote slots!!
        slots[i].capsuleClass->create(&slots[i]);

        const char* controllerName = UMLRTDeploymentMap::getControllerNameForCapsule(slots[i].name);
        if( controllerName != NULL )
        {
            UMLRTHost * host = UMLRTDeploymentMap::getHostForController(controllerName);
            if ( host == NULL )
                FATAL( "Missing host for controller %s", controllerName);

            if( localhost == host )
            {
                UMLRTController* controller = UMLRTDeploymentMap::getControllerFromName(controllerName);
                if( controller == NULL )
                    controller = new UMLRTController( (char*)controllerName );

                slots[i].controller = controller;
            }
            else
            {
                slots[i].remote = true;
                slots[i].host = host;
            }

            UMLRTSignal emptySignal;
	        emptySignal.initialize("INITIALIZE", UMLRTSignal::invalidSignalId);
	        UMLRTFrameService::initializeCapsule(slots[i].capsule, emptySignal);
        }
    }

    for( int i=0; i<slotsCount; i++ )
    {
        const char* controllerName = UMLRTDeploymentMap::getControllerNameForCapsule(slots[i].name);
        if( controllerName != NULL )
        {
            UMLRTSignal emptySignal;
	        emptySignal.initialize("INITIALIZE", UMLRTSignal::invalidSignalId);
	        UMLRTFrameService::initializeCapsule(slots[i].capsule, emptySignal);
        }
    }
}

void * UMLRTExecutionDirector::runLocal ( void * args )
{
    UMLRTController* defaultController = NULL;

	
    for( int i=0; i<slotsCount; i++ )
    {
        slots[i].capsuleClass->create(&slots[i]);

        const char* controllerName = UMLRTDeploymentMap::getControllerNameForCapsule(slots[i].name);
        if( controllerName != NULL )
        {
            UMLRTController* controller = UMLRTDeploymentMap::getControllerFromName(controllerName);
            if( controller == NULL )
                controller = new UMLRTController( (char*)controllerName );

            slots[i].controller = controller;
        }
        else
        {
            if( defaultController == NULL)
                defaultController = new UMLRTController( "DefaultController" );

            slots[i].controller = defaultController;
        }
    }

    for( int i=0; i<slotsCount; i++ )
    {
        UMLRTSignal emptySignal;
	    emptySignal.initialize("INITIALIZE", UMLRTSignal::invalidSignalId);
	    UMLRTFrameService::initializeCapsule(slots[i].capsule, emptySignal);
    }

    UMLRTDeploymentMap::spawnAllControllers();
    UMLRTDeploymentMap::joinAllControllers();
    return NULL;
}

void * UMLRTExecutionDirector::run ( void * args )
{
    // if no address was provided, this is a local single threaded run
    if(localaddr == NULL)
        return runLocal( args );

    communicator = new UMLRTCommunicator( localaddr );

    // client mode: wait for parent to send deployment
    if( !UMLRTDeploymentMap::isLoaded() )
        communicator->waitForDeployment();

    localhost = UMLRTDeploymentMap::getHostFromAddress( localaddr );
    if( localhost == NULL )
        FATAL( "Local host with address '%s' not found in deployment plan", localaddr );
    communicator->setLocalHost(localhost);

    deploy();

    // send deployment to children
    for( int i=0; i<slotsCount; i++ )
    {
        if( !slots[i].remote )
        {
            for( int j=0; j<slots[i].numParts; j++ )
            {
                for( int k=0; k<slots[i].parts[j].numSlot; k++ )
                {
                    UMLRTSlot * childSlot = slots[i].parts[j].slots[k];
                    
                    if( childSlot->remote )
                    {
                        UMLRTHost * childHost = childSlot->host;

                         if( !childHost->connected )
                            communicator->connect(childHost);
                            
                         if( !childHost->deployed )
                         {
                            communicator->sendDeployment( childHost );
                            communicator->waitForReadySignal( childHost );
                         }
                    }
                }
            }
        }
    }

    // ack deployment
    for( int i=0; i<slotsCount; i++ )
    {
        if( slots[i].remote )
        {
            UMLRTHost * parentHost = slots[i].host;

            for( int j=0; j<slots[i].numParts; j++ )
            {
                for( int k=0; k<slots[i].parts[j].numSlot; k++ )
                {
                    UMLRTSlot * childSlot = slots[i].parts[j].slots[k];
                    
                    if( !childSlot->remote )
                    {
                         if( !parentHost->connected )
                            communicator->connect( parentHost );

                         if( !parentHost->signaled )
                            communicator->sendReadySignal( parentHost);
                    }
                }
            }
        }
    }

    // wait for go signal
    for( int i=0; i<slotsCount; i++ )
    {
        if( slots[i].remote )
        {
            UMLRTHost * parentHost = slots[i].host;

            for( int j=0; j<slots[i].numParts; j++ )
            {
                for( int k=0; k<slots[i].parts[j].numSlot; k++ )
                {
                    UMLRTSlot * childSlot = slots[i].parts[j].slots[k];

                    if( !childSlot->remote )
                    {
                         communicator->waitForGoSignal( parentHost );
                    }
                }
            }
        }
    }

    // send go signal
    for( int i=0; i<slotsCount; i++ )
    {
        if( !slots[i].remote )
        {
            for( int j=0; j<slots[i].numParts; j++ )
            {
                for( int k=0; k<slots[i].parts[j].numSlot; k++ )
                {
                    UMLRTSlot * childSlot = slots[i].parts[j].slots[k];

                    if( childSlot->remote )
                    {
                        UMLRTHost * childHost = childSlot->host;

                         if( !childHost->goSignaled )
                         {
                        	 communicator->sendGoSignal( childHost );
                         }
                    }
                }
            }
        }
    }

	for( int i=0; i<slotsCount; i++ )
	{
		if( slots[i].remote && !slots[i].host->connected )
			communicator->connect(slots[i].host);
	}

//    for( int i=0; i<slotsCount; i++ )
//    {
//        if ( !slots[i].remote )
//        {
//            //connect to remote ports
//            for ( int j=0; j<slots[i].numPorts; j++ )
//            {
//                for(int k=0; k < slots[i].ports[j].numFarEnd; k++)
//                {
//                    if(slots[i].ports[j].farEnds[k].port != NULL)
//                    {
//                        UMLRTSlot * farEndSlot = slots[i].ports[j].farEnds[k].port->slot;
//                        if( farEndSlot->remote )
//                        {
//                            if( !farEndSlot->host->connected )
//                                communicator->connect(farEndSlot->host);
//                        }
//                    }
//                }
//            }
//        }
//        else
//        {
//            //connect to remote ports
//            for ( int j=0; j<slots[i].numPorts; j++ )
//            {
//                for(int k=0; k < slots[i].ports[j].numFarEnd; k++)
//                {
//                    if(slots[i].ports[j].farEnds[k].port != NULL)
//                    {
//                        UMLRTSlot * farEndSlot = slots[i].ports[j].farEnds[k].port->slot;
//                        if( !farEndSlot->remote )
//                        {
//                            if( !slots[i].host->connected )
//                                communicator->connect(slots[i].host);
//                        }
//                    }
//                }
//            }
//        }
//    }

    //communicator->spawn();
    UMLRTDeploymentMap::spawnAllControllers();

    while(!abort)
    {
        UMLRTCommunicator::Message * msg = communicator->sendrecv();
        if(msg == NULL)
        {
            usleep(100);
            continue;
        }

        UMLRTSlot* destSlot = UMLRTDeploymentMap::getSlotFromName(msg->destSlot);
        if(destSlot == NULL)
         FATAL("Invalid destination slot %s", msg->destSlot);

        UMLRTSlot* srcSlot = UMLRTDeploymentMap::getSlotFromName(msg->srcSlot);
        if(srcSlot == NULL)
         FATAL("Invalid source slot %s", msg->srcSlot);

        const UMLRTCommsPort * srcPort;
        if(msg->srcInternal)
        	srcPort = srcSlot->capsule->getInternalPorts()[msg->srcPort];
        else
        	srcPort = srcSlot->capsule->getBorderPorts()[msg->srcPort];

        UMLRTSignal signal;
        UMLRTSignalRegistry::getRegistry().fromJSON((const char*)msg->payload, srcPort, signal);

        if(destSlot->capsule == NULL)
            FATAL("No capsule in destination slot");

        if(destSlot->controller == NULL)
            FATAL("No controller for capsule %s", destSlot->capsule->getName());

        const UMLRTCommsPort * destPort;
        if(msg->destInternal)
            destPort = destSlot->capsule->getInternalPorts()[msg->destPort];
        else
            destPort = destSlot->capsule->getBorderPorts()[msg->destPort];

        bool ok = destSlot->controller->deliver(destPort, signal,  msg->srcPort);
        if(!ok)
            FATAL("Error delivering signal to controller");

        // don't release to keep msg->signalName && msg->payload
        delete msg;
    }

    UMLRTDeploymentMap::joinAllControllers();

	return NULL;
}
