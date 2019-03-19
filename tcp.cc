/*!***************************************************************************
*!
*! FILE NAME  : tcp.cc
*!
*! DESCRIPTION: TCP, Transport control protocol
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "tcpsocket.hh"
#include "http.hh"

#define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** TCP DEFINITION SECTION *************************/
enum {
	FIN = 0x01,
	SYN = 0x02,
	PST = 0x04,
	PSH = 0x08,
	ACK = 0x10,
	URG = 0x20,
	ECE = 0x40,
	CWR = 0x80
};
//----------------------------------------------------------------------------
//
TCP::TCP()
{
  trace << "TCP created." << endl;
}

//----------------------------------------------------------------------------
//
TCP&
TCP::instance()
{
  static TCP myInstance;
  return myInstance;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::getConnection(IPAddress& theSourceAddress,
                   uword      theSourcePort,
                   uword      theDestinationPort)
{
  TCPConnection* aConnection = NULL;
  // Find among open connections
  uword queueLength = myConnectionList.Length();
  myConnectionList.ResetIterator();
  bool connectionFound = false;
  while ((queueLength-- > 0) && !connectionFound)
  {
    aConnection = myConnectionList.Next();
    connectionFound = aConnection->tryConnection(theSourceAddress,
                                                 theSourcePort,
                                                 theDestinationPort);
  }
  if (!connectionFound)
  {
    trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    trace << "Found connection in queue" << endl;
  }
  return aConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::createConnection(IPAddress& theSourceAddress,
                      uword      theSourcePort,
                      uword      theDestinationPort,
                      InPacket*  theCreator)
{
  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);
  myConnectionList.Append(aConnection);
  return aConnection;
}

//----------------------------------------------------------------------------
//
void
TCP::deleteConnection(TCPConnection* theConnection)
{
  myConnectionList.Remove(theConnection);
  delete theConnection;
}

void
TCP::connectionEstablished(TCPConnection *theConnection)
{
	TCPSocket* aSocket = new TCPSocket(theConnection);
	// Create a new TCPSocket. 
	theConnection->registerSocket(aSocket);
	if (theConnection->myPort == 7) // SimpleApplication port
	{
		// Register the socket in the TCPConnection. 
		Job::schedule(new SimpleApplication(aSocket)); // SCHEDULE HTTPSERVER
		// Create and start an application for the connection. 
	}
	else if (theConnection->myPort == 80) /* Hindenburg (HTTP server) port */{
		Job::schedule(new HTTPServer(aSocket));
	}
	else {
		// Unused port?
	}
}

bool
TCP::acceptConnection(uword portNo) {
	return (portNo == 7 || portNo == 80);
}

//----------------------------------------------------------------------------
//
TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort)
{
  trace << "TCP connection created" << endl;
  myRetransmitTimer = new RetransmitTimer(this, Clock::seconds*2);
  myWaitTimer = new WaitTimer(this, Clock::seconds*3);
  myTCPSender = new TCPSender(this, theCreator),
  myState = ListenState::instance();
  sentMaxSeq = 0;
  queueLength = 0;
  firstSeq = 0;
}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  trace << "TCP connection destroyed" << endl;
  delete myTCPSender;
  delete myRetransmitTimer;
}

//----------------------------------------------------------------------------
//
bool
TCPConnection::tryConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort)
{
  return (theSourcePort      == hisPort   ) &&
         (theDestinationPort == myPort    ) &&
         (theSourceAddress   == hisAddress);
}
void TCPConnection::Receive(udword theSynchronizationNumber, byte* theData, udword theLength) {
	myState->Receive(this, theSynchronizationNumber, theData, theLength);
}

void TCPConnection::Send(byte* theData, udword theLength) {
	myState->Send(this, theData, theLength);
}

void TCPConnection::Synchronize(udword theSynchronizationNumber){
	myState->Synchronize(this, theSynchronizationNumber);
}
// Handle an incoming SYN segment
void TCPConnection::NetClose(){
	myState->NetClose(this);
}
// Handle an incoming FIN segment
void TCPConnection::AppClose(){
	myState->AppClose(this);
}
void TCPConnection::Kill(){
	myState->Kill(this);
}
void TCPConnection::Acknowledge(udword theAcknowledgementNumber) {
	myState->Acknowledge(this, theAcknowledgementNumber);
}

