#include "log_duration.h"
#include "request_queue.h"
#include "paginator.h"

#include <stdexcept>

using namespace std;


void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
	LOG_DURATION_STREAM("Operation time", std::cerr);
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
	LOG_DURATION_STREAM("Operation time", std::cerr);
    try {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}

int main() {
//    TestSearchServer();

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::BANNED, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::REMOVED, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::IRRELEVANT, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::IRRELEVANT, {1, 1, 1});
    search_server.AddDocument(6, "big sparrow"s, DocumentStatus::ACTUAL, {5, 3, 6});

    {
		std::cout << "Запрос 1: "s << std::endl;
		auto results = search_server.FindTopDocuments("big white dog");
		for (const auto item : results) {
			std::cout << item << "\n";
		}
    }
    {
		std::cout << "Запрос 2: "s << std::endl;
		auto results = search_server.FindTopDocuments("big white dog", DocumentStatus::IRRELEVANT);
		for (const auto item : results) {
			std::cout << item << "\n";
		}
    }
    {
		std::cout << "Запрос 3: "s << std::endl;
		auto results = search_server.FindTopDocuments("big white dog fancy collar");
		for (const auto item : results) {
			std::cout << item << "\n";
		}
    }
    {
		std::cout << "Запрос 4: "s << std::endl;
		auto results = search_server.FindTopDocuments("big cat white dog fancy collar",
				[](int document_id, DocumentStatus status, int rating) {
			return (document_id > 2)
					&& ((status == DocumentStatus::IRRELEVANT) || (status == DocumentStatus::ACTUAL))
					&& (rating > 1);
		});
		for (const auto item : results) {
			std::cout << item << "\n";
		}
    }

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    std::cout << "After For total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    std::cout << "After \"curly dog\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    std::cout << "After \"big collar\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // ещё один запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "After \"sparrow\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    {
    	LOG_DURATION_STREAM("Operation__time", std::cout);
    	MatchDocuments(search_server, "big white dog");
    }
    {
    	LOG_DURATION_STREAM("Operation_time", std::cout);
    	for (int i = 0; i < 1; ++i) {
    		FindTopDocuments(search_server, "big white dog");
    	}
    }
    return 0;
}
