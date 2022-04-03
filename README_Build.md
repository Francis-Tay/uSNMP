**uSNMP ("micro-SNMP")** is a small, portable open-source 'C' library for developing SNMPv1 agent and manager. For more details, see [README.md](README.md)

#### Compiling in Windows and Unix/Linux/Cygwin

Two `make` files, for Embarcadero BCC32C C++ compiler (for Windows) and GCC (for Unix), are included.

To illustrate how uSNMP may be used, some example programs are provided:

1. An SNMPv1 agent, *usnmpd.c*, to simulate an Arduino, with Enterprise OID "1.3.6.1.4.1.38644.30". The state of the digital pins and the values of the analog pins are read from a text file named *usnmpd.dat*. For a real agent on an Arduino, see the next section on **Installing the uSNMP agent-only library in Arduino**

2. Command-line utilities (*usnmpget.c, usnmpgetnext.c, usnmpset.c, usnmptrap.c*) to send SNMPv1 **GET, GetNext, SET** request and **TRAP** respectively.

3. A command-line utility (*usnmptrapd.c*) to receive and display a SNMPv1 **TRAP** packet.

4. A set of test scripts named *testcmd.sh, testagent.sh* (for \*nix) and *testcmd.bat, testagent.sh* (for Windows). The *testcmd* scripts uses the commands built with uSNMP whereas *testagent* uses Net-SNMP commands (available from www.net-snmp.org)

These commands support options including a debug feature to display the packet content. Use the -h option to get a list of available options and valid arguments.

To compile and test in **Windows** with **BCC32C**
1. `cd <installed uSNMP directory>`
2. `cd src`
   `make -f Makefile.bcc`
   `cd ..\examples`
   `make -f Makefile.bcc`
3. Start the agent in a new command box to see its output
     `usnmpd -d P.38644.30`
   Check that UDP port 161 is not used by another SNMP agent
4. Start the trap receiver in another command box
     `usnmptrapd -d`
   Check that UDP port 162 is not used by another SNMP agent
5. Start the test script with
     `testcmd.bat 127.0.0.1`
     
To compile and test in **Unix/Linux/Cygwin** with **GCC**
1. `cd <installed uSNMP directory>`
2. `cd src`
   `make -f Makefile.gcc`
   `cd ../examples`
   `make -f Makefile.gcc`
3. Start the usnmpd agent in a new shell to see its output
     `./usnmpd -d P.38644.30`
   Check that UDP port 161 is not used by another SNMP agent.
4. Start the trap receiver in another shell
     `./usnmptrapd -d`
   Check that UDP port 162 is not used by another SNMP agent.
5. Start the test with
     `./testcmd.sh 127.0.0.1`

#### Installing the uSNMP agent-only library in Arduino

For 3rd party hardware packages such as NodeMCU, it is first necessary to add the URL of their Boards Manager JSON file in the Arduino IDE. The URLs point to JSON index files that Arduino IDE uses to build the list of available installed boards.
 
1. Run `2Arduino.sh` (for \*nix) or `2Arduino.bat` (for Windows) in `src` to create a directory named `Arduino/SnmpAgent`, into which are copied these files required to build an agent:
		mib.c
		mib.h
		list.c
		list.h
		miblist.c
		miblist.h
		endian.c
		endian.h
		misc.c
		misc.h
		oid.c
		oid.h
		varbind.c
		varbind.h
		retval.h
		snmpdefs.h
		usnmp.h
		SnmpAgent.h
		SnmpAgent.c (renamed as SnmpAgent.cpp)
		examples/usnmp/usnmpd.ino
2. Copy this *SnmpAgent* directory to the *libraries* directory of the Arduino project directory.
3. In `usnmp.h`, modify the buffer size as appropriate to the amount of SRAM provided by your target processor board.
4. Launch the Arduino IDE, select the board, and load the example *usnmpd.ino* from the SnmpAgent library.
5. Modify the network and agent parameters in *usnmpd.ino* as required:
   - Whether Ethenet or WiFi is used
   - WiFi SSID and password
   - IP configuration of the agent
   - Agent's Enterprise OID, community strings and trap destination
6. Connect the board, select the assigned COM port, and download the code to the board.

The *usnmpd.ino* example works on AVR ATmega328P, ATmega2560 and ESP8266 and may be adapted to other boards by modifying the buffer size definitions in *usnmp.h* in the *SnmpAgent* library directory. More digital I/O and analog input pins can be added with more MIB entries in *usnmpd.ino*, depending on the amount of SRAM provided by your target processor.


