REM ...using the Net-SNMP binaries.
REM Start the usnmpd agent with the command "usnmpd P.38644.30" in another command box.
REM Make sure that UDP port 161 is not used by another SNMP agent.
snmpget -v 1 -c private %1 1.3.6.1.2.1.1.2.0
snmpget -v 1 -c public %1 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.2.0 1.3.6.1.2.1.1.3.0
snmpgetnext -v 1 -c public %1 1.3.6.1.2.1.1.1.0 1.3.6.1.2.1.1.2.0 1.3.6.1.2.1.1.3.0
snmpget -v 1 -c private %1 1.3.6.1.4.1.38644.30.1.1.2.2 1.3.6.1.4.1.38644.30.1.1.2.3 1.3.6.1.4.1.38644.30.3.1.2.0
snmpgetnext -v 1 -c private %1 1.3.6.1.4.1.38644.30.1.1.1.2 1.3.6.1.4.1.38644.30.1.1.2.2
snmpset -v 1 -c private %1 1.3.6.1.2.1.1.6.0 s "someWhere" 1.3.6.1.4.1.38644.30.2.1.2.6 i 0
snmpget -v 1 -c private %1 1.3.6.1.2.1.1.6.0 1.3.6.1.4.1.38644.30.2.1.2.6
snmpset -v 1 -c private %1 1.3.6.1.2.1.1.6.0 s "placeName" 1.3.6.1.4.1.38644.30.2.1.2.6 i 1
snmpwalk -r 0 -v 1 -c private %1 1.3.6.1.2.1
snmpwalk -r 0 -v 1 -c private %1 1.3.6.1.4.1.38644.30.1
snmpwalk -r 0 -v 1 -c private %1 1.3.6.1.4.1.38644.30.2
snmpwalk -r 0 -v 1 -c private %1 1.3.6.1.4.1.38644.30.3
# The following requests should generate exception
snmpget -v 1 -c something %1 1.3.6.1.2.1.1.3.0
snmpget -v 1 -c private %1 1.3.6.1.2.1.1.8.0
snmpset -v 1 -c public %1 1.3.6.1.2.1.1.6.0 s "placeName"
snmpset -v 1 -c private %1 1.3.6.1.4.1.38644.30.1.1.2.2 i 0
