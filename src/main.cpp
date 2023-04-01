#include <Arduino.h>  
#include <Ethernet.h>                                                               
#include <SPI.h>                                                                    
#include <Streaming.h>                                                              
#include <Flash.h>                                                                
#include <MemoryFree.h>                                                           
#include <Agentuino.h>                                                              
#include <DHT.h>                                                                    


#define DHTPIN 5                                                                  
#define DHTTYPE DHT22                                                              
DHT dht(DHTPIN, DHTTYPE);                                                           


// Network information
static byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};                         
static byte ip[] = {192, 168, 0, 95};                                         
static byte ip_dns[] = {192, 168, 1, 1};                                          
static byte ip_gateway[] = { 192, 168, 0, 1};
static byte subnet[] = {255, 255, 255, 0 };

// Sensor variables
float humidity = 0;
float temperature = 0;
int current = 0; 

const int voltage_delta = 2475; 
const int relation_AmperVolt = 12;

char result[8];

// Device info OID 
const char sysDescr[] PROGMEM      = "1.3.6.1.2.1.1.1.0";                          
const char sysContact[] PROGMEM    = "1.3.6.1.2.1.1.4.0";                           
const char sysName[] PROGMEM       = "1.3.6.1.2.1.1.5.0";                           
const char sysLocation[] PROGMEM   = "1.3.6.1.2.1.1.6.0";                           
const char sysServices[] PROGMEM   = "1.3.6.1.2.1.1.7.0";                           
 
// Sensor OID
const char snmp_temperature[]     PROGMEM     = "1.3.6.1.3.2016.5.1.0";             
const char snmp_humidity[]        PROGMEM     = "1.3.6.1.3.2016.5.1.1";             
const char snmp_current[]        PROGMEM     = "1.3.6.1.3.2016.5.1.2"; 

// RFC1213 local values
static char locDescr[35]            = "System Sensing v1.0";              
static char locContact[25]          = "Silica Networks SA";                         
static char locName[20]             = "A NOC product";                                        
static char locLocation[20]         = "SMA 638 - CABA";                             
static int32_t locServices          = 2;                                            
 
uint32_t prevMillis = millis();
char oid[SNMP_MAX_OID_LEN];
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;


// Return current value
int deltaCurrent() {
  float sum = 0;
  float current_temp = 0;
  float voltage = 0;

  for (int i = 0; i < 3; i++) {           
    voltage = (analogRead(0) * 5000.0) / 1023; 
    current = (voltage - voltage_delta) / relation_AmperVolt;  
    sum = current + sum;      
    delay(200);        
  }
  current_temp = sum / 3;
  return current_temp;
}
 


void pduReceived() {
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
    } else if ( strcmp_P(oid, snmp_temperature ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {      
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } 
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(temperature,6,2,result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, snmp_humidity ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {      
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } 
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(humidity,6,2,result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    } else if ( strcmp_P(oid, snmp_current ) == 0 ) {
      // handle sysName (set/get) requests
      if ( pdu.type == SNMP_PDU_SET ) {      
        // response packet from set-request - object is read-only
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_READ_ONLY;
      } 
      else {
        status = pdu.VALUE.encode(SNMP_SYNTAX_OCTETS, dtostrf(current,6,2,result));
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = status;
      }
    }   
    else {
      // oid don't exist
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
  api_status = Agentuino.begin();                        
 
  if ( api_status == SNMP_API_STAT_SUCCESS ) {
    Agentuino.onPduReceive(pduReceived);
    delay(10);
    return;
  }
  delay(10);
}
 
void loop() { 
  Agentuino.listen();                                      

  if (millis() - prevMillis > 2000) {
    temperature = dht.readTemperature();
    current = deltaCurrent();
    humidity = dht.readHumidity();
    prevMillis = millis();
  }  
}