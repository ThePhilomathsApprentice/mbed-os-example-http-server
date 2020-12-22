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

#include "http_server.h"

Semaphore sockThreadSem;
Semaphore sockThreadLLSem;

Mail<bool, 1> wait_mail_box;//Only used for making the thread wait to indicate its deletion.


/**
 * HttpRequest Constructor
 *
 * @param[in] network The network interface
 */
HttpServer::HttpServer(NetworkInterface* network) {
	//printf("INFO: httpServer Constructor_START\n");
	_network = network;
	server = NULL;
	socket_threads_head = NULL;
	main_thread = NULL;
	_port = 8080;
	//printf("INFO: httpServer Constructor_END\n");

}

HttpServer::~HttpServer() {
	//printf("INFO: httpServer Desstructor_START\n");
	server->close();

	delete server;

	socket_threads_head = NULL;

	//printf("INFO: httpServer Destructor_END\n");
}

/**
 * Start running the server (it will run on it's own thread)
 */
nsapi_error_t HttpServer::start(uint16_t port,  Callback<void(ParsedHttpRequest* request, TCPSocket* socket)> a_handler ) {
	printf("INFO: Server Started.. \n");

	handler = a_handler;

	_port = port;//will be used by main thread to restart the server.
	if( server != NULL ){
		//printf("INFO: server is not null, terminating \n");
		server->close();
		delete server;
		server = NULL;

	}
	server = new TCPSocket();

	nsapi_error_t ret;

	ret = server->open(_network);
	if (ret != NSAPI_ERROR_OK) {
		return ret;
	}
	//printf("INFO: Server opened.. \n");

	int i =2;
	ret = server->setsockopt(NSAPI_SOCKET, NSAPI_REUSEADDR, (void*)&i , sizeof(i));//NSAPI_SOCKET, NSAPI_REUSEADDR,&(int){1}, sizeof(int)
	if (ret != NSAPI_ERROR_OK) {
		printf("FAILED: setsockopt failed(%d)\n",ret);
		return ret;
	}

	ret = server->bind(_port);
	if (ret != NSAPI_ERROR_OK) {
		printf("INFO: Server not binded..(%d) \n",ret);
		return ret;
	}
	//printf("INFO: Server binded.. \n");

	server->listen(HTTP_SERVER_MAX_CONCURRENT); //HTTP_SERVER_MAX_CONCURRENT :  max. concurrent connections...
	//printf("INFO: Server listened.. \n");


	if( main_thread != NULL ){
		//printf("INFO: main_thread is not null, terminating(%d) \n",main_thread->get_state());
		osStatus ret =  main_thread->terminate();
		//delete main_thread;
		printf("INFO: main_thread is not null, terminating(%d)\n",ret);
		main_thread = NULL;

	}

	main_thread = new Thread();
	//printf("INFO: Server Thread created..(main_thread->get_id():%d) \n",main_thread->get_id());
	main_thread->start(callback(this, &HttpServer::main));


	return NSAPI_ERROR_OK;
}