uword 
TCPConnection::serverPortNumber() {
	return myPort;
}

void 
TCPConnection::registerSocket(TCPSocket* theSocket) {
	mySocket = theSocket;
}

// TCPConnection cont...




//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.


//----------------------------------------------------------------------------
//
void
TCPState::Kill(TCPConnection* theConnection)
{
  trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}

void TCPState::Synchronize(TCPConnection* theConnection,
	udword theSynchronizationNumber){
	//cout << "UNIMPL SYNC" << endl;
}
// Handle an incoming SYN segment
void TCPState::NetClose(TCPConnection* theConnection){
	//cout << "UNIMPL NC" << endl;
}
// Handle an incoming FIN segment
void TCPState::AppClose(TCPConnection* theConnection){
	//cout << "UNIMPL AC" << endl;
}
// Handle close from application

// Handle an incoming RST segment, can also called in other error conditions
void TCPState::Receive(TCPConnection* theConnection,
	udword theSynchronizationNumber,
	byte*  theData,
	udword theLength){
	//cout << "UNIMPL RC" << endl;
}
// Handle incoming data
void TCPState::Acknowledge(TCPConnection* theConnection,
	udword theAcknowledgementNumber){
	//cout << "UNIMPL ACK" << endl;
}
// Handle incoming Acknowledgement
void TCPState::Send(TCPConnection* theConnection,
	byte*  theData,
	udword theLength){
	//cout << "UNIMPL SEND" << endl;
}




//----------------------------------------------------------------------------
//
//ListenState::ListenState(){}


ListenState*
ListenState::instance()
{
  static ListenState myInstance;
  return &myInstance;
}

void ListenState::Synchronize(TCPConnection* theConnection,
	udword theSynchronizationNumber){
	switch (theConnection->myPort)
	{
	case 80:
	case 7:
		//trace << "got SYN on ECHO port" << endl;
		theConnection->receiveNext = theSynchronizationNumber + 1;
		theConnection->receiveWindow = 8 * 1024;
		theConnection->sendNext = get_time();
		// Next reply to be sent. 
		theConnection->sentUnAcked = theConnection->sendNext;
		// Send a segment with the SYN and ACK flags set. 
		theConnection->myTCPSender->sendFlags(SYN|ACK);
		// Prepare for the next send operation. 
		theConnection->sendNext += 1;
		// Change state 
		theConnection->myState = SynRecvdState::instance();
		break;
	default:
		//trace << "send RST..." << endl;
		theConnection->sendNext = 0;
		// Send a segment with the RST flag set. 
		theConnection->myTCPSender->sendFlags(0x04);
		TCP::instance().deleteConnection(theConnection);
		break;
	}
}




//----------------------------------------------------------------------------
//
SynRecvdState*
SynRecvdState::instance()
{
  static SynRecvdState myInstance;
  return &myInstance;
}


void SynRecvdState::Acknowledge(TCPConnection* theConnection,
	udword theAcknowledgementNumber){
	//cout << "SYNRECVD ACK" << endl;
	// We in bois
	switch (theConnection->myPort) {
	case 80:
	case 7:
		if (theAcknowledgementNumber == theConnection->sendNext /*&& check seq num*/) {
			theConnection->sentUnAcked = theConnection->sendNext; // yeboi
			TCP::instance().connectionEstablished(theConnection);
			theConnection->myState = EstablishedState::instance();
			cout << "Established connection to " << theConnection->hisAddress << endl;
			
		}
		break;
	default:
		// Kuken

		break;
	}
}




//----------------------------------------------------------------------------
//
EstablishedState*
EstablishedState::instance()
{
  static EstablishedState myInstance;
  return &myInstance;
}


//----------------------------------------------------------------------------
//
void
EstablishedState::NetClose(TCPConnection* theConnection)
{
  //trace << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK
  theConnection->receiveNext += 1;
  theConnection->myTCPSender->sendFlags(ACK);
  //cout << "Sent termination flags" << endl;
  theConnection->mySocket->socketEof();
  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();
  
  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...
  //theConnection->AppClose();
}

