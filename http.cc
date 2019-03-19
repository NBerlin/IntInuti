/*!***************************************************************************
*!
*! FILE NAME  : http.cc
*!
*! DESCRIPTION: HTTP, Hyper text transfer protocol.
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
}

#include "iostream.hh"
#include "tcpsocket.hh"
#include "http.hh"
#include "fs.hh"

//#define D_HTTP
#ifdef D_HTTP
#define trace cout
#else
#define trace if(false) cout
#endif

enum {
	OK = 200,
	FORBIDDEN = 401,
	NOTFOUND = 404
};
/****************** HTTPServer DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
// Allocates a new null terminated string containing a copy of the data at
// 'thePosition', 'theLength' characters long. The string must be deleted by
// the caller.
//
char*
HTTPServer::extractString(char* thePosition, udword theLength)
{
  char* aString = new char[theLength + 1];
  strncpy(aString, thePosition, theLength);
  aString[theLength] = '\0';
  return aString;
}

//----------------------------------------------------------------------------
//
// Will look for the 'Content-Length' field in the request header and convert
// the length to a udword
// theData is a pointer to the request. theLength is the total length of the
// request.
//
udword
HTTPServer::contentLength(char* theData, udword theLength)
{
  udword index = 0;
  bool   lenFound = false;
  const char* aSearchString = "Content-Length: ";
  while ((index++ < theLength) && !lenFound)
  {
    lenFound = (strncmp(theData + index,
                        aSearchString,
                        strlen(aSearchString)) == 0);
  }
  if (!lenFound)
  {
    return 0;
  }
  trace << "Found Content-Length!" << endl;
  index += strlen(aSearchString) - 1;
  char* lenStart = theData + index;
  char* lenEnd = strchr(theData + index, '\r');
  char* lenString = this->extractString(lenStart, lenEnd - lenStart);
  udword contLen = atoi(lenString);
  trace << "lenString: " << lenString << " is len: " << contLen << endl;
  delete [] lenString;
  return contLen;
}

//----------------------------------------------------------------------------
//
// Decode user and password for basic authentication.
// returns a decoded string that must be deleted by the caller.
//
char*
HTTPServer::decodeBase64(char* theEncodedString)
{
  static const char* someValidCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
  
  int aCharsToDecode;
  int k = 0;
  char  aTmpStorage[4];
  int aValue;
  char* aResult = new char[80];

  // Original code by JH, found on the net years later (!).
  // Modify on your own risk. 
  
  for (unsigned int i = 0; i < strlen(theEncodedString); i += 4)
  {
    aValue = 0;
    aCharsToDecode = 3;
    if (theEncodedString[i+2] == '=')
    {
      aCharsToDecode = 1;
    }
    else if (theEncodedString[i+3] == '=')
    {
      aCharsToDecode = 2;
    }

    for (int j = 0; j <= aCharsToDecode; j++)
    {
      int aDecodedValue;
      aDecodedValue = strchr(someValidCharacters,theEncodedString[i+j])
        - someValidCharacters;
      aDecodedValue <<= ((3-j)*6);
      aValue += aDecodedValue;
    }
    for (int jj = 2; jj >= 0; jj--) 
    {
      aTmpStorage[jj] = aValue & 255;
      aValue >>= 8;
    }
    aResult[k++] = aTmpStorage[0];
    aResult[k++] = aTmpStorage[1];
    aResult[k++] = aTmpStorage[2];
  }
  aResult[k] = 0; // zero terminate string

  return aResult;  
}

//------------------------------------------------------------------------
//
// Decode the URL encoded data submitted in a POST.
//
char*
HTTPServer::decodeForm(char* theEncodedForm)
{
  char* anEncodedFile = strchr(theEncodedForm,'=');
  anEncodedFile++;
  char* aForm = new char[strlen(anEncodedFile) * 2]; 
  // Serious overkill, but what the heck, we've got plenty of memory here!
  udword aSourceIndex = 0;
  udword aDestIndex = 0;
  
  while (aSourceIndex < strlen(anEncodedFile))
  {
    char aChar = *(anEncodedFile + aSourceIndex++);
    switch (aChar)
    {
     case '&':
       *(aForm + aDestIndex++) = '\r';
       *(aForm + aDestIndex++) = '\n';
       break;
     case '+':
       *(aForm + aDestIndex++) = ' ';
       break;
     case '%':
       char aTemp[5];
       aTemp[0] = '0';
       aTemp[1] = 'x';
       aTemp[2] = *(anEncodedFile + aSourceIndex++);
       aTemp[3] = *(anEncodedFile + aSourceIndex++);
       aTemp[4] = '\0';
       udword anUdword;
       anUdword = strtoul((char*)&aTemp,0,0);
       *(aForm + aDestIndex++) = (char)anUdword;
       break;
     default:
       *(aForm + aDestIndex++) = aChar;
       break;
    }
  }
  *(aForm + aDestIndex++) = '\0';
  return aForm;
}

HTTPServer::HTTPServer(TCPSocket* theSocket) : mySocket(theSocket)
{

}

char*
HTTPServer::findPathName(char* str)
{
	char* firstPos = strchr(str, ' ');     // First space on line 
	firstPos++;                            // Pointer to first / 
	char* lastPos = strchr(firstPos, ' '); // Last space on line 
	char* thePath = 0;                     // Result path 
	if ((lastPos - firstPos) == 1)
	{
		// Is / only 
		thePath = 0;                         // Return NULL 
	}
	else
	{
		// Is an absolute path. Skip first /. 
		thePath = extractString((char*)(firstPos + 1),
			lastPos - firstPos);
		if ((lastPos = strrchr(thePath, '/')) != 0)
		{
			// Found a path. Insert -1 as terminator. 
			*lastPos = '\xff';
			*(lastPos + 1) = '\0';
			while ((firstPos = strchr(thePath, '/')) != 0)
			{
				// Insert -1 as separator. 
				*firstPos = '\xff';
			}
		}
		else
		{
			// Is /index.html 
			delete thePath; thePath = 0; // Return NULL 
		}
	}
	return thePath;
}

HTTPContents::HTTPContents() {
	memset(method, '\0', sizeof(method));
	memset(path, '\0', sizeof(path));
	memset(name, '\0', sizeof(name));
}

HTTPContents* HTTPServer::extractContents(byte* data, udword length) {
	HTTPContents* contents = new HTTPContents();
	udword offset;

	// Method
	if (!strncmp((char*)data, "GET", 3)) {
		memcpy(contents->method, data, 3); // GET
		offset = 4;
	}
	else {
		memcpy(contents->method, data, 4); // POST, HEAD
		offset = 5;
	}
	cout << "Got a " << contents->method << " packet" << endl;

	// Path
	udword pathLength = ((udword)strstr((char*)data, "HTTP") - 1) - ((udword)data + offset);
	char *pathName = extractString((char*)(data + offset), pathLength);
	char *nameTemp = strrchr(pathName, '/') + 1;

	char* path = findPathName((char*)data);
	//cout << "Got name and path" << endl;
	cout << "Found pathname for " << contents->method << " packet" << endl;
	if (*nameTemp == '\0') {
		delete pathName;
		pathName = NULL;
		nameTemp = "index.htm";
		strncpy(contents->path, pathName, 50);
		strncpy(contents->name, nameTemp, 40);
	}
	else {
		strncpy(contents->path, path, 50);
		strncpy(contents->name, nameTemp, 40);
		delete pathName;
	}
	cout << "Read contents of " << contents->method << endl;

	return contents;
}

void
HTTPServer::craftHeader(char* header, udword contentLength, uword state, char* fileSuffix) {
	// WE MAKIN A HEADER
	header[0] = '\0';

	switch (state) {
	case OK:
		strcat(header, "HTTP/1.1 200 OK\r\n");
		break;
	case FORBIDDEN:
		strcat(header, "HTTP/1.1 401 Unauthorized\r\n");
		break;
	case NOTFOUND:
		strcat(header, "HTTP/1.1 404 Not Found\r\n");
		break;
	default:
		strcat(header, "HTTP/1.1 1337 WTF\r\n");
		break;
	}

	if (!strncmp(fileSuffix, "htm", 3)) {
		strcat(header, "Content-Type: text/html\r\n");
	}
	else if (!strncmp(fileSuffix, "gif", 3)) {
		strcat(header, "Content-Type: image/gif\r\n");
	}
	else if (!strncmp(fileSuffix, "jpg", 3)) {
		strcat(header, "Content-Type: image/jpg\r\n");
	}

	if (state == FORBIDDEN) {
		strcat(header, "WWW-Authenticate: Basic realm=\"private\"\r\n\r\n");
	}
	strcat(header, "\r\n");
	cout << "Crafted header" << endl;
}

void
HTTPServer::doit()
{
	HTTPContents* contents;
	udword aLength;
	byte* aData;
	//char* answer;
	//udword answerLen;
	bool done = false;
	cout << "Hindenburg has taken off" << endl;
	while (!done && !mySocket->isEof())
	{
		aData = mySocket->Read(aLength);
		if (aLength > 0)
		{
			//contents = extractContents(aData, aLength);

			//cout << "Got a request" << endl;
			//cout << "METHOD: " << contents->method << endl;
			//cout << "PATH: " << contents->path << "LEN: " << strlen(contents->path) << endl;
			//cout << "NAME: " << contents->name << "LEN: " << strlen(contents->name) << endl;
			if (!strncmp((char*)aData, "GET", 3)) {
				contents = extractContents(aData, aLength);
				// A GET request
				char answer[200];
				udword fileLength = 0;
				udword headerLength;
				byte* file = FileSystem::instance().readFile(contents->path, contents->name, fileLength);

				if (file == 0) {
					// 404'd
					
					craftHeader(answer, 0, NOTFOUND, "htm");
					headerLength = strlen(answer);
					mySocket->Write((byte*)answer, headerLength);
				}
				else if (!strncmp(contents->path,"private", 7)) {
					char* pass = strstr((char*)aData, "Authorization: Basic ");
					if (pass == NULL) {
						craftHeader(answer, 0, FORBIDDEN, "htm");
						char* gitout = "<html><head><title>401 Unauthorized</title></head><CRLF><body><h1>401 Unauthorized</h1></body></html>";
						mySocket->Write((byte*)answer, strlen(answer));
						mySocket->Write((byte*)answer, strlen(gitout));
					}
					else {
						pass = pass + 21;
						char* last = strstr(pass, "\r\n");
						char* myPass = extractString(pass, (udword)(last - pass));
						char* decodedPass = decodeBase64(myPass);
						cout << "Decode pass is " << decodedPass << endl;
						if (!strncmp(decodedPass, "admin:admin", 11)) {
							craftHeader(answer, 0, OK, "htm");
							mySocket->Write((byte*)answer, strlen(answer));
							mySocket->Write((byte*)file, fileLength);
						}
						else {
							craftHeader(answer, 0, FORBIDDEN, "htm");
							char* gitout = "<html><head><title>401 Unauthorized</title></head><CRLF><body><h1>401 Unauthorized</h1></body></html>";
							mySocket->Write((byte*)answer, strlen(answer));
							mySocket->Write((byte*)answer, strlen(gitout));
						}
					}
					
				}
				else {
					//fileLength = strlen((char*)file);
					cout << "File found!" << endl;
					cout << "Core " << ax_coreleft_total() << endl;
					craftHeader(answer, 0, OK, strchr(contents->name, '.') + 1);
					headerLength = strlen(answer);
					cout << "File found! Length is " << fileLength << ", header length is " << headerLength << endl;
					mySocket->Write((byte*)answer, headerLength);
					cout << "Sent header" << endl;
					mySocket->Write((byte*)file, fileLength);
					cout << "Sent body" << endl;
					//memcpy(answer + strlen(answer), file, fileLength);
				}
				// What does it want?

				/*char* dest = strchr((char*)aData, '/');
				uword destLength = (strstr((char*)aData, "HTTP") - dest) - 1; //maybe works
				char path[destLength];
				memcpy(path, dest, destLength);
				char* tempName = strrchr(path, '/');*/

				//udword fileLength;
				//byte* file = readFile(contents->path, contents->name, fileLength);
				// git files from filesystem

				// Maybe format a response?
				delete contents;
				
			}
			else if (!strncmp((char*)aData, "POST", 4)) {
				cout << "Got a POST!" << endl;
				char answer[200];

				byte* recvBuffer = new byte[20000];
				udword recvd = 0;
				udword pointer = 0;

				udword cLength = contentLength((char*)aData, aLength);

				byte* recvContent = (byte*)decodeForm(strstr((char*)aData, "\r\n\r\n")+4);
				recvd = strlen((char*)recvContent);
				cout << "Content length is " << recvd << endl;
				memcpy(recvBuffer, recvContent, recvd);
				pointer = recvd;
				delete recvContent;

				while (recvd < cLength) {
					aData = mySocket->Read(aLength);
					cLength = contentLength((char*)aData, aLength);
					recvContent = (byte*)decodeForm(strstr((char*)aData, "\r\n\r\n") + 4);
					recvd = recvd + strlen((char*)recvContent);
					memcpy(recvBuffer + pointer, recvContent, strlen((char*)recvContent));
					pointer = recvd;
					delete recvContent;
				}
				contents = extractContents(recvBuffer, recvd);
				cout << "Recevied POST content of length " << recvd << endl;
				//recvBuffer[recvd + 1] = '\0';
				if (FileSystem::instance().writeFile(contents->path, contents->name, recvBuffer, recvd)) {
					craftHeader(answer, 0, OK, strchr(contents->name, '.') + 1);
					char* success = "<html><head><title>Accepted</title></head><body><h1>The file dynamic.htm was updated successfully.</h1></body></html>";
					mySocket->Write((byte*)answer, strlen(answer));
					mySocket->Write((byte*)success, strlen(success));
				}

				delete recvBuffer;
				delete contents;
			}
			else if (!strncmp((char*)aData, "HEAD", 4)) {
			}
			else {
				// Shiet request
				cout << "Got bad request. Request was " << extractString((char*)aData, 4) << endl;
			}

			// What type is it?



			/*
			if ((char)*aData == 'q')
			{
				done = true;
			}
			else if ((char)*aData == 's')
			{
				// Here we send a lot
				bigData = new byte[theLength];
				fillMeUp(bigData, theLength);
				mySocket->Write(bigData, theLength);
				delete bigData;
			}
			mySocket->Write(aData, aLength);
			delete aData;

			*/
			
			done = true;
		}
	}
	cout << "Hindenburg crashed gently." << endl;
	mySocket->Close();
}

/************** END OF FILE http.cc *************************************/
