#!/bin/sh
mkdir ../Arduino
mkdir ../Arduino/SnmpAgent
mkdir ../Arduino/SnmpAgent/examples
mkdir ../Arduino/SnmpAgent/examples/usnmpd_atmega
mkdir ../Arduino/SnmpAgent/examples/usnmpd_esp32
mkdir ../Arduino/SnmpAgent/examples/usnmpd_esp8266
cp mib.c ../Arduino/SnmpAgent
cp mib.h ../Arduino/SnmpAgent
cp list.c ../Arduino/SnmpAgent
cp list.h ../Arduino/SnmpAgent
cp miblist.c ../Arduino/SnmpAgent
cp miblist.h ../Arduino/SnmpAgent
cp endian.c ../Arduino/SnmpAgent
cp endian.h ../Arduino/SnmpAgent
cp misc.c ../Arduino/SnmpAgent
cp misc.h ../Arduino/SnmpAgent
cp oid.c ../Arduino/SnmpAgent
cp oid.h ../Arduino/SnmpAgent
cp varbind.c ../Arduino/SnmpAgent
cp varbind.h ../Arduino/SnmpAgent
cp retval.h ../Arduino/SnmpAgent
cp snmpdefs.h ../Arduino/SnmpAgent
cp usnmp.h ../Arduino/SnmpAgent
cp SnmpAgent.h ../Arduino/SnmpAgent
cp SnmpAgent.c ../Arduino/SnmpAgent/SnmpAgent.cpp
cp ../examples/usnmpd_atmega.ino ../Arduino/SnmpAgent/examples/usnmpd_atmega
cp ../examples/usnmpd_esp32.ino ../Arduino/SnmpAgent/examples/usnmpd_esp32
cp ../examples/usnmpd_esp8266.ino ../Arduino/SnmpAgent/examples/usnmpd_esp8266
