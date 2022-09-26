#include "read_input_functions.h"


std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<int> ReadIntVector() {
    int amount;
    std::cin >> amount;
    std::vector<int> numbers(amount, 0);
    for (int& number : numbers) {
        std::cin >> number;
    }
    ReadLine();
    return numbers;
}
