#include <SnmpAgent.h>

// Agent's IP configuration. Retain these global variable names.
IPAddress hostIpAddr( 192, 168, 1, 177 ),
	dnsServer( 192, 168, 1, 1 ),
	hostGateway( 192, 168, 1, 1 ),
	hostNetmask( 255, 255, 255, 0 );
char staSSID[] = "Wifi_SSID";
char staPSK[] = "Wifi_Password";

// SNMP agent configuration.
#define ENTERPRISE_OID  "P.38644.30"  // used as sysObjectID and in trap
#define RO_COMMUNITY    "public"				  
#define RW_COMMUNITY    "private"
#define TRAP_DST_ADDR   "192.168.1.170"

void initMibTree();
char trapDstAddr[] = TRAP_DST_ADDR;

#if defined( ESP8266 )
char sysDescr[] = "ESP8266";
#else
char sysDescr[] = "Arduino";
#endif
unsigned char entOIDBer[MIB_DATA_SIZE];
char sysContact[] = "sysAdmin";
char sysName[] = "hostName";
char sysLocation[] = "placeName";

int get_uptime(MIB *);
int get_dio(MIB *);
int set_dio(MIB *, void *, int);
int get_ain(MIB *thismib);

uint32_t i, j;
char dInIndex[] = "P.38644.30.1.1.2.0";
unsigned char c, lastDIN;

void setup()
{
	pinMode(D2, INPUT_PULLUP); pinMode(D3, INPUT_PULLUP);
	pinMode(D4, INPUT_PULLUP); pinMode(D5, INPUT_PULLUP);
	pinMode(D6, OUTPUT); pinMode(D7, OUTPUT); pinMode(D8, OUTPUT);

	initSnmpAgent(SNMP_PORT, ENTERPRISE_OID, RO_COMMUNITY, RW_COMMUNITY);
	initMibTree();
	trapBuild(&request, enterpriseOID, hostIpAddr, COLD_START, 0, NULL); // cold start trap
	trapSend(&request, trapDstAddr, TRAP_DST_PORT, roCommunity);

	digitalWrite(D6, LOW); digitalWrite(D7, LOW); digitalWrite(D8, LOW);
	c=digitalRead(D2); lastDIN = c; // read and store digital input in a byte
	c=digitalRead(D3); lastDIN |= c<<1;
	c=digitalRead(D4); lastDIN |= c<<2;
	c=digitalRead(D5); lastDIN |= c<<3;
}

void loop()
{
	unsigned char x, y;

	c=digitalRead(D2); y = c;
	c=digitalRead(D3); y |= c<<1;
	c=digitalRead(D4); y |= c<<2;
	c=digitalRead(D5); y |= c<<3;

	x = (lastDIN ^ y); // has any input pin changed state?
	if (x) {
		for ( j=D2; j<=D5; j++ ) {
			if ( x & 0x01 ) {
				vblistReset(&response); dInIndex[17]='0'+j; // use response buffer to build trap
				if ( lastDIN & 0x01 ) { // input pin y was 1
					i = 0;          // it is thus 0 now
					vblistAdd(&response, dInIndex, INTEGER, &i, 0);
					trapBuild(&request, enterpriseOID, hostIpAddr, ENTERPRISE_SPECIFIC, 1, &response);
				}
				else {
					i = 1;
					vblistAdd(&response, dInIndex, INTEGER, &i, 0);
					trapBuild(&request, enterpriseOID, hostIpAddr, ENTERPRISE_SPECIFIC, 2, &response);
				}
				trapSend(&request, trapDstAddr, TRAP_DST_PORT, rwCommunity);
			}
			x>>=1; lastDIN>>=1;
		}
		lastDIN=y;
	}
	if ( processSNMP() == COMM_STR_MISMATCH ) {
		trapBuild(&request, enterpriseOID, hostIpAddr, AUTHENTICATE_FAIL, 0, NULL); // authentication fail trap
		trapSend(&request, trapDstAddr, TRAP_DST_PORT, rwCommunity);
	}
	delay(100);
}

