#include "mongoose.h"
#include <cstring>
#include <string>
#include "RequestHandler.h"
#include <csignal>
#include <thread>

#include "iostream"

const char* root = "../html";   //bad move
struct mg_http_serve_opts opts = {
        //.mime_types = extension_to_type(extension)
};
static int s_signo;
static void signal_handler(int signo) {
    s_signo = signo;
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        auto *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/api"), NULL)) {
            char* request = new char[hm->body.len + 1];
            std::memcpy(request, hm->body.buf, hm->body.len);
            request[hm->body.len] = '\0';
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                          "%s", handleRequest(request).c_str());
        } else {
            char * path;
            if (hm->uri.buf[hm->uri.len - 1] == '/') {
                path = new char [strlen(root) + hm->uri.len + strlen("index.html") + 1]();
                std::strcpy (path,root);
                std::memcpy(path + strlen(root), hm->uri.buf, hm->uri.len);
                std::strcat(path, "index.html");
            }else{
                path = new char [strlen(root) + hm->uri.len + 1]();
                std::strcpy (path, root);
                std::memcpy(path + strlen(root), hm->uri.buf, hm->uri.len);
            }

            mg_http_serve_file(c, hm, path, &opts);

            delete[] path;
        }
    }
}

int main() {
    //mg_log_set(1);
    std::cout << "CTS c++ server v0" << std::endl;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGABRT, signal_handler);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);                                      // Init manager
    mg_http_listen(&mgr, "http://0.0.0.0:99", fn, NULL);  // Setup listener
    std::cout << "Server alive" << std::endl;
    while(s_signo == 0) {
        mg_mgr_poll(&mgr, 500);
    }

    mg_mgr_free(&mgr);

    std::cout << "server exit" << std::endl << std::flush;
    using std::operator""s;
    std::this_thread::sleep_for(2s);
    return 0;
}