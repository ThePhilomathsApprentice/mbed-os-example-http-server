/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _HTTP_SERVER_
#define _HTTP_SERVER_

#include "mbed.h"
#include "mbed-http/source/http_request_parser.h"
#include "mbed-http/source/http_response.h"
#include "http_response_builder.h"

#ifndef HTTP_SERVER_MAX_CONCURRENT
#define HTTP_SERVER_MAX_CONCURRENT      5// was 4. changed to 7 to test if this is causing TCP Connection failure(by Yash)
#endif

#define HTTP_RECEIVE_BUFFER_SIZE 1024
typedef HttpResponse ParsedHttpRequest;


#define FLAG_THREAD_TERMINATE_ME (1UL << 1)


extern Semaphore sockThreadSem;
extern Semaphore sockThreadLLSem;

Mail<bool, 1> extern wait_mail_box;//Only used for making the thread wait to indicate its deletion.

/**
 * \brief HttpServer implements the logic for setting up an HTTP server.
 */
class HttpServer {
public:
	/**
	 * HttpRequest Constructor
	 *
	 * @param[in] network The network interface
	 */
	HttpServer(NetworkInterface* network);
	~HttpServer();

	/**
	 * Start running the server (it will run on it's own thread)
	 */
	nsapi_error_t start(uint16_t port,  Callback<void(ParsedHttpRequest* request, TCPSocket* socket)> a_handler );

private:

	void receive_data( void );

	void main(); // main thread which will accept the new connections;

	typedef struct SOCK_THREAD_META_DATA {
		TCPSocket* socket;
		Thread*    thread;
		struct SOCK_THREAD_META_DATA* next;
	} socket_thread_metadata_t;


	TCPSocket* server;
	uint16_t _port;
	NetworkInterface* _network;
	Thread* main_thread;

	socket_thread_metadata_t* socket_threads_head;
	Callback<void(ParsedHttpRequest* request, TCPSocket* socket)> handler;

	EventFlags ThreadTerminationFlag;
};

#endif // _HTTP_SERVER
