#include <Arduino.h>  
#include <Ethernet.h>                                                               
#include <SPI.h>                                                                    
#include <Streaming.h>                                                              
#include <Flash.h>                                                                
#include <MemoryFree.h>                                                           
#include <Agentuino.h>                                                              
#include <DHT.h>                                                                    

#define DHTPIN 2                                                                  
#define DHTTYPE DHT22                                                              
DHT dht(DHTPIN, DHTTYPE);                                                           // Setup object sensor

// Network information
static byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};                         
static byte ip[] = {192, 168, 0, 95};                                         
static byte ip_dns[] = {192, 168, 1, 1};                                          // Dont change
static byte ip_gateway[] = { 192, 168, 0, 1};
static byte subnet[] = {255, 255, 255, 0 };


//Snesor variables
float humidity = 0;
float temperature = 0;
char result[8];


// Device information OID 
const char sysDescr[] PROGMEM      = "1.3.6.1.2.1.1.1.0";                          
const char sysContact[] PROGMEM    = "1.3.6.1.2.1.1.4.0";                           
const char sysName[] PROGMEM       = "1.3.6.1.2.1.1.5.0";                           
const char sysLocation[] PROGMEM   = "1.3.6.1.2.1.1.6.0";                           
const char sysServices[] PROGMEM   = "1.3.6.1.2.1.1.7.0";                           
 
// Sensor OID
const char snmp_temperature[]     PROGMEM     = "1.3.6.1.3.2016.5.1.0";             // Temperature in celsius
const char snmp_humidity[]        PROGMEM     = "1.3.6.1.3.2016.5.1.1";             // percentage humidity
const char snmp_current[]        PROGMEM     = "1.3.6.1.3.2016.5.1.2"; 

// RFC1213 local values
static char locDescr[35]            = "SNMPv1 - System Sensing v1.1";               // read-only (static)
static char locContact[25]          = "Silica Networks SA";                         // should be stored/read from EEPROM - read/write (dont for simplicity)
static char locName[20]             = "NOC";                                        // should be stored/read from EEPROM - read/write (dont for simplicity)
static char locLocation[20]         = "SMA638 - CABA";                              // should be stored/read from EEPROM - read/write (dont for simplicity)
static int32_t locServices          = 2;                                            // read-only (static)
 
uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

// Calculate average of sensor

 
void pduReceived(){
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);
  
  if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET)
    && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS ) {

    pdu.OID.toString(oid);
 
    if ( strcmp_P(oid, sysDescr ) == 0 ) {
      // handle sysDescr (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locDescr
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locDescr);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } 
    } else if ( strcmp_P(oid, sysName ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locName, strlen(locName)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locName
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locName);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysContact ) == 0 ) {
      // handle sysContact (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locContact, strlen(locContact)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locContact
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locContact);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysLocation ) == 0 ) {
      // handle sysLocation (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read/write
        status = pdu.VALUE.decode(locLocation, strlen(locLocation)); 
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      } else {
        // response packet from get-request - locLocation
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, locLocation);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    } else if ( strcmp_P(oid, sysServices) == 0 ) {
      // handle sysServices (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } else {
        // response packet from get-request - locServices
        status = pdu.VALUE.encode(SNMP_SYNTAX_INT, locServices);
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
      //
    }
    else if ( strcmp_P(oid, snmp_temperature ) == 0 ) 
    {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) 
      {      
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } 
      else 
      {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(temperature,6,2,result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } 
        else if ( strcmp_P(oid, snmp_humidity ) == 0 ) 
    {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) 
      {      
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } 
      else 
      {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(humidity,6,2,result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }   
    else {
      // oid does not exist
      // response packet - object not found
      pdu.type = SNMP_PDU_RESPONSE;
      pdu.error = SNMP_ERR_NO_SUCH_NAME;
    }
    Agentuino.responsePdu(&pdu);
  }
 
  Agentuino.freePdu(&pdu);
 
}
 
void setup() {
  dht.begin();
  
  Ethernet.begin(mac, ip, ip_dns, ip_gateway, subnet);    
  api_status = Agentuino.begin();                         // Begin SNMP agent
 
  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }
  delay(10);
}
 
void loop(){ 
  Agentuino.listen();                                      // Listen/Handle for incoming SNMP requests
  
  if(millis()-prevMillis>2000) {  
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    prevMillis = millis();
  }  
}