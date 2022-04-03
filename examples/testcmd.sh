#!/bin/sh
# To test these commands, start the usnmpd agent with the command
# "./usnmpd P.38644.30" and the trap receiver with "./usnmptrapd" in another
# shell. Make sure that UDP ports 161 and 162 are not used by another SNMP agent.
./usnmpget -c private $1 B.1.2.0
./usnmpget -c public $1 B.1.1.0 B.1.2.0 B.1.3.0
./usnmpgetnext -c public $1 B.1.1.0 B.1.2.0 B.1.3.0
./usnmpget -c private $1 P.38644.30.1.1.2.2 P.38644.30.1.1.2.3 P.38644.30.3.1.2.0
./usnmpgetnext -c private $1 P.38644.30.1.1.1.2 P.38644.30.1.1.2.2
./usnmpset -c private $1 B.1.6.0 s "someWhere" P.38644.30.2.1.2.6 i 0
./usnmpget -c private $1 B.1.6.0 P.38644.30.2.1.2.6
./usnmpset -c private $1 B.1.6.0 s "placeName" P.38644.30.2.1.2.6 i 1
./usnmptrap -c public -a 192.168.1.170 -d $1 P.38644.30 6 2 P.38644.30.1.1.2.2 i 1
# The following requests should generate exception.
./usnmpget -c something $1 B.1.3.0
./usnmpget -c private $1 B.1.8.0
./usnmpset -c public $1 B.1.6.0 s "placeName"
./usnmpset -c private $1 P.38644.30.1.1.2.2 i 0
