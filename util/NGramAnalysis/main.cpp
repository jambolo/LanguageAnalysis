// ngram_analyzer
//
// A C++ program to perform N - gram analysis on a dictionary.

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

typedef std::unordered_map<std::string, int64_t>                   NGramMap;
typedef std::vector<std::pair<std::string_view, std::string_view>> ReplacementList;

namespace
{
// Vowels in order of frequency in English, 'Y' and 'W' represent 'y' and 'w' as vowels
std::string_view const VOWELS = "eaoiuWY";
// Consonants in order of frequency in English, 'Q' represents 'qu'
std::string_view const CONSONANTS = "tnshrdlcmpgfybwvkxjqzQ";
// Replace certain character sequences with special characters for analysis
std::string replaceSpecialSequences(std::string const & input);
} // anonymous namespace

int main(int argc, char ** argv)
{
    CLI::App    app{"Dictionary Analyzer"};
    std::string path;
    int         top_k       = 10;
    bool        output_json = false;

    app.add_option("path", path, "Dictionary path")->required();
    app.add_option("-k", top_k, "Top K N-grams to display")->check(CLI::Range(1, 100));
    app.add_flag("--json", output_json, "Output results in JSON format");
    CLI11_PARSE(app, argc, argv);

    std::ifstream infile(path);
    if (!infile.is_open())
    {
        std::cerr << "Error opening file: " << path << std::endl;
        return 1;
    }

    // Load words and count N-grams
    std::vector<NGramMap> ngramMaps;
    std::vector<int64_t>  totals;
    std::string           word;
    int64_t               weight;
    int                   wordCount = 0;

    while (infile >> word >> weight)
    {
        size_t wordLength = word.length();
        if (ngramMaps.size() < wordLength + 1)
        {
            ngramMaps.resize(wordLength + 1);
            totals.resize(wordLength + 1, 0);
        }

        std::string_view wordView = word;
        for (size_t n = 1; n <= wordLength; ++n)
        {
            for (size_t i = 0; i <= wordLength - n; ++i)
            {
                std::string ngram = std::string(wordView.substr(i, n));

                // Special handling for certain sequences
                ngram = replaceSpecialSequences(ngram);

                // Count the n-gram
                size_t length = ngram.size();
                auto & counts = ngramMaps[length];
                counts[std::string(ngram)] += weight;
                totals[length] += weight;
            }
        }
        ++wordCount;
        if (wordCount % 10000 == 0)
        {
            std::cerr << "Processed " << wordCount << " words...\n";
        }
    }

    // If reading the file failed before reaching EOF
    if (infile.bad())
    {
        std::cerr << "Error reading file: " << path << std::endl;
        return 1;
    }

    infile.close();

    // Extract the counts for consonant-only and vowel-only n-grams
    NGramMap consonantNgrams;
    NGramMap vowelNgrams;
    int64_t  totalVowelNgrams     = 0;
    int64_t  totalConsonantNgrams = 0;
    for (auto const & ngram_map : ngramMaps)
    {
        for (auto const & [ngram, count] : ngram_map)
        {
            // If every characters in ngram is in VOWELS, it's a vowel n-gram
            if (ngram.find_first_not_of(VOWELS) == std::string_view::npos)
            {
                vowelNgrams[ngram] = count;
                totalVowelNgrams += count;
            }
            else if (ngram.find_first_not_of(CONSONANTS) == std::string_view::npos)
            {
                consonantNgrams[ngram] = count;
                totalConsonantNgrams += count;
            }
        }
    }

    if (output_json)
    {
        json j;
        j["ngrams"]     = ngramMaps;
        j["vowels"]     = vowelNgrams;
        j["consonants"] = consonantNgrams;
        std::cout << j.dump(2) << "\n";
    }
    else
    {
        std::cout << "Total words processed: " << wordCount << "\n";

        // Display results for each N
        int i = 0;
        for (auto const & ngram_map : ngramMaps)
        {
            if (ngram_map.empty())
            {
                ++i;
                continue;
            }

            // Convert map to vector and sort by occurrences
            std::vector<std::pair<std::string, int64_t>> ngram_vector;
            for (auto const & [ngram, count] : ngram_map)
            {
                ngram_vector.emplace_back(ngram, count);
            }
            std::sort(ngram_vector.begin(),
                      ngram_vector.end(),
                      [](auto const & a, auto const & b)
                      {
                          return b.second < a.second; // Sort in descending order
                      });

            std::cout << "Total " << i << "-grams counted: " << ngram_vector.size() << "\n";

            // Display top K N-grams
            std::cout << "Top " << top_k << " " << i << "-grams:\n";
            for (int j = 0; j < std::min(top_k, static_cast<int>(ngram_vector.size())); ++j)
            {
                double p = static_cast<double>(ngram_vector[j].second) / totals[i];
                std::cout << ngram_vector[j].first << ": " << ngram_vector[j].second << " (" << p * 100 << "%)" << "\n";
            }
            std::cout << "\n";
            ++i;
        }

        // Display the total number of n-grams processed
        int64_t total_ngrams = std::accumulate(totals.begin(), totals.end(), int64_t(0));
        std::cout << "Total n-grams processed: " << total_ngrams << "\n";
    }
    return 0;
}

namespace
{

std::string replaceSpecialSequences(std::string const & input)
{
    std::string_view inputView = input;

    std::string result;
    result.reserve(input.size()); // Reserve enough space

    size_t i = 0;
    while (i < input.size())
    {
        char c0 = input[i++];

        // It's the next character that determines what to do, so always push the current character
        result.push_back(c0);

        // The replaced sequences are all two characters long, so only check if there's a next character
        if (i < input.size())
        {
            char c1 = input[i];
            // Replace 'y' preceded by certain vowels with 'Y'
            if (c1 == 'y' && (c0 == 'a' || c0 == 'e' || c0 == 'o' || c0 == 'u'))
            {
                result.push_back('Y');
                ++i; // Eat two characters
            }
            // Replace 'w' preceded by certain vowels with 'W'
            else if (c1 == 'w' && (c0 == 'a' || c0 == 'e' || c0 == 'o'))
            {
                result.push_back('W');
                ++i; // Eat two characters
            }
            // Replace 'y' preceded by a consonant with 'Y'
            else if (c1 == 'y' && CONSONANTS.find(c0) != std::string_view::npos)
            {
                result.push_back('Y');
                ++i; // Eat two characters
            }
            // Replace "qu" with 'Q'
            else if (c0 == 'q' && c1 == 'u')
            {
                result.back() = 'Q'; // Replace 'q' that was already pushed with 'Q'. Forget the 'u'.
                ++i;                 // Eat two characters
            }
            else
            {
                // No replacement
            }
        }
    }

    return result;
}

} // anonymous namespace
