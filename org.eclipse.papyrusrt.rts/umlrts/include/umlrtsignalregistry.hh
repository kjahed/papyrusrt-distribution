// umlrtsignalregistry.hh

#ifndef UMLRTSIGNALREGISTRY_HH
#define UMLRTSIGNALREGISTRY_HH

#include "umlrthashmap.hh"
#include "umlrtobjectclass.hh"
#include "umlrtsignal.hh"
#include "umlrtcommsport.hh"


#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

class UMLRTHashMap;

class UMLRTSignalRegistry
{
public:
    bool registerOutSignal ( const char* protocol, const char* signalName, int signalID, UMLRTObject* payloadObj );
    bool registerInSignal ( const char* protocol, const char* signalName, int signalID, UMLRTObject* payloadObj );

    int getOutSignalID ( const char* protocol, const char* signalName );
	int getInSignalID ( const char* protocol, const char* signalName );

	UMLRTObject * getOutSignalPayloadObject ( const char* protocol, const char* signal );
	UMLRTObject * getInSignalPayloadObject ( const char* protocol, const char* signal );

	bool fromJSON( const char * json, const UMLRTCommsPort * port, UMLRTSignal & signal );
	std::string toJSON( const UMLRTSignal & signal );

	static UMLRTSignalRegistry& getRegistry ( )
    {
        static UMLRTSignalRegistry instance;
        return instance;
    }

private:
	UMLRTSignalRegistry ( ) { };
	UMLRTSignalRegistry(UMLRTSignalRegistry const&);
	void operator=(UMLRTSignalRegistry const&);
    ~UMLRTSignalRegistry ( );

	struct SignalEntry {
		const char* protocol;
		const char* name;
		UMLRTObject* payloadObj;
		int id;
	};

    UMLRTHashMap * getOutSignalMap ( );
    UMLRTHashMap * getInSignalMap ( );
    
    bool decodeParam ( const Value& param, const UMLRTObject_class * desc, uint8_t * buffer );
    void encodeParam ( Document & document, Value & paramObj, const UMLRTObject_class * desc, void * data );

    UMLRTHashMap * outSignalMap;
    UMLRTHashMap * inSignalMap;
};

#endif // UMLRTSIGNALREGISTRY_HH
