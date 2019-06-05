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
#include "basefatal.hh"

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
	fieldObjMap = new UMLRTHashMap("fieldObjMap", UMLRTHashMap::compareString, false/*objectIsString*/);
	fieldDataMap = new UMLRTHashMap("fieldDataMap", UMLRTHashMap::compareString, false/*objectIsString*/);
}

UMLRTCapsule::Serializer::~Serializer ( )
{
	//TODO: free maps?
}

void UMLRTCapsule::Serializer::registerField ( const UMLRTObject_field * field, void * data )
{
	fieldObjMap->insert(field->name, (void*)field);
	fieldDataMap->insert(field->name, data);
}

const char * UMLRTCapsule::Serializer::write ( int currentState )
{
    Document document;
    document.SetObject();

    Value fields;
	fields.SetArray();

	fieldObjMap->lock();
    UMLRTHashMap::Iterator iter = fieldObjMap->getIterator();

    while (iter != iter.end())
    {
    		const UMLRTObject_field * field = (const UMLRTObject_field *)iter.getObject();
    		void * data = fieldDataMap->getObject(field->name);

    		Value name;
    		name.SetString(StringRef(field->name));

    		Value * value = field->desc->toJson(document, field->desc, data, 0, field->arraySize);
    		value->AddMember("name", name, document.GetAllocator());
    		fields.PushBack(*value, document.GetAllocator());

        iter = iter.next();
    }
    fieldObjMap->unlock();

    if(currentState != -1)
    		document.AddMember("currentState", Value().SetInt(currentState), document.GetAllocator());
    	document.AddMember("fields", fields, document.GetAllocator());

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	document.Accept(writer);
	return strdup(buffer.GetString());
}

void UMLRTCapsule::Serializer::read ( const char * json, int * currentState )
{
	Document document;
	document.Parse(json);

	if(currentState != NULL)
		*currentState = document["currentState"].GetInt();


	const Value& fields = document["fields"];
	for (Value::ConstValueIterator itr = fields.Begin(); itr != fields.End(); ++itr) {
		const char * fieldName = (*itr)["name"].GetString();

		const UMLRTObject_field * field = (const UMLRTObject_field *)fieldObjMap->getObject(fieldName);
		if(field == NULL)
			FATAL("UMLRTObject_field for field %s not found", fieldName);

		void * data = fieldDataMap->getObject(field->name);
		if(data == NULL)
			FATAL("data region for field %s not found", fieldName);

		field->desc->fromJson((Value&)(*itr)["value"], field->desc, data, 0);
	}
}


