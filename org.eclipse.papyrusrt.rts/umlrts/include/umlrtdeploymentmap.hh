// umlrtdeploymentmap.hh

/*******************************************************************************

*******************************************************************************/

#include "umlrthashmap.hh"
#include <stdlib.h>
#include <vector>

#ifndef UMLRTDEPLOYMENTMAP_HH
#define UMLRTDEPLOYMENTMAP_HH

#define UMLRTDEPLOYMENTMAP_SCHEMA_FILE "fbschemas/deployment.fbs"

class UMLRTCapsule;
class UMLRTController;
struct UMLRTSlot;
struct UMLRTHost;

class UMLRTDeploymentMap
{
public:

    // Assign a controller to a capsule (and optionally capsule class).
    static void addCapsuleToController ( char * capsuleName, char * controllerName );

    static void addControllerToHost ( char * controllerName, char * hostName );

    // Add a capsule to the list.
    static void addCapsule ( const char * capsulename, UMLRTCapsule * capsule );

    // Add a controller by name.
    static bool addController ( const char * controllerName, UMLRTController * controller );

    // Add a controller by name.
    static bool addHost ( const char * hostName, UMLRTHost * host );

    // Add a controller by name.
    static bool addSlot ( const char * slotName, UMLRTSlot * slot );

    // Enqueue abort commands to all controllers.
    static void enqueueAbortAllControllers ( );

    // Enqueue exit commands to all controllers.
    static void enqueueExitAllControllers ( void * exitValue );

    // Spawn all controllers' threads
    static void spawnAllControllers ( );

    // Join all controller's threads
    static void joinAllControllers ( );

    // Return the controller for a given name. Returns NULL if the controller is not found.
    static UMLRTController * getControllerFromName ( const char * controllerName );

    // Return the controller assigned to this capsule. Returns NULL if no controller is assigned or doesn't exist.
    static UMLRTController * getControllerForCapsule ( const char * capsuleName );

    // Return the controller name assigned to a capsule and its class.
    static const char * getControllerNameForCapsule ( const char * capsuleName );

    // Pass in the global slot list for processing after the capsule-to-controller map is processed.
    static void setDefaultSlotList ( UMLRTSlot * slots, size_t size );

    // // Return the first controller. Returns NULL if no controller defined.
    static UMLRTController * getFirstController ( );

    // Get a capsule by name.
    static const UMLRTCapsule * getCapsuleFromName ( const char * capsuleName );

    // Return the host where this capsule is running. Returns NULL if no host is assigned or doesn't exist.
    static UMLRTHost * getHostForCapsule ( const char * capsuleName );

    // Return the host assigned to this controller. Returns NULL if no controller is assigned or doesn't exist.
    static UMLRTHost * getHostForController ( const char * controllerName );

    // Return the host name assigned to a controller.
    static const char * getHostNameForController ( const char * controllerName );

    // Get the default slot list and return the number of slots in the list.
    static int getDefaultSlotList ( UMLRTSlot * * slots_p );

    // Get a host by name.
    static UMLRTHost * getHostFromName ( const char * hostName );

    // Get a host by address.
    static UMLRTHost * getHostFromAddress ( const char * hostAddress );

    // Get a slot by name.
    static UMLRTSlot * getSlotFromName ( const char * slotName );

    // Remove a capsule instance from the list.
    static void removeCapsule ( const char * capsulename, UMLRTCapsule * capsule );

    // Remove a controller instance from the list.
    static void removeController ( const char * controllerName );

    // Remove a host instance from the list.
    static void removeHost ( const char * hostName );

    static bool fromFile( const char* fileName );

    static void decode( uint8_t* flatbuffer, size_t flatbufferSize  );

    static std::vector<uint8_t> & encode( );

    static bool isLoaded( );

    // Debug output controller list.
    static void debugOutputControllerList ( );

    // Debug output the the capsule, controller and capsule-to-controller maps.
    static void debugOutputCapsuleList ( );

    // Debug output the the capsule, controller and capsule-to-controller maps.
    static void debugOutputHostList ( );

    // Debug output of slots list.
    static void debugOutputSlotList ( );

    // Debug output of capsule-to-controller map.
    static void debugOutputCaspuleToControllerMap ( );

    // Debug output of controller-to-host map.
    static void debugOutputControllerToHostMap ( );

private:
    // Parse an individual capsule-to-controller line.
    static bool parseCapsuleControllerLine ( char * line );

    // Parse an individual host line.
    static bool parseHostLine ( char * line );

    static UMLRTHashMap * getControllerNameMap ( );
    static UMLRTHashMap * getCapsuleNameMap ( );
    static UMLRTHashMap * getHostNameMap ( );
    static UMLRTHashMap * getSlotNameMap ( );
    static UMLRTHashMap * getCapsuleToControllerMap ( );
    static UMLRTHashMap * getControllerToHostMap ( );

    static UMLRTHashMap * controllerNameMap;
    static UMLRTHashMap * capsuleToControllerMap;
    static UMLRTHashMap * controllerToHostMap;
    static UMLRTHashMap * capsuleNameMap;
    static UMLRTHashMap * hostNameMap;
    static UMLRTHashMap * slotNameMap;

    static int numDefaultSlotList;
    static UMLRTSlot * defaultSlotList;

    static std::vector<uint8_t> payload;
};

#endif // UMLRTDEPLOYMENTMAP_HH
