// umlrtdeploymentmap.cc

/*******************************************************************************

*******************************************************************************/

#include "umlrtcapsule.hh"
#include "umlrtdeploymentmap.hh"
#include "umlrtcontroller.hh"
#include "umlrtslot.hh"
#include "umlrthost.hh"
#include "umlrthashmap.hh"
#include "basefatal.hh"
#include "basedebug.hh"
#include "basedebugtype.hh"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <cstdio>
#include <fstream>
#include "osutil.hh"

#include "rapidjson/document.h"

UMLRTHashMap * UMLRTDeploymentMap::capsuleToControllerMap = NULL;
UMLRTHashMap * UMLRTDeploymentMap::controllerToHostMap = NULL;
UMLRTHashMap * UMLRTDeploymentMap::controllerNameMap = NULL;
UMLRTHashMap * UMLRTDeploymentMap::capsuleNameMap = NULL;
UMLRTHashMap * UMLRTDeploymentMap::hostNameMap = NULL;
UMLRTHashMap * UMLRTDeploymentMap::slotNameMap = NULL;

UMLRTSlot * UMLRTDeploymentMap::defaultSlotList = NULL;
int UMLRTDeploymentMap::numDefaultSlotList = 0;

const char * UMLRTDeploymentMap::payload = NULL;

