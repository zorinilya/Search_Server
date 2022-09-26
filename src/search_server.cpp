#include "search_server.h"
#include "string_processing.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

using namespace std::string_literals;


SearchServer::SearchServer(const std::string& stop_text)
        : SearchServer(SplitIntoWords(stop_text)) {
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
     if (document_id < 0) {
          throw std::invalid_argument("invalid document_id"s);
     }
     if (IdIsExists(document_id)) {
          throw std::invalid_argument("invalid document_id"s);
     }
     std::vector<std::string> words = SplitIntoWordsNoStop(document);
     const double inv_word_count = 1.0 / words.size();
     for (const std::string& word : words) {
          word_to_document_freqs_[word][document_id] += inv_word_count;
     }
     documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
     document_id_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int document_rating)
            { return document_status == status; }
    );
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    if (index >= 0 && index < GetDocumentCount()) {
        return document_id_[index];
    } else {
        throw std::out_of_range("requested index out of range [0; number of documents)"s);
    }
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id) != 0) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IdIsExists(int new_id) {
    for (const int id : document_id_) {
        if (new_id == id) {
            return true;
        }
    }
    return false;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid char (with codes from 0 to 31)"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if ((text[0] == '-') && (text[1] == '-')) {
         throw std::invalid_argument("не более одного минуса перед словом"s);
    } else if ((text[0] == '-') && (text.size() == 1u)) {
        throw std::invalid_argument("не допустим отдельный минус"s);
    } else if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return QueryWord{
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid char (with codes from 0 to 31)"s);
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
}
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}
