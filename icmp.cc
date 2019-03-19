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
#include "ip.hh"
#include "ipaddr.hh"
#include "icmp.hh"

ICMPInPacket::ICMPInPacket(byte* theData, udword theLength, InPacket* theFrame) :
	InPacket(theData, theLength, theFrame)
{

}

void ICMPInPacket::decode() {
	ICMPHeader *head = (ICMPHeader*)myData;
	
	//ICMPECHOHeader *echo = (ICMPECHOHeader*)(myData + icmpHeaderLen);
	// Return payload
	if (head->type == 8) {
		// we gots requezt
		head->type = 0;
		head->checksum = head->checksum + 0x8; //magic
		//cout << "Handling ICMP" << endl;
		this->answer((byte*)head, myLength);
	}
	
}

void ICMPInPacket::answer(byte* theData, udword theLength) {
	myFrame->answer(theData, theLength);
}

uword ICMPInPacket::headerOffset() {
	return 8;
}