//----------------------------------------------------------------------------
//
void
EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  //trace << "EstablishedState::Receive" << endl;

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
  theConnection->receiveNext += theLength;
  theConnection->myTCPSender->sendFlags(ACK);
  //theConnection->Send(theData, theLength);
  if (!strncmp((char*)theData, "POST", 4)) {
	  cout << "Received POST" << endl;
  }
  // Applikationen får datan
  theConnection->mySocket->socketDataReceived(theData, theLength);

}


void EstablishedState::Acknowledge(TCPConnection* theConnection,
	udword theAcknowledgementNumber)
{
	theConnection->sentUnAcked = theAcknowledgementNumber - 1;
	if ((theConnection->firstSeq + theConnection->queueLength) == theAcknowledgementNumber) {
		theConnection->firstSeq = 0;
		theConnection->queueLength = 0;
		theConnection->myRetransmitTimer->cancel();
		theConnection->mySocket->socketDataSent();
	}
	else if (theConnection->firstSeq > 0) {
		theConnection->myTCPSender->sendFromQueue();
	}
	//theConnection->sendNext = theAcknowledgementNumber;
}
// Handle incoming Acknowledgement
void EstablishedState::Send(TCPConnection* theConnection,
	byte*  theData,
	udword theLength){
	theConnection->transmitQueue = theData;
	theConnection->queueLength = theLength;
	theConnection->firstSeq = theConnection->sendNext;

	theConnection->myTCPSender->sendFromQueue();
}

void EstablishedState::AppClose(TCPConnection* theConnection) 
{
	theConnection->myState = FinWait1State::instance();
	theConnection->myTCPSender->sendFlags(FIN|ACK);
}





//----------------------------------------------------------------------------
//
CloseWaitState*
CloseWaitState::instance()
{
  static CloseWaitState myInstance;
  return &myInstance;
}


void CloseWaitState::AppClose(TCPConnection* theConnection){
	theConnection->myRetransmitTimer->cancel();
	theConnection->myState = LastAckState::instance();
	//cout << "Sending termination" << endl;
	theConnection->myTCPSender->sendFlags(FIN | ACK);
}







//----------------------------------------------------------------------------
//
LastAckState*
LastAckState::instance()
{
  static LastAckState myInstance;
  return &myInstance;
}

void LastAckState::Acknowledge(TCPConnection* theConnection,
	udword theAcknowledgementNumber){
	// Shits dead yo
	theConnection->Kill();
}

FinWait1State* FinWait1State::instance()
{
	static FinWait1State myInstance;
	return &myInstance;
}

void FinWait1State::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber)
{
	//cout << "FinWait1State::Ack" << endl;
	if (theAcknowledgementNumber == theConnection->sendNext + 1) {
		// stäng av retransmitt
		theConnection->myRetransmitTimer->cancel();
		theConnection->sentUnAcked = theAcknowledgementNumber - 1;
		theConnection->sendNext = theAcknowledgementNumber;
		theConnection->myState = FinWait2State::instance();
		//cout << "Switch to FW2" << endl;
	}
	else {
		theConnection->sentUnAcked = theAcknowledgementNumber - 1;
		theConnection->sendNext = theAcknowledgementNumber;
	}
	
}

//----------------------------------------------------------------------------
//

FinWait2State* FinWait2State::instance()
{
	static FinWait2State myInstance;
	return &myInstance;
}

void FinWait2State::NetClose(TCPConnection* theConnection)
{
	//cout << "FINWAIT2 NETCLOSE" << endl;
	theConnection->receiveNext += 1;
	theConnection->myTCPSender->sendFlags(ACK);
	theConnection->myWaitTimer->start();
}

//----------------------------------------------------------------------------
//
TCPSender::TCPSender(TCPConnection* theConnection, 
                     InPacket*      theCreator):
        myConnection(theConnection),
        myAnswerChain(theCreator->copyAnswerChain()) // Copies InPacket chain!
{
}

//----------------------------------------------------------------------------
//
TCPSender::~TCPSender()
{
  myAnswerChain->deleteAnswerChain();
}

