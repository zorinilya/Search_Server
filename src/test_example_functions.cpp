/*
 * ��� ������������ �� ��������� ���������� �������� ������ #include "test_example_functions.h"
 */

#include "log_duration.h"
#include "process_queries.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace std::string_literals;


void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "������ ���������� ��������� "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "���������� ������ �� �������: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "������ ������: "s << e.what() << std::endl;
    }
}

void MatchDocuments(SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "������� ���������� �� �������: "s << query << std::endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "������ �������� ���������� �� ������ "s << query << ": "s << e.what() << std::endl;
    }
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // �������� ��������� 2, ����� �����
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // ������� ������ � ����-������, ������� ����������
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // ��������� ���� ����� ��, ������� ���������� ��������� 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // ���������� ����� �����, ���������� �� ��������
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // ���� �� ��� �����, �� �������� ����������
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // ����� �� ������ ����������, �� �������� ����������
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    search_server.RemoveDocument(1);
    search_server.RemoveDocument(6);
    std::cout << "After document 1 and 6 removed: "s << search_server.GetDocumentCount() << std::endl;

}

void TestRequest() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::BANNED, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::REMOVED, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::IRRELEVANT, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::IRRELEVANT, {1, 1, 1});
    search_server.AddDocument(6, "big sparrow"s, DocumentStatus::ACTUAL, {5, 3, 6});

    {
        std::cout << "������ 1: "s << std::endl;
        auto results = search_server.FindTopDocuments("big white dog");
        for (const auto item : results) {
            std::cout << item << "\n";
        }
    }
    {
        std::cout << "������ 2: "s << std::endl;
        auto results = search_server.FindTopDocuments("big white dog", DocumentStatus::IRRELEVANT);
        for (const auto item : results) {
            std::cout << item << "\n";
        }
    }
    {
        std::cout << "������ 3: "s << std::endl;
        auto results = search_server.FindTopDocuments("big white dog fancy collar");
        for (const auto item : results) {
            std::cout << item << "\n";
        }
    }
    {
        std::cout << "������ 4: "s << std::endl;
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

    // 1439 �������� � ������� �����������
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    std::cout << "After For total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // ��� ��� 1439 �������� � ������� �����������
    request_queue.AddFindRequest("curly dog"s);
    std::cout << "After \"curly dog\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // ����� �����, ������ ������ ������, 1438 �������� � ������� �����������
    request_queue.AddFindRequest("big collar"s);
    std::cout << "After \"big collar\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    // ��� ���� ������ ������, 1437 �������� � ������� �����������
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "After \"sparrow\" Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    {
        LOG_DURATION_STREAM("MatchDocuments", std::cerr);
        MatchDocuments(search_server, "big white dog");
    }
    {
        LOG_DURATION_STREAM("FindTopDocuments", std::cerr);
        for (int i = 0; i < 1; ++i) {
            FindTopDocuments(search_server, "big white dog");
        }
    }

}

void TestGetDocumentCount() {
    SearchServer search_server("and"s);
    std::cerr << search_server.GetDocumentCount() << " documents\n";
}

std::string GenerateWord(std::mt19937& generator, int max_length) {
    const int length = std::uniform_int_distribution(1, max_length)(generator);
    std::string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(std::uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

std::vector<std::string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length) {
    std::vector<std::string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

std::string GenerateQuery(std::mt19937& generator, const std::vector<std::string>& dictionary, int max_word_count) {
    const int word_count = std::uniform_int_distribution(1, max_word_count)(generator);
    std::string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        query += dictionary[std::uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

std::vector<std::string> GenerateQueries(std::mt19937& generator, const std::vector<std::string>& dictionary, int query_count, int max_word_count) {
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename QueriesProcessor>
void TestProcessQueries(std::string mark, QueriesProcessor processor, const SearchServer& search_server, const std::vector<std::string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

#define TEST_PROCESS_QUERIES(processor) TestProcessQueries(#processor, processor, search_server, queries)

void TestProcessQueries() {
    std::mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 1, 50'000'000);
    const auto documents = GenerateQueries(generator, dictionary, 1, 1);
    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }
    const auto queries = GenerateQueries(generator, dictionary, 1, 7);
    TEST_PROCESS_QUERIES(ProcessQueries);
    std::cerr << "TestProcessQueries - OK\n";
}

using namespace std;

template <typename ExecutionPolicy>
void TestRemoveDocumentParalley(std::string mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    std::cout << search_server.GetDocumentCount() << " documents\n";
}

#define TEST_REMOVE_DOCUMENT_PARALLEY(mode) TestRemoveDocumentParalley(#mode, search_server, std::execution::mode)


void TestRemoveDocumentParalley() {
    std::mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 20'000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);
    {
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }

        TEST_REMOVE_DOCUMENT_PARALLEY(seq);
    }
    {
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }

        TEST_REMOVE_DOCUMENT_PARALLEY(par);
    }
}
