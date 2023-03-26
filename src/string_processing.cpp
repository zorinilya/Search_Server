#include "string_processing.h"

#include <algorithm>
#include <iostream>


std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    int64_t pos = text.find_first_not_of(" ");
    const int64_t pos_end = text.npos;
    while (pos != pos_end) {
        int64_t space = text.find(' ', pos);
        words.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));

        for(const auto& item : words) {
        	std::cout << item << "_";
        }
        std::cout << "- this is SplitIntoWords" << std::endl;

        pos = text.find_first_not_of(" ", space);
    }

    return words;
}
