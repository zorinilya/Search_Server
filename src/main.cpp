#include "log_duration.h"
#include "process_queries.h"
#include "search_server.h"
#include "test_example_functions.h"

#include <iostream>
#include <random>
#include <string>
#include <vector>


int main() {
    //TestRemoveDuplicates();
    //TestRequest();
    //TestGetDocumentCount();
    //TestProcessQueries();
    TestRemoveDocumentParalley();
    TestMatchDocumentParalley();
}
