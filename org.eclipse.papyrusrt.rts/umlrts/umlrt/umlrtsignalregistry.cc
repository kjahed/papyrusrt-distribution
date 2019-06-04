// umlrtsignalregistry.cc

#include "basedebug.hh"
#include "basefatal.hh"
#include "umlrtsignalregistry.hh"
#include <string>

UMLRTSignalRegistry::~UMLRTSignalRegistry ( ) 
{
	getOutSignalMap()->lock();

	UMLRTHashMap::Iterator iter1 = getOutSignalMap()->getIterator();
	while (iter1 != iter1.end())
    {
    	UMLRTHashMap* signalsMap = (UMLRTHashMap*) iter1.getObject();

    	signalsMap->lock();

    	UMLRTHashMap::Iterator iter2 = signalsMap->getIterator();
		while (iter2 != iter2.end())
	    {
	    	SignalEntry* entry = (SignalEntry*) iter2.getObject();
	    	delete entry;

	    	iter2 = iter2.next();
	    }


	    signalsMap->unlock();

    	iter1 = iter1.next();
    }

    getOutSignalMap()->unlock();

	getInSignalMap()->lock();

	iter1 = getInSignalMap()->getIterator();
	while (iter1 != iter1.end())
    {
    	UMLRTHashMap* signalsMap = (UMLRTHashMap*) iter1.getObject();

    	signalsMap->lock();

    	UMLRTHashMap::Iterator iter2 = signalsMap->getIterator();
		while (iter2 != iter2.end())
	    {
	    	SignalEntry* entry = (SignalEntry*) iter2.getObject();
	    	delete entry;

	    	iter2 = iter2.next();
	    }


	    signalsMap->unlock();

    	iter1 = iter1.next();
    }

    getInSignalMap()->unlock();
}

UMLRTHashMap * UMLRTSignalRegistry::getOutSignalMap()
{
    if (outSignalMap == NULL)
    {
        outSignalMap = new UMLRTHashMap("outSignalMap", UMLRTHashMap::compareStringIgnoreCase, false/*objectIsString*/);
    }

    return outSignalMap;
}

UMLRTHashMap * UMLRTSignalRegistry::getInSignalMap()
{
    if (inSignalMap == NULL)
    {
        inSignalMap = new UMLRTHashMap("inSignalMap", UMLRTHashMap::compareStringIgnoreCase, false/*objectIsString*/);
    }

    return inSignalMap;
}

bool UMLRTSignalRegistry::registerOutSignal ( const char* protocol, const char* signalName, int signalID, UMLRTObject* payloadObj ) 
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getOutSignalMap()->getObject(protocol);
	if(signalsMap == NULL) {
		signalsMap = new UMLRTHashMap(protocol, UMLRTHashMap::compareStringIgnoreCase, false/*objectIsString*/);
		getOutSignalMap()->insert(protocol, signalsMap);
	}

	if(signalsMap->getObject(signalName) != NULL)
		return false; // already registered

	SignalEntry* entry = new SignalEntry;
	entry->protocol = protocol;
	entry->name = signalName;
	entry->id = signalID;
	entry->payloadObj = payloadObj;

	signalsMap->insert(signalName, entry);

	return true;
}

bool UMLRTSignalRegistry::registerInSignal ( const char* protocol, const char* signalName, int signalID, UMLRTObject* payloadObj ) 
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getInSignalMap()->getObject(protocol);
	if(signalsMap == NULL) {
		signalsMap = new UMLRTHashMap(protocol, UMLRTHashMap::compareStringIgnoreCase, false/*objectIsString*/);
		getInSignalMap()->insert(protocol, signalsMap);
	}

	if(signalsMap->getObject(signalName) != NULL)
		return false; // already registered

	SignalEntry* entry = new SignalEntry;
	entry->protocol = protocol;
	entry->name = signalName;
	entry->id = signalID;
	entry->payloadObj = payloadObj;

	signalsMap->insert(signalName, entry);

	return true;
}
    
int UMLRTSignalRegistry::getOutSignalID ( const char* protocol, const char* signalName )
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getOutSignalMap()->getObject(protocol);
	if(signalsMap != NULL) {
		SignalEntry* entry = (SignalEntry*) signalsMap->getObject(signalName);
		if(entry != NULL)
			return entry->id;
	}

	return -1;
}


int UMLRTSignalRegistry::getInSignalID ( const char* protocol, const char* signalName )
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getInSignalMap()->getObject(protocol);
	if(signalsMap != NULL) {
		SignalEntry* entry = (SignalEntry*) signalsMap->getObject(signalName);
		if(entry != NULL)
			return entry->id;
	}

	return -1;
}

