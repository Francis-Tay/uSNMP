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

#if defined( ESP32 )
char sysDescr[] = "ESP32";
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
char dInIndex[] = "P.38644.30.1.1.2.10";
unsigned char c, lastDIN;

#define GPIO16 16
#define GPIO17 17
#define GPIO18 18
#define GPIO19 19
#define GPIO21 21
#define GPIO22 22
#define GPIO23 23
#define GPIO33 33
#define GPIO34 34
#define GPIO35 35

void setup()
{
	pinMode(GPIO16, INPUT_PULLUP); pinMode(GPIO17, INPUT_PULLUP);
	pinMode(GPIO18, INPUT_PULLUP); pinMode(GPIO19, INPUT_PULLUP);
	pinMode(GPIO21, OUTPUT); pinMode(GPIO22, OUTPUT); pinMode(GPIO23, OUTPUT);               

	initSnmpAgent(SNMP_PORT, ENTERPRISE_OID, RO_COMMUNITY, RW_COMMUNITY);
	initMibTree();
	trapBuild(&request, enterpriseOID, hostIpAddr, COLD_START, 0, NULL); // cold start trap
	trapSend(&request, trapDstAddr, TRAP_DST_PORT, roCommunity);

	digitalWrite(GPIO21, LOW); digitalWrite(GPIO22, LOW); digitalWrite(GPIO23, LOW);
	c=digitalRead(GPIO16); lastDIN = c; // read and store digital input in a byte
	c=digitalRead(GPIO17); lastDIN |= c<<1;
	c=digitalRead(GPIO18); lastDIN |= c<<2;
	c=digitalRead(GPIO19); lastDIN |= c<<3;
}

void loop()
{
	unsigned char x, y;

	c=digitalRead(GPIO16); y = c;
	c=digitalRead(GPIO17); y |= c<<1;
	c=digitalRead(GPIO18); y |= c<<2;
	c=digitalRead(GPIO19); y |= c<<3;

	x = (lastDIN ^ y); // has any input pin changed state?
	if (x) {
		for ( j=6; j<=9; j++ ) {
			if ( x & 0x01 ) {
				vblistReset(&response); dInIndex[18]='0'+j; // use response buffer to build trap
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
		lastDIN = y;
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

	/* GPIO16-19 are designated for digital inputs */

	// Digital input #16 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.16", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO16; mibsetvalue(thismib, &i, 0);
	// The value of Digital #16
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.16", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #17 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.17", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO17; mibsetvalue(thismib, &i, 0);
	// The value of Digital #17
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.17", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #18 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.18", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO18; mibsetvalue(thismib, &i, 0);
	// The value of Digital #18
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.18", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #19 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.19", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO19; mibsetvalue(thismib, &i, 0);
	// The value of Digital #19
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.19", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	/* GPIO21-23 are designated for digital outputs. */

	// Digital output #21 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.21", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO21; mibsetvalue(thismib, &i, 0);
	// The value of Digital #21
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.21", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #22 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.22", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO22; mibsetvalue(thismib, &i, 0);
	// The value of Digital #22
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.22", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #23 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.23", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO23; mibsetvalue(thismib, &i, 0);
	// The value of Digital #23
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.23", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	/* GPIO33-35 are designated for analog inputs. */

	// Analog input #33 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.33", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO33; mibsetvalue(thismib, &i, 0);
	// The value of Analog #25
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.33", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);

	// Analog input #34 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.34", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO34; mibsetvalue(thismib, &i, 0);
	// The value of Analog #26
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.34", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);

	// Analog input #35 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.35", INTEGER, RD_ONLY, NULL, 0);
	i = GPIO35; mibsetvalue(thismib, &i, 0);
	// The value of Analog #27
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.35", GAUGE, RD_ONLY, NULL, 0);
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
	j = (uint32_t) digitalRead(c);
	thismib->u.intval = j;
	return SUCCESS;
}

int set_dio(MIB *thismib, void *ptr, int len)
{
	c = thismib->oid.array[thismib->oid.len-1];
	j = *(uint32_t *)ptr;
	if ( j!=0 && j!=1 )
		return ILLEGAL_DATA;
	else
		digitalWrite(c, j);
	thismib->u.intval = j;
	return SUCCESS;
}

int get_ain(MIB *thismib)
{
	c = thismib->oid.array[thismib->oid.len-1];
	thismib->u.intval = analogRead(c);
	return SUCCESS;
}
