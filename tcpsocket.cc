#include "compiler.h"
#include "threads.hh"
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

TCPSocket::TCPSocket(TCPConnection* connection):
	myConnection(connection),
	myReadSemaphore(Semaphore::createQueueSemaphore("myReadSemaphore",0)),
	myWriteSemaphore(Semaphore::createQueueSemaphore("myWriteSemaphore", 0)),
	eofFound(false)
{
}

TCPSocket::~TCPSocket()
{
	// DESTROY
	delete myReadSemaphore;
	delete myWriteSemaphore;
}

void
TCPSocket::Close()
{
	cout << "Closing socket " << endl;
	myConnection->AppClose(); //kanske?
}

byte*
TCPSocket::Read(udword& theLength)
{
	myReadSemaphore->wait(); // Wait for available data 
	theLength = myReadLength;
	byte* aData = myReadData;
	myReadLength = 0;
	myReadData = 0;
	return aData;
}
void
TCPSocket::socketDataReceived(byte* theData, udword theLength)
{
	myReadData = new byte[theLength];
	memcpy(myReadData, theData, theLength);
	myReadLength = theLength;
	myReadSemaphore->signal(); // Data is available 
}

void
TCPSocket::Write(byte* theData, udword theLength)
{
	myConnection->Send(theData, theLength);
	myWriteSemaphore->wait(); // Wait until the data is acknowledged 
}

void
TCPSocket::socketDataSent()
{
	myWriteSemaphore->signal(); // The data has been acknowledged 
}

void
TCPSocket::socketEof()
{
	eofFound = true;
	myReadLength = 0;
	myReadSemaphore->signal();
}

bool
TCPSocket::isEof()
{
	// true if FIN receive from remote host 
	// TODO!!
	return eofFound;
}

SimpleApplication::SimpleApplication(TCPSocket* theSocket) {
	// Init
	mySocket = theSocket;
}

void
SimpleApplication::doit()
{
	udword aLength;
	byte* aData;
	byte* bigData;
	udword theLength = 10000;
	bool done = false;
	cout << "Simple application running" << endl;
	while (!done && !mySocket->isEof())
	{
		aData = mySocket->Read(aLength);
		//cout << "Data read as " << (char*)aData << endl;
		if (aLength > 0)
		{
			if ((char)*aData == 'q')
			{
				done = true;
			} else if ((char)*aData == 's')
			{
				// Here we send a lot
				bigData = new byte[theLength];
				fillMeUp(bigData, theLength);
				mySocket->Write(bigData, theLength);
				delete bigData;
			}
			mySocket->Write(aData, aLength);
			delete aData;
			
		}
	}
	cout << "Alright, closing SimpleAplication and socket." << endl;
	mySocket->Close();
}

void SimpleApplication::fillMeUp(byte* bigData, udword length) {
	for (udword i = 0; i < length; i++) {
		bigData[i] = 'q';
	}
}
