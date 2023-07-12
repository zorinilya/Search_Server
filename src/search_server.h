#pragma once

#include "string_processing.h"
#include "document.h"

#include <algorithm>
#include <execution>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;    // allowable error;

class SearchServer {
public:
    template<typename StringCollection>
    SearchServer(const StringCollection& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        using namespace std::string_literals;
        for (const std::string& stop_word : stop_words_) {
            if (!IsValidWord(stop_word)) {
                throw std::invalid_argument("invalid char (with codes from 0 to 31)"s);
            }
        }
    }

    explicit SearchServer(const std::string& stop_text);

    explicit SearchServer(std::string_view stop_text);

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename Predicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::set<int>::iterator begin();

    std::set<int>::iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::sequenced_policy seq_policy, int document_id);

    void RemoveDocument(std::execution::parallel_policy par_policy, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            std::execution::sequenced_policy seq_policy,
            const std::string_view& raw_query,
            int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
            std::execution::parallel_policy par_policy,
            const std::string_view& raw_query,
            int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_id_;

    bool IsStopWord(const std::string_view& word) const;

    bool IdIsExists(int new_id);

    static bool IsValidWord(const std::string_view& word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view, std::less<>> plus_words;
        std::set<std::string_view, std::less<>> minus_words;
    };

    struct QueryPar {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view& text) const;

    QueryPar ParseQueryPar(const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;
};

template<typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, Predicate predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = SearchServer::FindAllDocuments(query, predicate);
    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        const double delta = std::abs(lhs.relevance - rhs.relevance);    // compute equality with required precision;
        if (delta < EPS) {
            return lhs.rating > rhs.rating;
        }
        return lhs.relevance > rhs.relevance;
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template<typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
            const DocumentData& document_data = documents_.at(document_id);
            if (predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
        });
    }
    return matched_documents;
}
