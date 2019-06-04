// umlrtcapsule.cc

/*******************************************************************************
* Copyright (c) 2015 Zeligsoft (2009) Limited  and others.
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*******************************************************************************/

#include "umlrtcapsuleclass.hh"
#include "umlrtcapsule.hh"
#include "umlrtframeservice.hh"
#include "umlrtmessage.hh"
#include "basedebug.hh"
#include "basedebugtype.hh"

UMLRTCapsule::~UMLRTCapsule ( )
{
    BDEBUG(BD_INSTANTIATE, "%s destructor\n", slot->name);
    UMLRTDeploymentMap::removeCapsule(slot->name, this);
}

UMLRTCapsule::UMLRTCapsule ( const UMLRTRtsInterface * rtsif_, const UMLRTCapsuleClass * capsuleClass_, UMLRTSlot * slot, const UMLRTCommsPort * * borderPorts_, const UMLRTCommsPort * * internalPorts_, bool isStatic_ ) : msg(NULL), rtsif(rtsif_), capsuleClass(capsuleClass_), slot(slot), borderPorts(borderPorts_), internalPorts(internalPorts_), isStatic(isStatic_)
{
    BDEBUG(BD_INSTANTIATE, "slot %s constructor\n", slot->name);
    UMLRTDeploymentMap::addCapsule(slot->name, this);
}

void UMLRTCapsule::bindPort ( bool isBorder, int portIndex, int farEndIndex )
{
}

void UMLRTCapsule::unbindPort ( bool isBorder, int portIndex, int farEndIndex )
{
}

void UMLRTCapsule::logMsg ( )
{
    if (base::debugTypeEnabled(BD_LOGMSG))
    {
        const UMLRTMessage * msg = getMsg();
        fprintf(stdout, "LOGMSG: capsule %s received signal %s", name(), msg->signal.getName());
        if (msg->signal.getType() != NULL)
        {
            fprintf(stdout, " ");
            UMLRTObject_fprintf(stdout, msg->signal.getType(), msg->signal.getPayload(), 0/*nest*/, 1/*arraysize*/);
        }
        fprintf(stdout, "\n");
    }
}

void UMLRTCapsule::unexpectedMessage ( ) const
{
    fprintf( stderr, "Unexpected message to capsule instance %s role %s on port %s protocol %s signal %s\n",
            name(),
            getName(),
            getMsg()->sap() ? getMsg()->sap()->getName() : "(no sap)",
            getMsg()->sap() ? (getMsg()->sap()->role() ? getMsg()->sap()->role()->protocol : "(sap no role)") : "(no sap)",
            getMsg()->getSignalName());
}

UMLRTCapsule::Serializer::Serializer ( )
{
	document.SetObject();
	fields.SetArray();
}

void UMLRTCapsule::Serializer::addField ( const UMLRTObject_field & field, void * data )
{
	Value * value = field.desc->toJson(document, field.desc, data, 0, field.arraySize);
	fields.PushBack(*value, document.GetAllocator());
	//TODO: delete value?
}

const char * UMLRTCapsule::Serializer::write ( int currentState )
{
	document.AddMember("fields", fields, document.GetAllocator());

	if(currentState != -1)
		document.AddMember("currentState", Value().SetInt(currentState), document.GetAllocator());

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	document.Accept(writer);
	return strdup(buffer.GetString());
}

void UMLRTCapsule::Serializer::read ( const char * json, int * currentState )
{
	document.Parse(json);

	if(currentState != NULL)
		*currentState = document["currentState"].GetInt();
}