void HttpServer::receive_data( ) {

	//printf("INFO: Enterered working thread.\n");

	mbed_stats_heap_t heap_stats;
	mbed_stats_heap_get(&heap_stats);
	//printf(" INFO:main() Heap size: %lu/%lu bytes\n", heap_stats.current_size, heap_stats.reserved_size); //Removed MBED_PRN due to too many log files getting created with seemingly no useful information.


	TCPSocket* socket = NULL;

	sockThreadLLSem.acquire();
	for (socket_thread_metadata_t* sockThreadITR = socket_threads_head ; sockThreadITR != NULL ; sockThreadITR = sockThreadITR->next){

		sockThreadSem.acquire();
		//printf("INFO:ThisThread:(%d)::sock_threads:(%d)\n",ThisThread::get_id(),sockThreadITR->thread->get_id());
		sockThreadSem.release();

		if(sockThreadITR->thread->get_id() == ThisThread::get_id() ){
			socket = sockThreadITR->socket;

			sockThreadSem.acquire();
			//printf("INFO: Thread found the corresponding Socket(%d).\n",socket);
			sockThreadSem.release();

		}else{
			continue;
		}

	}
	sockThreadLLSem.release();

	// needs to keep running until the socket gets closed
	while (1) {

		ParsedHttpRequest* response = new ParsedHttpRequest();
		HttpParser* parser = new HttpParser(response, HTTP_REQUEST);

		// Set up a receive buffer (on the heap)
		uint8_t* recv_buffer = (uint8_t*)malloc(HTTP_RECEIVE_BUFFER_SIZE);


		// TCPSocket::recv is called until we don't have any data anymore
		nsapi_size_or_error_t recv_ret;
		while ((recv_ret = socket->recv(recv_buffer, HTTP_RECEIVE_BUFFER_SIZE)) > 0) {
			// Pass the chunk into the http_parser
			size_t nparsed = parser->execute((const char*)recv_buffer, recv_ret);
			if (nparsed != recv_ret) {
				printf("FAILED: receive_data():Parsing failed... parsed %d bytes, received %d bytes\n", nparsed, recv_ret);
				recv_ret = -2101;
				break;
			}

			if (response->is_message_complete()) {
				break;
			}
		}
		// error?
		switch(recv_ret){

		case NSAPI_ERROR_OK://0

		case NSAPI_ERROR_IS_CONNECTED://-3015
		case NSAPI_ERROR_WOULD_BLOCK://-3001
		case NSAPI_ERROR_IN_PROGRESS://-3013
		case NSAPI_ERROR_ALREADY://-3014
			//removed irrelevant code which never gets executed and also was not deleting socket.
		case NSAPI_ERROR_UNSUPPORTED://-3002
		case NSAPI_ERROR_PARAMETER://-3003
		case NSAPI_ERROR_NO_CONNECTION://-3004
		case NSAPI_ERROR_NO_SOCKET://-3005, in what condition does this get returned?
		case NSAPI_ERROR_NO_ADDRESS://-3006
		case NSAPI_ERROR_NO_MEMORY://-3007
		case NSAPI_ERROR_NO_SSID://-3008
		case NSAPI_ERROR_DNS_FAILURE://-3009
		case NSAPI_ERROR_DHCP_FAILURE://-3010
		case NSAPI_ERROR_AUTH_FAILURE:// -3011
		case NSAPI_ERROR_DEVICE_ERROR://-3012
		case NSAPI_ERROR_CONNECTION_LOST://-3016
		case NSAPI_ERROR_CONNECTION_TIMEOUT://-3017
		case NSAPI_ERROR_ADDRESS_IN_USE://-3018
		case NSAPI_ERROR_TIMEOUT://-3019
			sockThreadSem.acquire();
			//printf("INFO: FAILED socket failed to accept connection.(%d)\n",recv_ret);
			sockThreadSem.release();

			// error = recv_ret;
			delete response;
			response = NULL;

			// When done, call parser.finish()
			parser->finish();
			delete parser;
			parser = NULL;

			free(recv_buffer);
			recv_buffer = NULL;


			//                    socket->close();
			nsapi_error_t ret = socket->close();
			//printf("INFO: socket->close called in webserver_thread.(ret=%d)\n",ret);
			if(ret == 0){

				sockThreadSem.acquire();
				printf(" Deleted socket in the list(%d)(%d).\n",ThisThread::get_id(),socket);
				sockThreadSem.release();

				//delete socket;
				//socket = NULL;

			}

			wait_mail_box.get();//No one will put any message in this.. This is used only to make the thread wait here and let the linked list manager delete it.

			break;
		}

		if(recv_ret == 0){
			continue;

		}

		// When done, call parser.finish()
		parser->finish();

		// Free the receive buffer
		free(recv_buffer);
		recv_buffer = NULL;

		// Let user application handle the request, if user needs a handle to response they need to memcpy themselves
		if (recv_ret > 0) {
			//respSem.acquire();
			handler(response, socket);
			//respSem.release();

		}

		// Free the response and parser
		delete response;
		response = NULL;
		delete parser;
		parser = NULL;
	}

	// sock_mutex.unlock();


}


