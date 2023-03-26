#include "remove_duplicates.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>


void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> document_for_delete;
    std::set<std::set<std::string_view>> document_words;
    for (const int document_id : search_server) {
        std::set<std::string_view> words;
        for (const auto [word, freq] : search_server.GetWordFrequencies(document_id)) {
            words.insert(word);
        }
        if (document_words.find(words) == document_words.end()) {
            document_words.insert(words);
        }
        else {
            document_for_delete.push_back(document_id);
        }
    }
    for (const int id : document_for_delete) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id " << id << std::endl;
    }
}
