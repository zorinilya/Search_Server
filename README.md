# SearchServer
## Описание
Поисковый сервер предназначен для поиска текстовых документов по ключевым словам. При создании сервера указываются стоп-слова, которые не учитываются при поиске. У каждого документа на сервере есть параметры: __id, статус и рейтинг__. Пользователь имеет возможность добавлять на сервер документы и осуществлять поисковые запросы, используя параметры докуменов для фильтрации результатов поиска. Поисковые запросы формируют очередь, а результаты разбиваются на отдельные страницы.

## Функционал
- запрос как отдельными словами, так и с целыми предложениями;
- использование минус-слов;
- фильтрация результатов поиска;
- ранжирование результатов по __TF-IDF__;
- разбиение результатов на отдельные страницы;
- хранение поступивших запросов;
- измерение времени выполнения отдельных операций с помощью встроенного профилировщика.

## Команды 
### Добавление документа на сервер
Для добавления документа на сервер необходимо передать сами текстовые данные, а также указать необходимые параметры документа: id, статус (_ACTUAL, IRRELEVANT, BANNED, REMOVED_) и набор оценок пользователь (из них будет подсчитан средний рейтинг документа).
```C++
SearchServer search_server("and in at"s);   // "and", "in", "at" - стоп-слова

search_server.AddDocument(1, "curly cat curly tail"s DocumentStatus::ACTUAL, {7, 2, 7, 10, 5});
search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::IRRELEVANT, {1, 2, 3});
search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::BANNED, {1, 2, 8, 4});
search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::REMOVED, {1, 3, 2});
```

### Поисковый запрос
Запрос представляет собой обычную строку с указанием (при необходимости) минус-слов, которые исключают документ из результатов поиска (при наличии их в этом документе). Также можно указать необходимый статус документов (по умолчанию статус _ACTUAL_) или использовать предикат для фильтрации результатов.
#### Минус-слова в запросе
Чтобы исключить из результатов поиска документы, содержащие определённые слова, в запросе нужно указать эти слова в формате _-слово_.
```C++
SearchServer search_server("and in at"s);
RequestQueue request_queue(search_server);

search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

FindTopDocuments(search_server, "big white dog");
FindTopDocuments(search_server, "big white dog -collar");
```
Вывод:
Второй и третий документ не попали в результат поиска, т.к. содержат слово _collar_.

    Результаты поиска по запросу: big white dog
    { document_id = 4, relevance = 0.255413, rating = 2 }
    { document_id = 5, relevance = 0.255413, rating = 1 }
    { document_id = 3, relevance = 0.127706, rating = 3 }
    { document_id = 2, relevance = 0.127706, rating = 2 }
    Результаты поиска по запросу: big white dog -collar
    { document_id = 4, relevance = 0.255413, rating = 2 }
    { document_id = 5, relevance = 0.255413, rating = 1 }

#### Фильтрация
По умолчанию в результатах поиска появляются документы только со статусом _ACTUAL_, но в запросе можно указать необходимый статус документа. Также в запросе можно использовать свой предикат.
```C++
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
```
Вывод:
- документы 3, 4 и 5 не попали в результат поиска, т.к. имеют статус не _ACTUAL_.

        Запрос 1:
        { document_id = 6, relevance = 0.202733, rating = 4 }
        { document_id = 2, relevance = 0.173287, rating = 2 }

- документы 2, 3 и 6 не попали в результат поиска, т.к. имеют статус не _IRRELEVANT_.

        Запрос 2:
        { document_id = 4, relevance = 0.274653, rating = 2 }
        { document_id = 5, relevance = 0.274653, rating = 1 }

- документы 3, 4 и 5 не попали в результат поиска, т.к. имеют статус не _ACTUAL_. Документ 2 имеет большую релевантность, чем документ 6, поэтому он выше в результатах поиска. 

        Запрос 3:
        { document_id = 2, relevance = 0.722593, rating = 2 }
        { document_id = 6, relevance = 0.202733, rating = 4 }

- все документы содержат слова из запроса, однако документы 1 и 2 не попали в результаты, потому что их _id_ меньше 2, документ 3 - потому что имеет статус не _ACTUAL_ или _IRRELEVANT_, а документ 5 - потому что имеет рейтинг не больше 1.

        Запрос 4:
        { document_id = 4, relevance = 0.274653, rating = 2 }
        { document_id = 6, relevance = 0.202733, rating = 4 }

#### Очередь запросов
Размер очереди задаётся в классе _RequestQueue_. В качестве примера задана частота запросов, равная одному запросу в минуту или 1440 запросам в сутки. Сохраняются самые актуальные запросы: запросы за последние сутки. 
```C++
class RequestQueue {
public:
...
private:
...
    const static int min_in_day_ = 1440;    // размер очереди запросов 
...
}
...
int main() {
    SearchServer search_server("and in at"s);   // "and", "in", "at" - стоп-слова
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s DocumentStatus::ACTUAL, {7, 2, 7, 10, 5});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8, 4});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    cout << "After For total empty requests: "s << request_queue.GetNoResultRequests() << endl;

    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    cout << "After \"curly dog\" Total empty requests: "s << request_queue.GetNoResultRequests() << endl;

    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    cout << "After \"big collar\" Total empty requests: "s << request_queue.GetNoResultRequests() << endl;

    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "After \"sparrow\" Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
}
```
Вывод:

    After For total empty requests: 1439
    After "curly dog" Total empty requests: 1439
    After "big collar" Total empty requests: 1438
    After "sparrow" Total empty requests: 1437

#### Разбиение результатов на страницы
Разбиение результатов поиска на страницы осуществляется с помощью функции _Paginate_, которая принимает в качестве параметров контейнер с результатами поиска и размер страницы и которая возвращает экземпляр класса _Paginator_, содержащего диапазоны страниц.
```C++
SearchServer search_server("and with"s);

search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

const auto search_results = search_server.FindTopDocuments("curly dog and funny cat"s);
int page_size = 2;  // количество документов на странице
const auto pages = Paginate(search_results, page_size);

// Выводим найденные документы по страницам
for (auto page = pages.begin(); page != pages.end(); ++page) {
    cout << *page << endl;
    cout << "Page break"s << endl;
}
```

Вывод:

    { document_id = 2, relevance = 0.631432, rating = 2 }{ document_id = 4, relevance = 0.458145, rating = 2 }
    Page break
    { document_id = 1, relevance = 0.229073, rating = 5 }{ document_id = 3, relevance = 0.229073, rating = 3 }
    Page break
    { document_id = 5, relevance = 0.229073, rating = 1 }
    Page break