#include <mbed.h>
#include <EthernetInterface.h>
#include "http_server.h"
#include "http_response_builder.h"

DigitalOut led(LED1);

EthernetInterface eth;

// Requests come in here
void request_handler(ParsedHttpRequest* request, TCPSocket* socket) {

    printf("Request came in: %s %s\n", http_method_str(request->get_method()), request->get_url().c_str());

    if (request->get_method() == HTTP_GET && request->get_url() == "/") {
        HttpResponseBuilder builder(200);
        builder.set_header("Content-Type", "text/html; charset=utf-8");

        char response[] = "<html><head><title>Hello from mbed</title></head>"
            "<body>"
                "<h1>mbed webserver</h1>"
                "<button id=\"toggle\">Toggle LED</button>"
                "<script>document.querySelector('#toggle').onclick = function() {"
                    "var x = new XMLHttpRequest(); x.open('POST', '/toggle'); x.send();"
                "}</script>"
            "</body></html>";

        builder.send(socket, response, sizeof(response) - 1);
    }
    else if (request->get_method() == HTTP_POST && request->get_url() == "/toggle") {
        printf("toggle LED called\n");
        led = !led;

        HttpResponseBuilder builder(200);
        builder.send(socket, NULL, 0);
    }
    else {
        HttpResponseBuilder builder(404);
        builder.send(socket, NULL, 0);
    }
}

int main() {

	printf("INFO:MBED_Version:%d.%d.%d\n",MBED_MAJOR_VERSION,MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    // Connect to the network (see mbed_app.json for the connectivity method used)
	SocketAddress a;
	nsapi_error_t conn_err = eth.connect();
	eth.get_ip_address(&a);

    HttpServer server(&eth);
    nsapi_error_t res = server.start(8080, &request_handler);

    if (res == NSAPI_ERROR_OK) {
        printf("INFO:Server is listening at http://%s:8080\n", a.get_ip_address());
    }
    else {
        printf("INFO:Server could not be started... %d\n", res);
    }

    ThisThread::sleep_for(osWaitForever);
}