void HttpServer::main() {

	while (1) {

		mbed_stats_heap_t heap_stats;
		mbed_stats_heap_get(&heap_stats);
		//printf(" INFO:main() Heap size: %lu/%lu bytes\n", heap_stats.current_size, heap_stats.reserved_size); //Removed MBED_PRN due to too many log files getting created with seemingly no useful information.


		Thread* t =NULL;
		socket_thread_metadata_t* m=NULL;

		nsapi_error_t accept_res;
		sockThreadSem.acquire();
		//printf("INFO: httpServer:main():Socket getting Alloted\n");//TESTING
		sockThreadSem.release();
		TCPSocket* clt_sock = server->accept(&accept_res);//How to use accept_res? what values are expected to be present in it on success or failure?
		if(clt_sock != NULL){
			sockThreadSem.acquire();
			//printf("INFO: server Success Path.(%d)\n",clt_sock);
			sockThreadSem.release();

			// and start listening for events there
			t = new Thread(osPriorityNormal, 2048, NULL, "workerThread" );

			m = new socket_thread_metadata_t();
			m->socket = clt_sock;
			m->thread = t;
			m->next = NULL;
			clt_sock = NULL;



			if(!socket_threads_head){
				socket_threads_head = m;
			}else{
				socket_thread_metadata_t* tempPTR = socket_threads_head;

				while( tempPTR->next != NULL ){
					tempPTR = tempPTR->next;
				}
				tempPTR->next = m;

			}


			t->start(callback(this, &HttpServer::receive_data));
			//t->start( callback(receive_data, clt_sock ) );






		}else{
			sockThreadSem.acquire();

			//printf("INFO: FAILED server failed to accept connection.(%d)\n",clt_sock);
			sockThreadSem.release();

			sockThreadSem.acquire();
			//printf("INFO: Exiting FAILED server failed to accept connection(accept_res:%d).\n",(accept_res));
			sockThreadSem.release();

			nsapi_error_t ret;
			if( server != NULL ){
				//printf("INFO: server is not null, terminating \n");

				ret = server->close();//Closing before reopeneing.
				if (ret != NSAPI_ERROR_OK) {
					//printf("INFO: Server not closed..(%d) \n",ret);
					//return ret;
				}

				delete server;
				server = NULL;

			}
			server = new TCPSocket();

			ret = server->open(_network);
			if (ret != NSAPI_ERROR_OK) {
				printf("INFO: _main thread_ Server not opened..(%d) \n",ret);
				//return ret;
			}
			//printf("INFO: Server opened.. \n");

			int i =1;
			ret = server->setsockopt(NSAPI_SOCKET, NSAPI_REUSEADDR, (void*)&i , sizeof(i));//NSAPI_SOCKET, NSAPI_REUSEADDR,&(int){1}, sizeof(int)
			if (ret != NSAPI_ERROR_OK) {
				printf("FAILED: _main thread_ setsockopt failed(%d)\n",ret);
				//return ret;
			}

			ret = server->bind(_port);
			if (ret != NSAPI_ERROR_OK) {
				printf("INFO: _main thread_ Server not binded..(%d) \n",ret);
				//return ret;
			}
			//printf("INFO: Server binded.. \n");

			ret = server->listen(HTTP_SERVER_MAX_CONCURRENT); //HTTP_SERVER_MAX_CONCURRENT :  max. concurrent connections...
			if (ret != NSAPI_ERROR_OK) {
				printf("INFO: _main thread_ Server not listened..(%d) \n",ret);
				//return ret;
			}
			//printf("INFO: Server listened.. \n");
			continue;


		}

		ThisThread::sleep_for(10);//give chance to the newly created thread to parse the response first.


		socket_thread_metadata_t* sockThreadPrevITR = socket_threads_head;
		sockThreadLLSem.acquire();
		for (socket_thread_metadata_t* sockThreadITR = socket_threads_head ; sockThreadITR != NULL ; ){

			Thread::State thState = sockThreadITR->thread->get_state();
			//printf("httpServer:main():while(1):Entered for loop\n");
			sockThreadSem.acquire();
			printf("httpServer:main():while(1):INFO Thread.(%d:%d)\n",sockThreadITR->thread->get_id(),thState);
			sockThreadSem.release();

			if ( thState == Thread::WaitingMessageGet ) {// || Thread::WaitingThreadFlag  || Thread::Inactive || Thread::Deleted || Thread::WaitingMailbox

				sockThreadSem.acquire();
				//printf("httpServer:main():while(1):deleting Thread.(%d:%d)\n",sockThreadITR->thread,sockThreadITR->thread->get_id());
				sockThreadSem.release();

				//printf("httpServer:main():while(1): deleting sockThreadITR->thread_Entering\n");
				if( sockThreadITR->thread->get_id() != 0 ){
					sockThreadSem.acquire();
					//printf("httpServer:main():while(1): deleting sockThreadITR->thread\n");
					sockThreadSem.release();
					delete sockThreadITR->thread;
					sockThreadITR->thread = NULL;
				}else{
					//printf("FAILED: Thread Destructor NOT CALLED(%d)\n",sockThreadITR->thread->get_id() );

				}

				//printf("httpServer:main():while(1): deleting sockThreadITR->thread_Exiting\n");

				sockThreadSem.acquire();
				//printf("httpServer:main():while(1):deleting socket_threads_element\n");
				sockThreadSem.release();


				if(sockThreadITR->next != NULL){//This is not the last node.

					sockThreadITR->socket = sockThreadITR->next->socket;

					sockThreadITR->thread = sockThreadITR->next->thread;

					socket_thread_metadata_t* sockThreadnextTemp = sockThreadITR->next;

					sockThreadITR->next = sockThreadITR->next->next;

					//printf("httpServer:main():while(1):deleting LL_Node(%d)(%d)\n",sockThreadnextTemp->thread,sockThreadnextTemp->thread->get_id());

					delete sockThreadnextTemp;



				}else{//This is the last node.
					//printf("httpServer:main():while(1):deleting LL_LAST_Node()\n");
					if(sockThreadPrevITR->next == sockThreadITR ) {//LL is of minimum 2 nodes..
						//printf("httpServer:main():while(1):deleting LL_MIN_2_Node(%d)(%d)\n",sockThreadPrevITR->next->thread,sockThreadPrevITR->next->thread->get_id());
						delete sockThreadPrevITR->next;
						break;

					}else{//sockThreadITR and sockThreadPrevITR are the same, which means this is the 1st and last node of LL.
						//printf("httpServer:main():while(1):deleting LL_1st&LAST_Node(%d)(%d)\n",socket_threads_head->thread,socket_threads_head->thread->get_id() );
						delete socket_threads_head;
						socket_threads_head = NULL;
						break;

					}

				}



			}else{
				sockThreadPrevITR = sockThreadITR;
				sockThreadITR = sockThreadITR->next;

			}

		}
		printf("LL_START\n");
		for (socket_thread_metadata_t* sockThreadITR = socket_threads_head ; sockThreadITR != NULL ; sockThreadITR = sockThreadITR->next ){

			printf("socket_threads_head=%d\n",socket_threads_head);
			printf("sockThreadITR=%d\n",sockThreadITR);
			printf("sockThreadITR->socket=%d\n",sockThreadITR->socket);
			printf("sockThreadITR->thread=(%d)(%d)\n",sockThreadITR->thread,sockThreadITR->thread->get_id());
			printf("sockThreadITR->threadState=(%d)\n",sockThreadITR->thread->get_state());
			printf("sockThreadITR->next=%d\n",sockThreadITR->next);

		}
		printf("LL_END\n");

		sockThreadLLSem.release();

		ThisThread::sleep_for(10);

	}

}
