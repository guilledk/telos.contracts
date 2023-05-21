#include <telos.gpu/telos.gpu.hpp>

namespace telos {

    void gpu::enqueue(const name& user, const string& request_data) {
        require_auth(user);

        // MAYBE CHECK IF USER HAS GPU RESOURCE HERE

        work_queue _queue(get_self(), get_self().value);
        _queue.emplace(user, [&](auto& r) {
            r.id = _queue.available_primary_key();
            r.user = user;
            r.data = request_data;
            r.timestamp = current_time_point();
        });
    }

    void gpu::dequeue(const name& user, const uint64_t request_id) {
        require_auth(user);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        _queue.erase(rit);
    }

    void gpu::workbegin(const name& worker, const uint64_t request_id) {
        require_auth(worker);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        worker_status _status(get_self(), request_id);
        auto it = _status.find(request_id);
        check(it == _status.end(), "request already started");

        _status.emplace(worker, [&](auto& s) {
            s.worker = worker;
            s.status = "started";
            s.started = current_time_point();
        });

    }

    void gpu::workcancel(const name& worker, const uint64_t request_id) {
        require_auth(worker);

        worker_status _status(get_self(), request_id);
        auto it = _status.find(request_id);
        check(it != _status.end(), "status not found");

        _status.erase(it);
    }

    void gpu::submit(
        const name& worker,
        const uint64_t request_id,
        const checksum256 result_hash
    ) {
        require_auth(worker);

        work_queue _queue(get_self(), get_self().value);
        auto rit = _queue.find(request_id);
        check(rit != _queue.end(), "request not found");

        worker_status _status(get_self(), request_id);
        auto it = _status.find(request_id);
        check(it != _status.end(), "status not found");

        _status.erase(it);

        work_results _results(get_self(), get_self().value);
        auto worker_index = _results.get_index<"byworker"_n>();
        auto wit = worker_index.find(worker.value);
        check(wit == worker_index.end(), "already submited result");

        _results.emplace(worker, [&](auto& r) {
            r.id = _results.available_primary_key();
            r.request_id = request_id;
            r.user = rit->user;
            r.worker = worker;
            r.result_hash = result_hash;
            r.submited = current_time_point();
        });
    }
}  // namespace
