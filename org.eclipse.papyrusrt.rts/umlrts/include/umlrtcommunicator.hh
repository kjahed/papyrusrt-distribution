// umlrtcommunicator.hh

/*******************************************************************************
* Copyright (c) 2014-2015 Zeligsoft (2009) Limited  and others.
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*******************************************************************************/

#ifndef UMLRTCOMMUNICATOR_HH
#define UMLRTCOMMUNICATOR_HH

// UMLRTCommunicator is the main controller-class.

#include "umlrtqueue.hh"
#include "umlrtqueueelement.hh"
#include <stdlib.h>

struct UMLRTHost;

#define COMMUNICATOR_TIMEOUT 1000

class UMLRTCommunicator
{
public:
    UMLRTCommunicator ( const char * _localaddr );
    
    class Message : public UMLRTQueueElement {
        public:
            void release()
            {
                if(signalName != NULL)
                    free((void*)signalName);
                if(payload != NULL)
                    free((void*)payload);
            }

            UMLRTHost * destHost;
            const char * destSlot;
            const char * srcSlot;

            int destPort;
            int srcPort;
            bool destInternal;
            bool srcInternal;

            const char * protocolName;
            const char * signalName;
            const char * payload;
            int payloadSize;
    };

    void setLocalHost( UMLRTHost * host );
    void connect ( UMLRTHost * host );
    void sendDeployment ( UMLRTHost * host );
    void sendReadySignal ( UMLRTHost * host );
    void sendGoSignal ( UMLRTHost * host );
    void waitForReadySignal ( UMLRTHost * host );
    void waitForGoSignal ( UMLRTHost * host );
    const char *  waitForDeployment ( );

    void queueMessage ( UMLRTCommunicator::Message * msg );
    UMLRTCommunicator::Message* sendrecv();

private:    
    const char * localaddr;
    UMLRTHost * localhost;
    int localsock;
    bool abort;

    UMLRTQueue messageQueue;
    
    void disconnect ( UMLRTHost * host );
    void handshake ( UMLRTHost * host );
    void quit ( );
};

#endif // UMLRTCOMMUNICATOR_HH