void TCPSender::sendFromQueue() {
	myConnection->theOffset = myConnection->sendNext - myConnection->firstSeq;
	myConnection->theFirst = myConnection->transmitQueue + myConnection->theOffset;
	udword amountLeft = myConnection->queueLength - myConnection->theOffset;
	udword bytesToSend = MIN(amountLeft, 1460);
	udword windowSizeLeft = myConnection->myWindowSize - (myConnection->sendNext + myConnection->sentUnAcked);

	// Calculate how long to send
	udword sLength = MIN(bytesToSend, windowSizeLeft);
	if (sLength > 0) {

		sendData(myConnection->theFirst, sLength);
		myConnection->myRetransmitTimer->start();
		if (myConnection->sendNext >= myConnection->sentMaxSeq) {
			myConnection->sentMaxSeq = myConnection->sendNext;
		}
		else {
			myConnection->sendNext = myConnection->sentMaxSeq;
		}
		// Recursive call
		sendFromQueue();
	}

}

void TCPSender::sendFlags(byte theFlags){
	
	// Decide on the value of the length totalSegmentLength.
	udword totalSegmentLength = TCP::tcpHeaderLength;
	// Allocate a TCP segment. 
	byte* anAnswer = new byte[totalSegmentLength];
	// Calculate the pseudo header checksum 
	TCPPseudoHeader* aPseudoHeader =
		new TCPPseudoHeader(myConnection->hisAddress,
			totalSegmentLength);
	uword pseudosum = aPseudoHeader->checksum();
	delete aPseudoHeader;
	// Create the TCP segment. 
	TCPHeader *aTCPHeader = (TCPHeader*)anAnswer;
	// Set the goodest values
	aTCPHeader->sourcePort = HILO(myConnection->myPort);
	
	aTCPHeader->destinationPort = HILO(myConnection->hisPort);
	aTCPHeader->sequenceNumber = LHILO(myConnection->sendNext);
	aTCPHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
	aTCPHeader->headerLength = 0x50;
	aTCPHeader->flags = theFlags;
	aTCPHeader->windowSize = HILO(myConnection->receiveWindow);
	aTCPHeader->checksum = 0;
	aTCPHeader->urgentPointer = 0;
	// Calculate the final checksum. 
	aTCPHeader->checksum = calculateChecksum(anAnswer,
		totalSegmentLength,
		pseudosum);
	myAnswerChain->answer(anAnswer,
		totalSegmentLength);
	// Deallocate the dynamic memory 
	delete anAnswer;
}

void TCPSender::sendData(byte* theData, udword theLength) {
	//cout << "Sending data" << endl;
	// Decide on the value of the length totalSegmentLength.
	static udword hutt = 1;
	udword totalSegmentLength = TCP::tcpHeaderLength + theLength;
	// Allocate a TCP segment. 
	byte* anAnswer = new byte[totalSegmentLength];
	memcpy(anAnswer + TCP::tcpHeaderLength, theData, theLength);
	// Calculate the pseudo header checksum 
	TCPPseudoHeader* aPseudoHeader =
		new TCPPseudoHeader(myConnection->hisAddress,
			totalSegmentLength);
	uword pseudosum = aPseudoHeader->checksum();
	delete aPseudoHeader;
	// Create the TCP segment. 
	TCPHeader *aTCPHeader = (TCPHeader*)anAnswer;
	// Set the goodest values
	aTCPHeader->sourcePort = HILO(myConnection->myPort);

	aTCPHeader->destinationPort = HILO(myConnection->hisPort);
	aTCPHeader->sequenceNumber = LHILO(myConnection->sendNext);
	aTCPHeader->acknowledgementNumber = LHILO(myConnection->receiveNext);
	aTCPHeader->headerLength = 0x50;
	aTCPHeader->flags = PSH|ACK;
	aTCPHeader->windowSize = HILO(myConnection->receiveWindow);
	aTCPHeader->checksum = 0;
	aTCPHeader->urgentPointer = 0;
	// Calculate the final checksum. 
	aTCPHeader->checksum = calculateChecksum(anAnswer,
		totalSegmentLength,
		pseudosum);
	if (hutt % 10 != 0) {
		myAnswerChain->answer(anAnswer,
			totalSegmentLength);
	}
	else {
		hutt = 0;
	}
	myConnection->sendNext += theLength;
	hutt++;
	// Deallocate the dynamic memory 
	delete anAnswer;
}







