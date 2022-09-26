#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , no_result_request_number_(0)
    , current_time_(0) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(result.size());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddRequest(result.size());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_request_number_;
}
void RequestQueue::AddRequest(int results_number) {
    ++current_time_;
    if (!requests_.empty() && current_time_ - requests_.front().request_time >= min_in_day_) {
        // If delete empty request, to update empty requests for 24 hours
        if(requests_.front().results_number == 0) {
            --no_result_request_number_;
        }
        requests_.pop_front();
    }
    requests_.push_back({current_time_, results_number});
    if (results_number == 0) {
        ++no_result_request_number_;
    }
}
