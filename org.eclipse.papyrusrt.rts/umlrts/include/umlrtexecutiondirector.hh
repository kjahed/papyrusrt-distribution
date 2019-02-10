// umlrtexecutiondirector.hh

/*******************************************************************************

*******************************************************************************/

#ifndef UMLRTEXECUTIIONDIRECTOR_HH
#define UMLRTEXECUTIIONDIRECTOR_HH

// UMLRTExecutionDirector is the main driver class.

#include "umlrtbasicthread.hh"
#include "umlrtcommunicator.hh"

struct UMLRTSlot;
struct UMLRTHost;
struct UMLRTSignal;
struct UMLRTCommsPort;

class UMLRTExecutionDirector : UMLRTBasicThread
{
public:
	static void init ( const char * localaddr );
    static bool sendSignal ( const UMLRTCommsPort * destPort, const UMLRTSignal &signal, size_t srcPortIndex );

    static void spawn ( );
    static void join ( );
private:
	UMLRTExecutionDirector ( const char * _localaddr );

	static UMLRTExecutionDirector* instance;

	const char * localaddr;
    UMLRTHost * localhost;
    UMLRTSlot * slots;
    int slotsCount;
    bool abort;

    UMLRTCommunicator* communicator;

    void spawnThread ( );
    void joinThread ( );
    void deploy ( );

    void * runLocal ( void * args );

    // Main loop
	virtual void * run ( void * args );
};

#endif // UMLRTEXECUTIIONDIRECTOR_HH