void initMibTree()
{
	MIB *thismib;

	/* System MIB */

	// sysDescr Entry
	thismib = miblistadd(mibTree, "B.1.1.0", OCTET_STRING, RD_ONLY,
		sysDescr, strlen(sysDescr));

	// sysObjectID Entry
	thismib = miblistadd(mibTree, "B.1.2.0", OBJECT_IDENTIFIER, RD_ONLY,
		entOIDBer, 0);  // set length to 0 first
	i = str2ber(enterpriseOID, entOIDBer);
	mibsetvalue(thismib, (void *) entOIDBer, (int) i);  // proper length set

	// sysUptime Entry
	thismib = miblistadd(mibTree, "B.1.3.0", TIMETICKS, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_uptime, NULL);

	// sysContact Entry
	thismib = miblistadd(mibTree, "B.1.4.0", OCTET_STRING, RD_WR,
		sysContact, strlen(sysContact));

	// sysName Entry
	thismib = miblistadd(mibTree, "B.1.5.0", OCTET_STRING, RD_WR,
		sysName, strlen(sysName));

	// sysLocation Entry
	thismib = miblistadd(mibTree, "B.1.6.0", OCTET_STRING, RD_WR,
		sysLocation, strlen(sysLocation));

	// sysServices Entry
	thismib = miblistadd(mibTree, "B.1.7.0", INTEGER, RD_ONLY, NULL, 0);
	i = 5; mibsetvalue(thismib, &i, 0);

	/* Digital D2-D5 is designated for input */

	// Digital input #2 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.2", INTEGER, RD_ONLY, NULL, 0);
	i = 2; mibsetvalue(thismib, &i, 0);
	// The value of Digital #2
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.2", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #3 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.3", INTEGER, RD_ONLY, NULL, 0);
	i = 3; mibsetvalue(thismib, &i, 0);
	// The value of Digital #3
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.3", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #4 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.4", INTEGER, RD_ONLY, NULL, 0);
	i = 4; mibsetvalue(thismib, &i, 0);
	// The value of Digital #4
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.4", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #5 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.5", INTEGER, RD_ONLY, NULL, 0);
	i = 5; mibsetvalue(thismib, &i, 0);
	// The value of Digital #5
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.5", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	/* Digital D6-D8 is designated for output */

	// Digital output #6 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.6", INTEGER, RD_ONLY, NULL, 0);
	i = 6; mibsetvalue(thismib, &i, 0);
	// The value of Digital #6
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.6", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #7 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.7", INTEGER, RD_ONLY, NULL, 0);
	i = 7; mibsetvalue(thismib, &i, 0);
	// The value of Digital #7
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.7", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #8 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.8", INTEGER, RD_ONLY, NULL, 0);
	i = 8; mibsetvalue(thismib, &i, 0);
	// The value of Digital #8
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.8", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	/* Analog A0 input */

	// Analog input #0 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.0", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	// The value of Analog #0
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.0", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);
}

int get_uptime(MIB *thismib)
{
	thismib->u.intval = sysUpTime();
	return SUCCESS;
}

int get_dio(MIB *thismib)
{
	c = thismib->oid.array[thismib->oid.len-1];
	switch (c) {
	case 2:
		j = (uint32_t) digitalRead(D2);
		break;
	case 3:
		j = (uint32_t) digitalRead(D3);
		break;
	case 4:
		j = (uint32_t) digitalRead(D4);
		break;
	case 5:
		j = (uint32_t) digitalRead(D5);
		break;
	case 6:
		j = (uint32_t) digitalRead(D6);
		break;
	case 7:
		j = (uint32_t) digitalRead(D7);
		break;
	case 8:
		j = (uint32_t) digitalRead(D8);
		break;
	default:
		return FAIL;
	}
	thismib->u.intval = j;
	return SUCCESS;
}

int set_dio(MIB *thismib, void *ptr, int len)
{
	c = thismib->oid.array[thismib->oid.len-1];
	j = *(uint32_t *)ptr;
	if ( j!=0 && j!=1 ) return ILLEGAL_DATA;
	switch (c) {
	case 6:
		digitalWrite(D6, j);
		break;
	case 7:
		digitalWrite(D7, j);
		break;
	case 8:
		digitalWrite(D8, j);
		break;
	default:
		return FAIL;
	}
	thismib->u.intval = j;
	return SUCCESS;
}

int get_ain(MIB *thismib)
{
	c = thismib->oid.array[thismib->oid.len-1];
	if ( c==0 ) {
		thismib->u.intval = analogRead(A0);
		return SUCCESS;
	}
	else
		return FAIL;
}
