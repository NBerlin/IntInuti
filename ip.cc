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
#include "icmp.hh"
#include "tcp.hh"

IP::IP() {
  myIPAddress = new IPAddress::IPAddress(0x82, 0xeb, 0xc8, 0x74); //130.235.200.116
}

IP::IP& IP::instance() {
  static IP::IP ipInstance;
  return ipInstance;
}

const IPAddress& IP::myAddress() {
  return *myIPAddress;
}

IPInPacket::IPInPacket(byte* theData, udword theLength, InPacket* theFrame) :
	InPacket(theData, theLength, theFrame)
{

}

void IPInPacket::decode()
{
	IPHeader *ipHead = (IPHeader*)myData;

	if (ipHead->destinationIPAddress == IP::instance().myAddress()){
		if((((ipHead->versionNHeaderLength & 0xf0) >> 4) == 4) && ((ipHead->versionNHeaderLength & 0x0f) == 5) &&
			((HILO(ipHead->fragmentFlagsNOffset) & 0x3fff) == 0) ) {
			uword totalLength = HILO(ipHead->totalLength);
			myProtocol = ipHead->protocol;
			mySourceIPAddress = ipHead->sourceIPAddress;
			if (myProtocol == 1) {
				// ICMP

				ICMPInPacket icmpPack(((byte*)ipHead) + headerOffset(), totalLength - headerOffset(), this);
				icmpPack.decode();

				// TCP
			}
			else if (myProtocol == 6) {
				TCPInPacket tcpPack(((byte*)ipHead) + headerOffset(), totalLength - headerOffset(), this, mySourceIPAddress); // detta kan va fel
				tcpPack.decode();
			}
			
		}
	}
	// Skicka vidare till ICMP lager

	// Ändra skit

	// skicka tillbaka till LLC
}

void IPInPacket::answer(byte* theData, udword theLength)
{
	// Kolla protocol
	//cout << "Outgoing length is " << theLength << endl;
	//cout << "SRC PORT " << (theData[0]<<8 | theData[1]) << endl;
	//cout << "DEST PORT " << (uword)(theData + 2) << endl;
	byte *buffer = new byte[20 + theLength];
	IPHeader *ipPack = (IPHeader*)buffer;
	memcpy(buffer + 20, theData, theLength);
	ipPack->versionNHeaderLength = (4 << 4) | 5;
	ipPack->TypeOfService = 0;
	ipPack->totalLength = HILO(headerOffset() + theLength);
	ipPack->identification = ipPack->identification + 1;
	ipPack->fragmentFlagsNOffset = 0;
	ipPack->timeToLive = 64;
	ipPack->protocol = myProtocol;
	//cout << "PRUTT " << myProtocol << endl;
	ipPack->headerChecksum = 0;
	ipPack->sourceIPAddress = IP::instance().myAddress();
	ipPack->destinationIPAddress = mySourceIPAddress;
	ipPack->headerChecksum = calculateChecksum(buffer, headerOffset());
	//cout << "Answering ip" << endl;
	myFrame->answer(buffer, headerOffset() + theLength);
	// Ändra headers

	// skicka vidare
	delete[] buffer;
}

uword IPInPacket::headerOffset() {
	//IPHeader *ipHeader = (IPHeader*)myData;
	//return (ipHeader->versionNHeaderLength & 0x0f) * 4; // kan va fel
	return IP::ipHeaderLength;
}

InPacket* IPInPacket::copyAnswerChain() {
	IPInPacket* anAnswerPacket = new IPInPacket(*this);
	anAnswerPacket->setNewFrame(myFrame->copyAnswerChain());
	return anAnswerPacket;
}
