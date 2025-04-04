//#include "mongoose.h"

#include <WinSock2.h>
#include <thread>

#include "src/Diagnostics/Diagnostics.h"
#include <csignal>


//for affinity
#include <tchar.h>
#include <psapi.h>

#include "iostream"
#include <exception>
#include <stdexcept>

#include "deque"

Diagnostics diag;
const char* root = "../html";   //bad move
struct mg_http_serve_opts opts = {
        //.mime_types = extension_to_type(extension)
};
static int s_signo;
static void signal_handler(int signo) {
    s_signo = signo;
}

struct Task{
    long long int begin = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::thread* thread;
    struct mg_connection *c;
    char *request;
    Json response;
};

static std::deque<Task> deque;

static void threaded_reply(Task* task){
    SetThreadAffinityMask(GetCurrentThread(), 0b0001111000000000);
    //std::cout << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;
    //std::cout << "reply thread affinity: " << ' ' << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;

    Json payload({});
    try{
        payload = Json::parse(task->request);
    }catch(Json::parse_error& err){
        delete[] task->request;
        task->response =Json({
                                              {"ok", false},
                                              {"err", "request is not a valid JSON"}
                                      });
    }
    delete[] task->request;
    std::exception_ptr eptr;
    //do stuff
    try{
        task->response = diag.handleRequest(payload);

        /*
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);
        task->response =Json({
                                     {"ok", true},
                                     {"comment", "threaded"}
        });
        */
    }catch(...){
        eptr = std::current_exception();
    }
    try{
        if (eptr)
            std::rethrow_exception(eptr);
    }
    catch(const std::exception& e){
        std::cout << "exception in diag.handleRequest: '" << e.what() << "'\n";
        task->response =Json({
                                     {"ok", false},
                                     {"err", e.what()}
                             });
    }

    unsigned char ok = 1;
    mg_wakeup(task->c->mgr, task->c->id, &ok, sizeof(ok));
}

static void handleHTTP(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        auto *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/api"), NULL)) {
            char *request = new char[hm->body.len + 1]; //deleted in threadedReply
            std::memcpy(request, hm->body.buf, hm->body.len);
            request[hm->body.len] = '\0';

            deque.emplace_back(
                    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                    nullptr,
                            c,
                            request,
                    Json()
            );
            
            deque.back().thread = new std::thread(threaded_reply, &deque.back());
        }else{
            //std::cout << "req: " << hm->uri.buf << std::endl;
            char *path;
            if (hm->uri.buf[hm->uri.len - 1] == '/') {
                path = new char[strlen(root) + hm->uri.len + strlen("index.html") + 1]();
                std::strcpy(path, root);
                std::memcpy(path + strlen(root), hm->uri.buf, hm->uri.len);
                std::strcat(path, "index.html");
            }else if(mg_match(hm->uri, mg_str("/pub"), NULL)){
                
                std::cout << "matched" << std::endl;
                std::string pub = "//172.16.12.127/Pub/Диагностики/Томсоновская диагностика/Томсоновская диагностика (описание).pdf";
                
                std::cout << hm->body.buf << std::endl;
                path = new char[pub.size() + 1]();
                std::strcpy(path, pub.c_str());
                //std::memcpy(path + pub.size(), hm->body.buf, hm->body.len);
            }else{
                path = new char[strlen(root) + hm->uri.len + 1]();
                std::strcpy(path, root);
                std::memcpy(path + strlen(root), hm->uri.buf, hm->uri.len);
            }
            //std::cout << "Try serve file: " << path << std::endl;
            mg_http_serve_file(c, hm, path, &opts);
            delete[] path;
        }
    }else if(ev == MG_EV_WAKEUP){
        for(auto i = deque.begin(); i != deque.end(); i++){
            if(i->c->id == c->id){
                //i->response["elapsed seconds"] = (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() - i->begin);
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                              "%s", i->response.dump().c_str());           
                i->thread->join();
                delete i->thread;
                deque.erase(i);
                break;
             };
        }
    }else if(ev == MG_EV_CLOSE){
        c = nullptr;
    }else if (ev != MG_EV_POLL and ev != MG_EV_WRITE and ev != MG_EV_OPEN and ev != MG_EV_ACCEPT and ev != MG_EV_HTTP_HDRS and ev != MG_EV_READ){
        //check deque for timeouts

        std::cout << "Unhandled event: " << ev << " connection ID: " << c->id << ' ';
        for(int i = 0; i < 4; i++){
            std::cout << (int)(c->rem.ip[i]) << '.';
        }
        std::cout << (int)c->rem.port << std::endl;
    };
}

int main() {
    //mg_log_set(1);
    std::cout << "CTS c++ server v0" << std::endl;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGABRT, signal_handler);

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ){
        return 1;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    size_t ok = 0;
    for (i = 0; i < cProcesses; i++ ){
        if( aProcesses[i] != 0 ){
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                           PROCESS_ALL_ACCESS,
                                           FALSE, aProcesses[i] );
            ok += SetProcessAffinityMask(hProcess, 0b1110000000000000);
            //std::cout  << aProcesses[i] << ' ' << SetProcessAffinityMask(hProcess, 0b1110000000000000) << std::endl;
            CloseHandle( hProcess );
        }
    }
    std::cout << "moved " << ok << "/" << cProcesses << std::endl;
    std::cout << "Process affinity: " << ' ' << SetProcessAffinityMask(GetCurrentProcess(), 0b0001111111111111) << std::endl;
    std::cout << "process realtime: " << ' ' << SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) << std::endl;

    const unsigned long long mask = 0b0001111000000000;
    std::cout << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;
    std::cout << "Main thread " << GetCurrentThreadId() << " , affinity: " << ' ' << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;



    struct mg_mgr mgr;
    mg_mgr_init(&mgr);                                      // Init manager
    diag.setMgr(&mgr);

    mg_http_listen(&mgr, "http://0.0.0.0:80", handleHTTP, NULL);  // api/web
    mg_listen(&mgr, "udp://0.0.0.0:8888", diag.handleUDPBroadcast, &diag); //udp start/stop
    mg_listen(&mgr, "udp://0.0.0.0:4001", Laser330::cfn, &diag.laser); //udp laser

    mg_wakeup_init(&mgr);  // Initialise wakeup socket pair
    std::cout << "Server alive" << std::endl;
    while(s_signo == 0) {
        mg_mgr_poll(&mgr, 1);
    }
    std::cout << "Stop poll" << std::endl;
    diag.die();

    for(auto iter = deque.begin(); iter != deque.end();){
        delete iter->thread;
        deque.erase(iter);
    }
    std::cout << "Eraised qeque" << std::endl;

    aProcesses[1024], cbNeeded, cProcesses;
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ){
        return 1;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    ok = 0;
    for (i = 0; i < cProcesses; i++ ){
        if( aProcesses[i] != 0 ){
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                           PROCESS_ALL_ACCESS,
                                           FALSE, aProcesses[i] );
            ok += SetProcessAffinityMask(hProcess, 0b1111111111111111);
            //std::cout  << aProcesses[i] << ' ' << SetProcessAffinityMask(hProcess, 0b1110000000000000) << std::endl;
            CloseHandle( hProcess );
        }
    }

    mg_mgr_free(&mgr);

    std::cout << "server exit" << std::endl << std::flush;
    using std::operator""s;
    std::this_thread::sleep_for(2s);
    return 0;
}