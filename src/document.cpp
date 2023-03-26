#include "document.h"

#include <iostream>

using namespace std::string_literals;


Document::Document()
    : id(0)
    , relevance(0.0)
    , rating(0) {
}

Document::Document(int doc_id, double doc_relevance, int doc_rating)
    : id(doc_id)
    , relevance(doc_relevance)
    , rating(doc_rating) {
}

std::ostream& operator<<(std::ostream& output, const Document& document) {
    output << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s;
    return output;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string_view& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}
