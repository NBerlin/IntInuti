#ifndef http_hh
#define http_hh

#include "job.hh"
#include "tcpsocket.hh"

class HTTPServer;
class HTTPContents {
public:
	HTTPContents();

	char method[4];
	char path[50];
	char name[40];
};

class HTTPServer : public Job {
public: 
	HTTPServer(TCPSocket* theSocket);

	void doit();

private:
	HTTPContents* extractContents(byte* data, udword length);

	char* findPathName(char* str);

	char* decodeForm(char* theEncodedForm);

	char* decodeBase64(char* theEncodedString);

	udword contentLength(char* theData, udword theLength);

	char* extractString(char* thePosition, udword theLength);

	void craftHeader(char* header, udword contentLength, uword state, char* fileSuffix);

	TCPSocket* mySocket;

};

#endif
