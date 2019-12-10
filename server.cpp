#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <Windows.h>
#include <iostream>
#include <io.h>
#include <process.h>
#include <direct.h>
#include <thread>
#include <conio.h>
using namespace std;

//enable classic C function
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)
#pragma comment(lib, "WS2_32.lib")

#define NOT_FOUND 404
#define BAD_REQUESTION 400
#define OTG 114514

sockaddr_in cAddr = { 0 };
int len = sizeof cAddr;
SOCKET clientSocket;

int scount = 0;
int echoFlag = 1;

char fileBuff[1024];
char filePath[200];

//check if contains certain sub str
int containsSubstr(char* str1, const char* str2) {
	int size1 = strlen(str1);
	int size2 = strlen(str2);
	if (size1 < size2)
		return 0;
	int flag;
	for (int i = 0; i <= size1 - size2; i++) {
		flag = 1;
		for (int k = 0; k < size2; k++) {
			if (str1[i + k] != str2[k]) {
				flag = 0;
				break;
			}
		}
		if (flag == 1)	return 1;
	}
	return 0;
}
//replace symbol
char* replaceSubstr(char*str) {
	char path[1024];
	memset(path, 0, 1024);
	int i = 0;
	int j = 0;
	char ch = 0;
	while (str[i] != '\0') {
		ch = str[i];
		if (ch == '/') {
			//path[j++] = 92;
			path[j] = 92;
		}
		else {
			path[j] = ch;
		}
		i++; j++;
	}
	path[j] = '\0';
	return path;
}
//erase / synbol in url
char* eraseSubstr(char*str) {
	char tmp[200];
	int i = 1;
	while (str[i] != '\0') {
		tmp[i - 1] = str[i];
		i++;
	}
	tmp[i - 1] = 0;
	return tmp;
}
//read html file
void readHtml(char*filename) {
	FILE *fp=fopen(filename, "r");
	memset(fileBuff, 0, 1024);
	fgets(fileBuff, 1024, fp);
	fclose(fp);
	//printf("%s\r\n", buff);
}
//comm to clients
void commBrowser(SOCKET clientsocket) {
	scount++;
	char *method = { NULL };				//client request method
	char *url = { NULL };						//client request url
	char *version = { NULL };				//request protocol version
	char tmp[1024];
	//int fileSize;					//size of file
	char fileName[200];		//general menu of file to open

	char buff[1024];
	char sendBuff[1024];
	int code = 0;

	int r;
	while (1) {
		r = recv(clientSocket, buff, 1023, NULL);
		if (r > 0) {
			buff[r] = 0;
			printf("client %d dgram:\n%s\n", scount, buff);

			//analyze datagram to get requested html
			char *p = buff;
			for (int i = 0; i < 3; i++) {
				if (i == 0) {
					method = buff;	//save method
					while ((*p != ' ') && (*p != '\r'))
						p++;
					p[0] = 0;			//find space or \r to split method
					p++;
					printf("request method: %s\n", method);
				}
				if (i == 1) {
					url = p;				//save url
					while ((*p != ' ') && (*p != '\r'))
						p++;
					p[0] = 0;			//find space or \r to split url
					p++;
					printf("request url: %s\n", url);
				}
				if (i == 2) {
					version = p;		//save protocol version
					while (*p != '\r')
						p++;
					p[0] = '\0';			//split version and put an end to first line
					p++;
					printf("request protocol version: %s\n", version);
					p[0] = '\0';
					p++;
				}
			}
			printf("\n");

			//error handling
			if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0)
				code = BAD_REQUESTION;
			if (strcmp(version, "HTTP/1.1") != 0)
				code = BAD_REQUESTION;
			if (!url || url[0] != '/')
				code = BAD_REQUESTION;

			//send JPEG to browser
			if (containsSubstr(url, ".jpg")) {
				//sendPicture(url);
				memset(fileName, 0, 200);
				strcpy(fileName, filePath);
				strcat(fileName,eraseSubstr(url));
				FILE *fp = fopen(fileName, "r");
				fseek(fp, 0, SEEK_END);
				int img_size = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				char* imgbuf = (char*)malloc(sizeof(char)*(img_size + 8));
				fread(imgbuf, 1, img_size, fp);
				char* buf = (char*)malloc(sizeof(char)*(img_size + 1 + 128));
				int length = sprintf(buf, "HTTP/1.1 200 OK\r\n"
					"Content-Type: image/jpeg\r\n"
					"Content-Length: %d\r\n"
					"Connection: Keep-Alive\r\n\r\n",
					img_size);
				memcpy(buf + length, imgbuf, img_size);
				length += img_size;

				int sent1 = send(clientSocket, buf, length, 0);
				if (sent1 == SOCKET_ERROR)
					printf("content sending error\n");
				else
					printf("sending successful!\n");
			}
			//send html to browser
			else {
				strcpy(fileName, eraseSubstr(url));
				
				if (!fopen(fileName,"r"))		//can't find file in server folder
					code = NOT_FOUND;
				else if (!containsSubstr(url, ".htm") && !containsSubstr(url, ".jpg"))
					code = BAD_REQUESTION;
				else code = OTG;

				//according to code: send html file to client by HTTP datagram
				if (code == BAD_REQUESTION) {
					memset(fileName, 0, sizeof(fileName));
					strcpy(fileName, "400.htm");
				}
				if (code == NOT_FOUND) {
					memset(fileName, 0, sizeof(fileName));
					strcpy(fileName, "404.htm");
				}

				//send html
				if (strlen(url) == 1) {
					strcpy(fileName, filePath);
					strcat(fileName, "echoIndex.htm");
				}
				printf("request file name: %s\n\n", fileName);

				memset(fileBuff, 0, 1024);
				readHtml(fileName);
				sprintf(tmp, "content-length:%d\r\n\r\n", strlen(fileBuff));

				memset(sendBuff, 0, 1024);
				strcpy(sendBuff, "HTTP/1.1 200 ok\r\n");
				strcat(sendBuff, "Connection: Keep-Alive\r\n");
				strcat(sendBuff, tmp);
				//strcat(sendBuff, "content-length:33\r\n\r\n");
				strcat(sendBuff, fileBuff);
				//strcat(sendBuff, "<html><body>hello!</body></html>");
				//printf("%s\n", sendBuff);
				int sent = send(clientSocket, sendBuff, strlen(sendBuff), NULL);
				if (sent == SOCKET_ERROR)
					printf("content sending error\n");
				else
					printf("sending successful!\n");
			}
		}
	}
}