//----------------------------------------------------------------------------
//
TCPInPacket::TCPInPacket(byte*           theData,
                         udword          theLength,
                         InPacket*       theFrame,
                         IPAddress&      theSourceAddress):
        InPacket(theData, theLength, theFrame),
        mySourceAddress(theSourceAddress)
{
	
}

void TCPInPacket::decode() {
	// Få tag i vilken connection den tillhör
	TCPHeader *header = (TCPHeader*)myData;
	//TCPState *state;
	mySourcePort = HILO(header->sourcePort);
	myDestinationPort = HILO(header->destinationPort);
	mySequenceNumber = LHILO(header->sequenceNumber);
	myAcknowledgementNumber = LHILO(header->acknowledgementNumber);
	TCPConnection *connection = TCP::instance().getConnection(mySourceAddress, mySourcePort, myDestinationPort);
	if (connection == NULL) {
		// Behöver göra en ny connection
		connection = TCP::instance().createConnection(mySourceAddress, mySourcePort, myDestinationPort, this);
		connection->myWindowSize = HILO(header->windowSize);
		//cout << "Skapade en connection" << endl;
		if ((header->flags & SYN) != 0) {
			// State LISTEN. Received SYN flag
			if (TCP::instance().acceptConnection(connection->myPort)) {
				connection->Synchronize(mySequenceNumber);
			}
			//cout << "SYN fixat" << endl;
		}
		else {
			// Oopsie
			connection->Kill();
		}
	}
	else {
		connection->myWindowSize = HILO(header->windowSize);
		// Nuvarande state
		// Hantera ALLA states :(
		if ((header->flags & ACK) != 0) {
			connection->Acknowledge(myAcknowledgementNumber);
			if ((( myLength - headerOffset()) > 0) && (mySourcePort == 80)) {
				connection->Receive(mySequenceNumber, myData + headerOffset(), myLength - headerOffset());
			}
		}
		if ((header->flags & FIN) != 0) {
			//prolly
			connection->NetClose();
		}
		if((header->flags & PSH) != 0){
			//cout << "Should send plis" << endl;
			connection->Receive(mySequenceNumber, myData + headerOffset(), myLength - headerOffset());
		}
		

	}

}

void TCPInPacket::answer(byte* theData, udword theLength) {
	cout << "Kallas answer på??" << endl;
}

uword TCPInPacket::headerOffset(){
	return TCP::tcpHeaderLength;
}



//----------------------------------------------------------------------------
//
InPacket*
TCPInPacket::copyAnswerChain()
{  
  return myFrame->copyAnswerChain();
}

//----------------------------------------------------------------------------
//
TCPPseudoHeader::TCPPseudoHeader(IPAddress& theDestination,
                                 uword theLength):
        sourceIPAddress(IP::instance().myAddress()),
        destinationIPAddress(theDestination),
        zero(0),
        protocol(6)
{
  tcpLength = HILO(theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPPseudoHeader::checksum()
{
  return calculateChecksum((byte*)this, 12);
}

RetransmitTimer::RetransmitTimer(TCPConnection* theConnection,
	Duration retransmitTime) : myConnection(theConnection), myRetransmitTime(retransmitTime){}

void RetransmitTimer::start() {
	this->timeOutAfter(myRetransmitTime);
}

void RetransmitTimer::cancel() {
	this->resetTimeOut();
}
void RetransmitTimer::timeOut() {
	// ...->sendNext = ...->sentUnAcked; ..->sendFromQueue(); 
	myConnection->sendNext = myConnection->sentUnAcked;
	myConnection->myTCPSender->sendFromQueue();
}

WaitTimer::WaitTimer(TCPConnection* theConnection,
	Duration waitTime) : myConnection(theConnection), myWaitTime(waitTime){}

void WaitTimer::start() {
	this->timeOutAfter(myWaitTime);
}

void WaitTimer::timeOut() {
	myConnection->Kill();
}

/****************** END OF FILE tcp.cc *************************************/

