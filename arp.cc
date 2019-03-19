#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "ethernet.hh"
#include "llc.hh"
#include "arp.hh"
#include "ip.hh"

ARPInPacket::ARPInPacket(byte* theData, udword theLength, InPacket* theFrame) : InPacket(theData, theLength, theFrame)
{
	//cout<<"Vi är i ARPinPacket"<< endl;
}

void ARPInPacket::decode()
{
	// Hit kommer den
	ARPHeader* aHeader = (ARPHeader*)myData;
	//cout << "Target: " << aHeader->targetIPAddress << " Unit: " << IP::instance().myAddress() << endl;
	if (aHeader->targetIPAddress == IP::instance().myAddress()) {
		ARPHeader newAHeader;
		newAHeader.hardType = aHeader->hardType;
		newAHeader.protType = aHeader->protType;
		newAHeader.hardSize = (byte)6; //aHeader->hardSize;
		newAHeader.protSize = (byte)4; //aHeader->protSize;
		newAHeader.op = HILO(2);
		newAHeader.targetEthAddress = aHeader->senderEthAddress;
		newAHeader.targetIPAddress = aHeader->senderIPAddress;
		newAHeader.senderEthAddress = Ethernet::instance().myAddress();
		newAHeader.senderIPAddress = IP::instance().myAddress();
			this->answer((byte*)&newAHeader, myLength);
	} else {

	}
}

void ARPInPacket::answer(byte* theData, udword theLength)
{
	//cout << "we in bois" << endl;
	/*ARPHeader* aHeader = (ARPHeader*)myData;
	ARPHeader newAHeader;
	newAHeader.hardType = aHeader->hardType;
	newAHeader.protType = aHeader->protType;
	newAHeader.hardSize = aHeader->hardSize;
	newAHeader.protSize = aHeader->protSize;
	newAHeader.op = HILO(2);
	newAHeader.targetEthAddress = aHeader->senderEthAddress;
	newAHeader.targetIPAddress = aHeader->senderIPAddress;
	newAHeader.senderEthAddress = Ethernet::instance().myAddress();
	newAHeader.senderIPAddress = IP::instance().myAddress();*/

	/*cout << "SENDING RESPONSE" << endl;
	cout << "TARGET ETH: " << newAHeader.targetEthAddress << endl;
	cout << "TARGET IP: " << newAHeader.targetIPAddress << endl;
	cout << "SOURCE ETH: " << newAHeader.senderEthAddress << endl;
	cout << "SOURCE IP: " << newAHeader.senderIPAddress << endl << endl;*/

	//myFrame->answer((byte*)&m, sizeof(newAHeader));
	myFrame->answer(theData, theLength);
}

uword ARPInPacket::headerOffset() {
	return myFrame->headerOffset() + 28;
}