UMLRTObject* UMLRTSignalRegistry::getOutSignalPayloadObject ( const char* protocol, const char* signalName )
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getOutSignalMap()->getObject(protocol);
	if(signalsMap != NULL) {
		SignalEntry* entry = (SignalEntry*) signalsMap->getObject(signalName);
		if(entry != NULL)
			return entry->payloadObj;
	}

	return NULL;
}

UMLRTObject* UMLRTSignalRegistry::getInSignalPayloadObject ( const char* protocol, const char* signalName )
{
	UMLRTHashMap* signalsMap = (UMLRTHashMap*) getInSignalMap()->getObject(protocol);
	if(signalsMap != NULL) {
		SignalEntry* entry = (SignalEntry*) signalsMap->getObject(signalName);
		if(entry != NULL)
			return entry->payloadObj;
	}

	return NULL;
}

bool UMLRTSignalRegistry::fromJSON( const char * json, const UMLRTCommsPort * port, UMLRTSignal & signal )
{
	Document document;
	document.Parse(json);

	if(!document.IsObject()) {
		fprintf(stderr, "Error parsing JSON message: '%s'\n", json);
		return false;
	}

	if(!document.HasMember("signal")) {
		fprintf(stderr, "Error parsing JSON message: '%s'\n", json);
		return false;
	}

	const Value& sigVal = document["signal"];
	if(!sigVal.IsString()) {
		fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
		return false;
	}

	const char* signalName = sigVal.GetString();
	const char* protocolName = port->role()->protocol;

	int signalID = getInSignalID(protocolName, signalName);
	if(signalID == -1)
		return false;

	UMLRTObject * payloadObj = getInSignalPayloadObject(protocolName, signalName);
	if(payloadObj->sizeOf == 0)
		payloadObj = NULL;

	if(payloadObj != NULL && !document.HasMember("params")) {
		fprintf(stderr, "Mismatched signal parameters in JSON message '%s'\n", json);
		return false;
	}

	if(payloadObj == NULL && document.HasMember("params")) {
		const Value& paramsArr = document["params"];
		if(!paramsArr.IsArray()) {
			fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
			return false;
		}

		if(paramsArr.Size() > 0) {
			fprintf(stderr, "Mismatched signal parameters in JSON message '%s'\n", json);
			return false;
		}
	}

	if(payloadObj != NULL) {
		const Value& paramsArr = document["params"];
		if(!paramsArr.IsArray()) {
			fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
			return false;
		}

		if(payloadObj->numFields != paramsArr.Size()) {
			fprintf(stderr, "Mismatched signal parameters in JSON message '%s'\n", json);
			return false;
		}

		uint8_t* data = (uint8_t*) malloc(sizeof(uint8_t) * payloadObj->sizeOf);
		if(data == NULL)
			FATAL("memory allocation failed for payload data");

		for (size_t i=0; i<payloadObj->numFields; i++)
	    {
			const Value& paramObj = paramsArr[i];
			if(!paramObj.IsObject()) {
				fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
				return false;
			}

			if(!paramObj.HasMember("value")) {

				fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
				return false;
			}

			if(paramObj.HasMember("type")
				&& !paramObj["type"].IsString()) {

				fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
				return false;
			}

			if(payloadObj->fields[i].desc->fromJson((Value&)paramObj["value"],
					payloadObj->fields[i].desc, data + payloadObj->fields[i].offset, 0) == NULL) {
				fprintf(stderr, "Erroneous JSON message: '%s'\n", json);
								return false;
			}
	    }

		signal.initialize(signalName, signalID, port, payloadObj, data);

		free(data);
	}

	else {

		signal.initialize(signalName, signalID, port);

	}

	return true;
}

std::string UMLRTSignalRegistry::toJSON( const UMLRTSignal & signal ) {
	Document document;
	document.SetObject();
	Document::AllocatorType& allocator = document.GetAllocator();

	Value paramsArray(kArrayType);

	const UMLRTObject * payloadObj = getOutSignalPayloadObject(
		signal.getSrcPort()->role()->protocol, signal.getName());

	if(payloadObj == NULL)
		FATAL("cannot to encode signal with invalid payload object");

	for(size_t i=0; i<payloadObj->numFields; i++) {

		Value paramName;
		paramName.SetString(payloadObj->fields[i].name, allocator);
		Value * paramObj = payloadObj->fields[i].desc->toJson(document, payloadObj->fields[i].desc, signal.getParam(i), 0, payloadObj->fields[i].arraySize);
		paramObj->AddMember("name", paramName, allocator);
		paramsArray.PushBack(*paramObj, allocator);
	}

	Value signalName;
	signalName.SetString(signal.getName(), allocator);

	document.AddMember("signal", signalName, allocator);
	document.AddMember("params", paramsArray, allocator);

	StringBuffer strbuf;
	Writer<StringBuffer> writer(strbuf);
	document.Accept(writer);
	return strbuf.GetString();
}
