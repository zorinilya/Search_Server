#include "log_duration.h"
#include "search_server.h"
#include "string_processing.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


using namespace std::string_literals;

SearchServer::SearchServer(const std::string& stop_text)
        : SearchServer(std::string_view(stop_text)) {
}

SearchServer::SearchServer(std::string_view stop_text)
        : SearchServer(SplitIntoWords(stop_text)) {
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("invalid document_id"s);
    }
    if (IdIsExists(document_id)) {
        throw std::invalid_argument("invalid document_id"s);
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_freqs_[document_id][std::string(word)] += inv_word_count;
        word_to_document_freqs_[std::string(word)][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_id_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int document_rating)
            { return document_status == status; }
    );
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return document_id_.size();
}

std::set<int>::iterator SearchServer::begin() {
    return document_id_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return document_id_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> empty;
    if (!word_freqs_.count(document_id)) {
        return empty;
    }
    return (const std::map<std::basic_string_view<char>, double>&) word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_id_.count(document_id) == 0) {
        return;
    }
    for(const auto& [word, freq] : word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
        // удалить запись, если слово не присутствует в остальных документах
        if (word_to_document_freqs_.at(word).size() == 0) {
            word_to_document_freqs_.erase(word);
        }
    }
    word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_id_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy seq_policy, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par_policy, int document_id) {
    if (document_id_.count(document_id) == 0) {
        return;
    }
    std::vector<const std::string*> deleted_words;
    for(const auto& [word, freq] : word_freqs_.at(document_id)) {
        deleted_words.push_back(&word);
    }
    std::for_each(std::execution::par,
            deleted_words.begin(), deleted_words.end(),
            [document_id, this](const auto& word) {
        this->word_to_document_freqs_.at(*word).erase(document_id);
        // удалить запись, если слово не присутствует в остальных документах
        //if (this->word_to_document_freqs_.at(*word).size() == 0) {
        //    this->word_to_document_freqs_.erase(*word);
        //}
        return word;
    });
    word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_id_.erase(document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        // проверка наличия минус слова в заданном документе
        if (word_to_document_freqs_.at(std::string(word)).count(document_id) != 0) {
            matched_words.clear();
            return {std::vector<std::string_view>{}, documents_.at(document_id).status};;
        }
    }
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
        std::execution::sequenced_policy seq_policy,
        const std::string_view& raw_query,
        int document_id)
const {
    return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
        std::execution::parallel_policy par_policy,
        const std::string_view& raw_query,
        int document_id)
const {
    QueryPar query = ParseQueryPar(raw_query);
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
            [document_id, this](const std::string_view word) {
        if (!word_to_document_freqs_.count(std::string(word))) {
            return false;
        }
        if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            return true;
        }
        return false;
    })) {
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    std::transform(std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(),
            [document_id, this](std::string_view& word) {
        if (word_to_document_freqs_.count(std::string(word))) {
            if (word_to_document_freqs_.at(std::string(word)).count(document_id)) {
            	return word;
            }
        }
        return std::string_view{""};
    });
    std::sort(matched_words.begin(), matched_words.end());
    auto it = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(it, matched_words.end());
    if (*matched_words.begin() == ""s) {
        matched_words.erase(matched_words.begin());
    }
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(std::string(word)) > 0;
}

bool SearchServer::IdIsExists(int new_id) {
    if (document_id_.count(new_id) > 0) {
        return true;
    }
    return false;
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string_view> words;
    for (const std::string_view& word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if ((text[0] == '-') && (text[1] == '-')) {
         throw std::invalid_argument("не более одного минуса перед словом"s);
    }
    if ((text[0] == '-') && (text.size() == 1u)) {
        throw std::invalid_argument("не допустим отдельный минус"s);
    }
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return QueryWord{
        text,
        is_minus,
        IsStopWord(text)
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query query;
    for (const std::string_view& word : SplitIntoWords(text)) {
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

SearchServer::QueryPar SearchServer::ParseQueryPar(const std::string_view& text) const {
    QueryPar query;
    for (const std::string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid char (with codes from 0 to 31)"s);
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    return log(documents_.size() * 1.0 / word_to_document_freqs_.at(std::string(word)).size());
}