void keyboardCheck() {
	int ch, ch1;
	while (1) {
		Sleep(1000);
		if (_kbhit()) {
			ch = _getch();
			ch1 = _getch();
			if (ch == 27)
				echoFlag = 0;
		}
	}
}

int main() {

	std::thread t1(keyboardCheck);
	t1.detach();

	int echoFlag1;
	while (1) {

		echoFlag = 1;
		echoFlag1 = 1;

		//1. request protocol version
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);	//Start up
		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
			printf("request protocol version failure\n");
			return -1;			//clean up winsock, report
		}
		printf("request protocol success!\n");

		//2. initiate socket
		SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (serverSocket == SOCKET_ERROR) {
			printf("initiate socket failure\n");
			WSACleanup();
			return -2;
		}
		printf("initiate socket success!\n");

		//3. initiate protocol addr family
		sockaddr_in addr = { 0 };
		addr.sin_family = AF_INET;
		//manully set server ip and port
		char mip[16];
		int mport;
		cout << "input server ip to assign:(input 0 to assign local ip)" << endl;
		cin >> mip;
		cout << "input port you want to assign: ( range : 8888 - 18888 )" << endl;
		cin >> mport;
		if (!strcmp(mip, "0"))
			addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		else
			addr.sin_addr.S_un.S_addr = inet_addr(mip);
		addr.sin_port = htons(mport);	//htons and htonl: Host(byte) TO Network(byte) short/long
		if (!strcmp(mip, "0"))
			cout << "server set ip:" << "localhost" << "  server port:" << mport << endl;
		else
			cout << "server set ip:" << mip << "  server port:" << mport << endl;

		int ipath = 0;
		do {
			cout << "input main file path:" << endl;
			cin >> filePath;
			if (!strcmp(filePath, "0")) strcpy(filePath, getcwd(nullptr, 0));
			cout << "server main file path:" << filePath << endl;
			cout << "is this path legal?(1 for yes, and 0 for no)" << endl;
			cin >> ipath;
		} while (ipath == 0);
		strcat(filePath, "/");

		//4. bind
		::bind(serverSocket, (sockaddr*)&addr, sizeof addr);
		/*
		if (b == -1) {
			printf("binding failure");
			closesocket(serverSocket);
			WSACleanup();
			return -3;
		}
		*/
		printf("bind success!\n");

		//5. listen
		int l = listen(serverSocket, 10);
		if (l == -1) {
			printf("listening failure");
			system("pause");
			closesocket(serverSocket);
			WSACleanup();
			return -4;
		}
		printf("listen success!\n");

		//6. wait for client connection (×èÈû)
		//get and save client addr family
		while (1) {
			if (echoFlag == 0) {
				break;
			}
			clientSocket = accept(serverSocket, (sockaddr*)&cAddr, &len);
			if (echoFlag == 0) {
				break;
			}
			if (clientSocket == SOCKET_ERROR) {
				printf("socket failed");
				//8. close socket
				closesocket(serverSocket);
				//9. clean up protocol
				WSACleanup();
				return -5;
			}
			printf("\na client has connected to server !\n");
			printf("\nclient %d : IP addr : %s Port : %hu\n\n", scount, inet_ntoa(cAddr.sin_addr), cAddr.sin_port);
			//commBrowser(scount);
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)commBrowser, (LPVOID)clientSocket, NULL, NULL);

		}

		//8. close socket
		closesocket(serverSocket);
		printf("server socket closed\n");
		//9. clean up protocol
		WSACleanup();
		printf("server socket cleaned up\n");
	}
	return 0;
}