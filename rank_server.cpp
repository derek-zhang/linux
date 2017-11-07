#include <thread>
#include <string>
#include <fstream>
#include <unistd.h>
#include <Eigen/Core>
//#include <tbb/concurrent_unordered_map.h>
#include <unordered_map>
#include <gflags/gflags.h>
#include <comlog/comlog.h>
#include <baidu/rpc/server.h>
#include "document.pb.h"
#include "rank_service.pb.h"
#include <pthread.h>

pthread_rwlock_t l;

typedef ::Eigen::Matrix<float, 32, 1> Vector;
typedef ::Eigen::Map<Vector> MVector;
using ::baidu::lijiang01::proto::Document;

DEFINE_int32(log_level, 4, "");
DEFINE_uint64(limit, 10000, "");
DEFINE_int32(port, 8899, "");
DEFINE_int32(sleep, 1000, "");

static ::std::atomic<uint64_t> all;
static int64_t latency_us;

class Node {
public:
    Node(const Node& other) {
        char* x = NULL;
        CFATAL_LOG(" copy");
        x[10000] = 10;
    }

    Node(Node&& other) : _brife(other._brife.exchange(nullptr, ::std::memory_order_relaxed)) {}

    Node(const Document& brife) : _brife(new Document(brife)) {}

    const Document* brife() const {
        return _brife.load(::std::memory_order_relaxed);
    }

    void brife(Document* new_brife) {
        auto old_brife = _brife.exchange(new_brife, ::std::memory_order_relaxed);
        if (old_brife != nullptr) {
            usleep(1000);
            delete old_brife;
        }
    }

    void brife(Document&& new_brife) {
        auto created_brife = new Document();
        created_brife->Swap(&new_brife);
        brife(created_brife);
    }

    ~Node() {
        brife(nullptr);
    }

private:
    ::std::atomic<const Document*> _brife;
};

class RankServiceImpl : public ::baidu::lijiang01::proto::RankService {
public:
    RankServiceImpl(
        //::tbb::concurrent_unordered_map<uint64_t, Node>& store
        ::std::unordered_map<uint64_t, Node>& store
        ) : _store(store) {};
    virtual ~RankServiceImpl() {};
    virtual void rank(google::protobuf::RpcController* cntl_base,
                      const ::baidu::lijiang01::proto::RankRequest* request,
                      ::baidu::lijiang01::proto::RankResponse* response,
                      google::protobuf::Closure* done) {
        auto begin = ::base::cpuwide_time_ns();
        Vector v;
        for (size_t i = 0; i < 32; i++) {
            v(i, 0) = 0.0;
        }
        response->mutable_nid()->Reserve(request->nid_size());
        response->mutable_score()->Reserve(request->nid_size());
            pthread_rwlock_rdlock(&l);
        for (auto nid : request->nid()) { 
            auto it = _store.find(nid);
            auto& node = it->second;
            const Document* x = node.brife();
            MVector mv((float*)(x->ann_vector().data()));
            float score = v.dot(mv);
            response->add_nid(nid);
            response->add_score(score);

        }
            pthread_rwlock_unlock(&l);
        all.fetch_add(1, ::std::memory_order_relaxed);
        auto end = ::base::cpuwide_time_ns();
        latency_us = end - begin;
        done->Run();
    }

private:
    //::tbb::concurrent_unordered_map<uint64_t, Node>& _store;
    ::std::unordered_map<uint64_t, Node>& _store;
};

//::tbb::concurrent_unordered_map<uint64_t, Node> store;
::std::unordered_map<uint64_t, Node> store;
static void status() {
    uint64_t last_all = all.load(::std::memory_order_relaxed);
    ::std::ifstream doc("d");
    ::std::string line;
    Document document;
    for (size_t i = 0; i < 32; i++) {
        document.add_ann_vector(0.0);
    }

    while (true) {
        usleep(FLAGS_sleep);
        uint64_t current_all = all.load(::std::memory_order_relaxed);
        last_all = current_all;
        ::std::getline(doc, line);
        char* brife_start = nullptr;
        uint64_t nid = strtoull(line.c_str(), &brife_start, 10);
        ++brife_start;
        document.set_data(brife_start);

        pthread_rwlock_wrlock(&l);
        store.emplace(nid, Node(document));
        pthread_rwlock_unlock(&l);
    }
}

int32_t main(int32_t argc, char** argv) {
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    pthread_rwlock_init(&l, NULL);

    com_logstat_t logstat;
    logstat.sysevents = FLAGS_log_level;
    com_device_t dev[1];
    strcpy(dev[0].type, "TTY");
    COMLOG_SETSYSLOG(dev[0]);
    com_openlog("main", dev, 1, &logstat);

    {
        ::std::ifstream doc("doc");
        ::std::string line;
        uint64_t num = 0;
        Document document;
        for (size_t i = 0; i < 32; i++) {
            document.add_ann_vector(0.0);
        }
        while (::std::getline(doc, line)) {
            if (++num > FLAGS_limit) {
                break;
            }
            char* brife_start = nullptr;
            uint64_t nid = strtoull(line.c_str(), &brife_start, 10);
            ++brife_start;
            document.set_data(brife_start);
            store.emplace(nid, Node(document));
        }
    }

    CNOTICE_LOG(" doc = %llu", store.size());

    ::baidu::rpc::Server server;
    ::RankServiceImpl rank_service(store);
    server.AddService(&rank_service, ::baidu::rpc::SERVER_DOESNT_OWN_SERVICE);

    baidu::rpc::ServerOptions options;
    server.Start(FLAGS_port, &options);

    ::std::thread status_thread(status);
    status_thread.detach();
    
    server.RunUntilAskedToQuit();

    return 0;
}