// The 3 maps above are created dynamically and persist forever.
// capsuleToControllerMap
//   - created during the reading of the top.controllers file.
//   - 'objects' in capsuleToControllerMap are maps themselves (and also persist forever).
//   - these 'object-maps' will map a 'capsule class' to a 'controller' for an individual capsule instance.
//   - these 'object-maps' will generally contain one element with key == NULL, indicating the default controller for all capsule classes
//     (for the capsule instance) not explicitly mapped elsewhere in the map.
//   - one 'object' in each map 'capsuleToControllerMap' have
// controllerNameMap
//   - is created (and populated) during controller construction and elements persist forever (as dynamic controller creation/deletion is not supported).
// capsuleNameMap
//   - is created (and populated) during capsule construction. Elements are deleted as capsules are destroyed.
//   - elements associated with the static sub-structure of Top persist forever (as these capsules are never destroyed).

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getControllerNameMap()
{
    if (controllerNameMap == NULL)
    {
        controllerNameMap = new UMLRTHashMap("controllerNameMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return controllerNameMap;
}

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getCapsuleToControllerMap()
{
    if (capsuleToControllerMap == NULL)
    {
        capsuleToControllerMap = new UMLRTHashMap("capsuleToControllerMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return capsuleToControllerMap;
}

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getControllerToHostMap()
{
    if (controllerToHostMap == NULL)
    {
        controllerToHostMap = new UMLRTHashMap("controllerToHostMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return controllerToHostMap;
}

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getCapsuleNameMap()
{
    if (capsuleNameMap == NULL)
    {
        capsuleNameMap = new UMLRTHashMap("capsuleNameMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return capsuleNameMap;
}

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getHostNameMap()
{
    if (hostNameMap == NULL)
    {
        hostNameMap = new UMLRTHashMap("hostNameMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return hostNameMap;
}

/*static*/ UMLRTHashMap * UMLRTDeploymentMap::getSlotNameMap()
{
    if (slotNameMap == NULL)
    {
        slotNameMap = new UMLRTHashMap("slotNameMap", UMLRTHashMap::compareString, false/*objectIsString*/);
    }
    return slotNameMap;
}

// Add a controller by name.
/*static*/ bool UMLRTDeploymentMap::addController( const char * controllername, UMLRTController * controller )
{
    if (getControllerFromName(controllername) != NULL)
    {
        FATAL("ERROR: Controller %s has already registered.", controllername);
    }
    BDEBUG(BD_CONTROLLER, "add controller(%s) p(%p)\n", controllername, controller );

    getControllerNameMap()->insert( controllername, (void *)controller);

    return true;
}

// Add a capsule by name.
/*static*/ void UMLRTDeploymentMap::addCapsule( const char * capsulename, UMLRTCapsule * capsule )
{
    getCapsuleNameMap()->insert( capsulename, (void *)capsule);
}

// Add a controller by name.
/*static*/ bool UMLRTDeploymentMap::addHost( const char * hostName, UMLRTHost * host )
{
    if (getControllerFromName(hostName) != NULL)
    {
        FATAL("ERROR: Controller %s has already registered.", hostName);
    }
    BDEBUG(BD_CONTROLLER, "add host(%s) p(%p)\n", hostName, host );

    getHostNameMap()->insert( hostName, (void *)host);

    return true;
}

// Add a slot by name.
/*static*/  bool UMLRTDeploymentMap::addSlot ( const char * slotName, UMLRTSlot * slot )
{
    if (getSlotFromName(slotName) != NULL)
    {
        FATAL("ERROR: Slot %s has already registered.", slotName);
    }
    BDEBUG(BD_CONTROLLER, "add slot(%s) p(%p)\n", slotName, slot );

    getSlotNameMap()->insert( slotName, (void *)slot);

    return true;
}

/*static*/ const UMLRTCapsule * UMLRTDeploymentMap::getCapsuleFromName ( const char * capsulename )
{
    return (const UMLRTCapsule *)getCapsuleNameMap()->getObject(capsulename);
}

/*static*/ UMLRTController * UMLRTDeploymentMap::getControllerFromName( const char * controllerName )
{
    return (UMLRTController *)getControllerNameMap()->getObject(controllerName);
}

/*static*/ int UMLRTDeploymentMap::getDefaultSlotList ( UMLRTSlot * * slot_p )
{
    *slot_p = defaultSlotList;

    return numDefaultSlotList;
}

/*static*/ UMLRTHost * UMLRTDeploymentMap::getHostFromName( const char * hostName )
{
    return (UMLRTHost *)getHostNameMap()->getObject(hostName);
}

/*static*/ UMLRTHost * UMLRTDeploymentMap::getHostFromAddress( const char * hostAddress )
{
    getHostNameMap()->lock();
    UMLRTHashMap::Iterator iter = getHostNameMap()->getIterator();

    while (iter != iter.end())
    {
        UMLRTHost * host = (UMLRTHost *)((char *)iter.getObject());
        if( strcmp(hostAddress, host->address) == 0 )
        {
            getHostNameMap()->unlock();
            return host;
        }

        iter = iter.next();
    }
    getHostNameMap()->unlock();
    return NULL;
}

/*static*/ UMLRTSlot * UMLRTDeploymentMap::getSlotFromName( const char * slotName )
{
    return (UMLRTSlot *)getSlotNameMap()->getObject(slotName);
}

/*static*/ UMLRTController * UMLRTDeploymentMap::getFirstController()
{
    return (UMLRTController *)getControllerNameMap()->getFirstObject();
}

/*static*/ const char * UMLRTDeploymentMap::getControllerNameForCapsule ( const char * capsuleName )
{
    void * controllerName = getCapsuleToControllerMap()->getObject(capsuleName);
    if (controllerName != NULL)
        return (const char *) controllerName;

    return NULL;
}

// Return the controller assigned to this capsule. Returns NULL if no controller is assigned or doesn't exist.
/*static*/ UMLRTController * UMLRTDeploymentMap::getControllerForCapsule ( const char * capsuleName )
{
    const char * controllerName = getControllerNameForCapsule( capsuleName );

    BDEBUG(BD_CONTROLLERMAP, "getControllerForCapsule: capsule(%s)  map-controller(%s)\n",
            capsuleName,
            controllerName ? controllerName : "-no controller-");

    return (UMLRTController *)getControllerFromName(controllerName);
}

/*static*/ void UMLRTDeploymentMap::enqueueAbortAllControllers ( )
{
    getControllerNameMap()->lock();
    UMLRTHashMap::Iterator iter = getControllerNameMap()->getIterator();

    while (iter != iter.end())
    {
        UMLRTController * controller = (UMLRTController *)iter.getObject();
        BDEBUG(BD_CONTROLLERMAP, "enqueueAbortAllControllers: enqueue abort controller %s\n", controller->name());
        controller->enqueueAbort();
        iter = iter.next();
    }
    getControllerNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::enqueueExitAllControllers ( void * exitValue )
{
    getControllerNameMap()->lock();
    UMLRTHashMap::Iterator iter = getControllerNameMap()->getIterator();

    while (iter != iter.end())
    {
        UMLRTController * controller = (UMLRTController *)iter.getObject();
        BDEBUG(BD_CONTROLLERMAP, "enqueueExitAllControllers: enqueue exit controller %s\n", controller->name());
        controller->enqueueExit(exitValue);
        iter = iter.next();
    }
    getControllerNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::spawnAllControllers ( )
{
    getControllerNameMap()->lock();
    UMLRTHashMap::Iterator iter = getControllerNameMap()->getIterator();

    while (iter != iter.end())
    {
        UMLRTController * controller = (UMLRTController *)iter.getObject();
        controller->spawn();
        iter = iter.next();
    }
    getControllerNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::joinAllControllers ( )
{
    getControllerNameMap()->lock();
    UMLRTHashMap::Iterator iter = getControllerNameMap()->getIterator();

    while (iter != iter.end())
    {
        UMLRTController * controller = (UMLRTController *)iter.getObject();
        controller->join();
        iter = iter.next();
    }
    getControllerNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::addCapsuleToController ( char * capsuleName, char * controllerName )
{
    UMLRTController * controller = (UMLRTController *)getCapsuleToControllerMap()->getObject(capsuleName);

    if (controller != NULL)
        FATAL("capsule-to-controller-map already had an entry for capsule '%s'", capsuleName);
    else
        getCapsuleToControllerMap()->insert(strdup(capsuleName), (void *)strdup(controllerName));
}

/*static*/ void UMLRTDeploymentMap::addControllerToHost ( char * controllerName, char * hostName )
{
    UMLRTHost * host = (UMLRTHost *)getControllerToHostMap()->getObject(controllerName);

    if (host != NULL)
        FATAL("controller-to-host-map already had an entry for controller '%s'", controllerName);
    else
        getControllerToHostMap()->insert(strdup(controllerName), (void *)strdup(hostName));
}

/*static*/ const char * UMLRTDeploymentMap::getHostNameForController ( const char * controllerName )
{
    void * hostName = getControllerToHostMap()->getObject(controllerName);
    if (hostName != NULL)
        return (const char *) hostName;

    return NULL;
}

/*static*/ UMLRTHost * UMLRTDeploymentMap::getHostForController ( const char * controllerName )
{
    const char * hostName = getHostNameForController( controllerName );

    BDEBUG(BD_CONTROLLERMAP, "getHostForController: controller(%s) map-host(%s)\n",
            controllerName,
            hostName ? hostName : "-no host-");

    return (UMLRTHost *)getHostFromName(hostName);
}

// Return the host where this capsule is running. Returns NULL if no host is assigned or doesn't exist.
/*static*/ UMLRTHost * UMLRTDeploymentMap::getHostForCapsule ( const char * capsuleName )
{
    UMLRTHost* host = NULL;
    const char* controller = getControllerNameForCapsule( capsuleName );

    if( controller != NULL )
        host = getHostForController(controller);

    return host;
}

// Remove a capsule instance from the list.
/*static*/ void UMLRTDeploymentMap::removeCapsule ( const char * capsuleName, UMLRTCapsule * capsule )
{
    getCapsuleNameMap()->remove(capsuleName);
}

// Remove a controller instance from the list.
/*static*/ void UMLRTDeploymentMap::removeController ( const char * controllerName )
{
    getControllerNameMap()->remove(controllerName);
}

// Remove a controller instance from the list.
/*static*/ void UMLRTDeploymentMap::removeHost ( const char * hostName )
{
    getHostNameMap()->remove(hostName);
}

// Assign static capsule controllers based on the run-time map.
/*static*/ void UMLRTDeploymentMap::setDefaultSlotList ( UMLRTSlot * slots, size_t size )
{
    for (size_t i = 0; i < size; ++i)
    {
        addSlot( slots[i].name , &slots[i] );

        UMLRTController * controller = getControllerForCapsule(slots[i].name);

        BDEBUG(BD_CONTROLLERMAP, "setDefaultSlotList: slot[%d] slot-controller(%s) map-controller(%s)\n",
                i,
                slots[i].controller ? slots[i].controller->name() : "-none in slot-",
                controller ? controller->name() : "-no mapped controller-");

        // Only reassign static capsules (i.e. controller already assigned) whose controllers were specified in the run-time map.
        if ((slots[i].controller != NULL) && (controller != NULL))
        {
            // Reassign the capsule according to the run-time map collected.
            slots[i].controller = controller;
        }
    }
    defaultSlotList = slots;
    numDefaultSlotList = size;
}

/*static*/ bool UMLRTDeploymentMap::fromFile( const char* fileName )
{
	std::string line, body;
	std::ifstream file(fileName);
	if(file) {
		while(std::getline(file, line)) {
			body += line + "\n";
		}

		const char* data = body.c_str();
		decode(data);
		return true;
	}

   return false;
}

/*static*/ void UMLRTDeploymentMap::decode( const char* json )
{
	rapidjson::StringStream stream(json);
	rapidjson::Document document;
	document.ParseStream(stream);

	const rapidjson::Value& hosts = document["hosts"];
	for (rapidjson::Value::ConstValueIterator itr = hosts.Begin(); itr != hosts.End(); ++itr) {
		const char * hostName = (*itr)["name"].GetString();
		const char * hostAddress = (*itr)["address"].GetString();

		UMLRTHost* host = UMLRTDeploymentMap::getHostFromName( hostName );
		if( host != NULL )
		{
			if( strcmp( host->address, hostAddress ) != 0 )
				FATAL( "Duplicate hostname: %s\n", hostName );
		}
		else
		{
			new UMLRTHost( (char*)hostName, (char*)hostAddress );
		}
	}

	const rapidjson::Value& controllers = document["controllers"];
	for (rapidjson::Value::ConstValueIterator itr = controllers.Begin(); itr != controllers.End(); ++itr) {
		const char * controllerName = (*itr)["name"].GetString();
		const char * hostName = (*itr)["host"].GetString();

        UMLRTHost* host = UMLRTDeploymentMap::getHostFromName( hostName );
        if(host == NULL)
            FATAL("No such host: %s\n", hostName);

        UMLRTDeploymentMap::addControllerToHost( (char*)controllerName, (char*)hostName );
	}

	const rapidjson::Value& capsules = document["capsules"];
	for (rapidjson::Value::ConstValueIterator itr = capsules.Begin(); itr != capsules.End(); ++itr) {
		const char * capsuleName = (*itr)["name"].GetString();
		const char * controllerName = (*itr)["controller"].GetString();

		//TODO: check that controller exists
		UMLRTDeploymentMap::addCapsuleToController( (char*)capsuleName, (char*)controllerName );
	}

	if(payload != NULL)
		free((void*)payload);
	payload = strdup(json);
}

/*static*/ const char * UMLRTDeploymentMap::encode( )
{
    if( payload == NULL )
        FATAL("Deployment map not encoded yet\n");

    return payload;
}

/*static*/ bool UMLRTDeploymentMap::isLoaded( )
{
    return payload != NULL;
}

// Debug output the the capsule, controller and capsule-to-controller maps.
/*static*/ void UMLRTDeploymentMap::debugOutputControllerList ( )
{
    BDEBUG(BD_MODEL, "Controller list: { <controller name>, <instance address> }\n");

    getControllerNameMap()->lock();
    UMLRTHashMap::Iterator iter = getControllerNameMap()->getIterator();

    if (iter == iter.end())
    {
        BDEBUG(BD_MODEL, "    No controllers.\n");
    }
    else
    {
        while (iter != iter.end())
        {
            BDEBUG(BD_MODEL, "    { %s, %p }\n", ((UMLRTController *)((char *)iter.getObject()))->name(), (UMLRTController *)((char *)iter.getObject()));
            iter = iter.next();
        }
    }
    getControllerNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::debugOutputCapsuleList ( )
{
    BDEBUG(BD_MODEL, "Capsule list: { <capsule name>, <capsule role>, <instance address>, <capsule class>, <assigned controller> }\n");

    getCapsuleNameMap()->lock();
    UMLRTHashMap::Iterator iter = getCapsuleNameMap()->getIterator();

    if (iter == iter.end())
    {
        BDEBUG(BD_MODEL, "    No capsules.\n");
    }
    else
    {
        while (iter != iter.end())
        {
            UMLRTCapsule * capsule = (UMLRTCapsule *)iter.getObject();
            BDEBUG(BD_MODEL, "    { %s, %s, %p, %s, %s }\n",
                    capsule->name(), capsule->getName(), capsule, capsule->getTypeName(),
                    capsule->getSlot()->controller ? capsule->getSlot()->controller->name() : "no owning controller");
            iter = iter.next();
        }
    }
    getCapsuleNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::debugOutputHostList ( )
{
    BDEBUG(BD_MODEL, "Host list: { <host name>, <instance address> }\n");

    getHostNameMap()->lock();
    UMLRTHashMap::Iterator iter = getHostNameMap()->getIterator();

    if (iter == iter.end())
    {
        BDEBUG(BD_MODEL, "    No hosts.\n");
    }
    else
    {
        while (iter != iter.end())
        {
            BDEBUG(BD_MODEL, "    { %s, %p }\n", ((UMLRTHost *)((char *)iter.getObject()))->name, (UMLRTHost *)((char *)iter.getObject()));
            iter = iter.next();
        }
    }
    getHostNameMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::debugOutputSlotList ( )
{
    BDEBUG(BD_MODEL, "Host list: { <slot name>, <instance address> }\n");

    getSlotNameMap()->lock();
    UMLRTHashMap::Iterator iter = getSlotNameMap()->getIterator();

    if (iter == iter.end())
    {
        BDEBUG(BD_MODEL, "    No slots.\n");
    }
    else
    {
        while (iter != iter.end())
        {
            BDEBUG(BD_MODEL, "    { %s, %p }\n", ((UMLRTSlot *)((char *)iter.getObject()))->name, (UMLRTSlot *)((char *)iter.getObject()));
            iter = iter.next();
        }
    }
    getSlotNameMap()->unlock();
}

// Debug output of capsule-to-controller map.
/*static*/ void UMLRTDeploymentMap::debugOutputCaspuleToControllerMap ( )
{
    BDEBUG(BD_MODEL, "Capsule to controller map: { <slot>, <controller>  }\n");

    getCapsuleToControllerMap()->lock();
    UMLRTHashMap::Iterator ctclIter = getCapsuleToControllerMap()->getIterator();

    if (ctclIter == ctclIter.end())
    {
        BDEBUG(BD_MODEL, "    No capsule to controller assignments.\n");
    }
    else
    {
        while (ctclIter != ctclIter.end())
        {
            const char * capsuleName = (const char *)ctclIter.getKey();

            BDEBUG(BD_MODEL, "    { %s, %s }\n",
                                capsuleName,
                                (ctclIter.getObject() == NULL) ? "?no controller?" : ctclIter.getObject());
            ctclIter = ctclIter.next();
        }
    }
    getCapsuleToControllerMap()->unlock();
}

/*static*/ void UMLRTDeploymentMap::debugOutputControllerToHostMap ( )
{
    BDEBUG(BD_MODEL, "Controller to host map: { <controller>, <host> }\n");

    getControllerToHostMap()->lock();
    UMLRTHashMap::Iterator clIter = getControllerToHostMap()->getIterator();

    if (clIter == clIter.end())
    {
        BDEBUG(BD_MODEL, "    No controller to host assignments.\n");
    }
    else
    {
        while (clIter != clIter.end())
        {
            const char * controllerName = (const char *)clIter.getKey();
            
            BDEBUG(BD_MODEL, "    { %s, %s }\n",
                                controllerName,
                                (clIter.getObject() == NULL) ? "?no host?" : clIter.getObject());
            clIter = clIter.next();
        }
    }
    getControllerToHostMap()->unlock();
}
