// ngram_analyzer
//
// A C++ program to perform N - gram analysis on a dictionary.

#include <CLI/CLI.hpp>
#include <SubtlexImporter.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

typedef std::unordered_map<std::string, double>                    NGramMap;
typedef std::vector<std::pair<std::string_view, std::string_view>> ReplacementList;

namespace
{

// Vowels in order of frequency in English, 'Y' and 'W' represent 'y' and 'w' as vowels
std::string_view const VOWELS = "eoaiuYW";
// Consonants in order of frequency in English, 'Q' represents 'qu'
std::string_view const CONSONANTS = "tnhsrldymwgcfbpkvjxzqQ";
// Replace certain character sequences with special characters for analysis
std::string replaceSpecialSequences(std::string const & input);

} // anonymous namespace

int main(int argc, char ** argv)
{
    CLI::App    app{"Dictionary Analyzer"};
    int         top_k       = 10;
    bool        output_json = false;
    std::string subtlex_path;

    app.add_option("-k", top_k, "Top K N-grams to display")->check(CLI::Range(1, 100));
    app.add_flag("--json", output_json, "Output results in JSON format");
    app.add_option("--subtlex", subtlex_path, "Path to SUBTLEX CSV file to load")->required();
    CLI11_PARSE(app, argc, argv);

    std::unordered_map<std::string, DatasetImporter::Value> frequencies;
    try
    {
        SubtlexImporter subtlex(subtlex_path);
        std::cerr << "Loaded SUBTLEX file: " << subtlex_path << "\n";
        frequencies = subtlex.get("SUBTLWF"); // Get word frequencies (per million)
        std::cerr << "SUBTLEX words loaded: " << frequencies.size() << "\n";
    }
    catch (std::exception const & e)
    {
        std::cerr << "Error loading SUBTLEX file: " << e.what() << std::endl;
        return 1;
    }

    // Load words and count N-grams
    std::vector<NGramMap> ngramMaps;
    std::vector<double>   totalWeights;
    int                   wordCount = 0;

    for (auto const & [word, value] : frequencies)
    {
        // Extend the data if necessary to accommodate a word of this length.
        size_t wordLength = word.length();
        if (ngramMaps.size() < wordLength + 1)
        {
            ngramMaps.resize(wordLength + 1);
            totalWeights.resize(wordLength + 1, 0);
        }

        // For each possible n-gram in the word, accumulate its count/frequency/weight.
        double           weight   = std::get<double>(value); // The weight of an n-gram is the frequency of the word containing it.
        std::string_view wordView = word;
        for (size_t n = 1; n <= wordLength; ++n)
        {
            for (size_t i = 0; i <= wordLength - n; ++i)
            {
                std::string ngram = std::string(wordView.substr(i, n));

                // Special handling for certain sequences
                ngram = replaceSpecialSequences(ngram);

                // Count the n-gram
                size_t ngramSize = ngram.size();
                auto & counts    = ngramMaps[ngramSize];
                counts[std::string(ngram)] += weight;
                totalWeights[ngramSize] += weight;
            }
        }
        ++wordCount;
        if (wordCount % 10000 == 0)
        {
            std::cerr << "Processed " << wordCount << " words...\n";
        }
    }

    // Extract the counts for consonant-only and vowel-only n-grams
    NGramMap consonantNgrams;
    NGramMap vowelNgrams;
    double   totalVowelNgrams     = 0.0;
    double   totalConsonantNgrams = 0.0;
    for (auto const & ngram_map : ngramMaps)
    {
        for (auto const & [ngram, weight] : ngram_map)
        {
            // If every characters in ngram is in VOWELS, it's a vowel n-gram
            if (ngram.find_first_not_of(VOWELS) == std::string_view::npos)
            {
                vowelNgrams[ngram] = weight;
                totalVowelNgrams += weight;
            }
            else if (ngram.find_first_not_of(CONSONANTS) == std::string_view::npos)
            {
                consonantNgrams[ngram] = weight;
                totalConsonantNgrams += weight;
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
        int ngramSize = 0;
        for (auto const & ngram_map : ngramMaps)
        {
            if (ngram_map.empty())
            {
                ++ngramSize;
                continue;
            }

            // Convert map to vector and sort by weight
            std::vector<std::pair<std::string, double>> ngram_vector;
            for (auto const & [ngram, weight] : ngram_map)
            {
                ngram_vector.emplace_back(ngram, weight);
            }
            std::sort(ngram_vector.begin(),
                      ngram_vector.end(),
                      [](auto const & a, auto const & b)
                      {
                          return b.second < a.second; // Sort in descending order
                      });

            std::cout << "Total " << ngramSize << "-grams counted: " << ngram_vector.size() << "\n";

            // Display top K N-grams
            std::cout << "Top " << top_k << " " << ngramSize << "-grams:\n";
            for (int j = 0; j < std::min(top_k, static_cast<int>(ngram_vector.size())); ++j)
            {
                double p = ngram_vector[j].second / totalWeights[ngramSize];
                std::cout << ngram_vector[j].first << ": " << ngram_vector[j].second << " (" << p * 100 << "%)" << "\n";
            }
            std::cout << "\n";
            ++ngramSize;
        }

        // Display the total weight of n-grams processed
        double total_ngrams = std::accumulate(totalWeights.begin(), totalWeights.end(), 0.0);
        std::cout << "Total weight of n-grams processed: " << total_ngrams << "\n";
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
            // Replace 'y' preceded by certain vowels or any consonant with 'Y'
            if (c1 == 'y')
            {
                if (c0 == 'a' || c0 == 'e' || c0 == 'o' || c0 == 'u' || CONSONANTS.find(c0) != std::string_view::npos)
                {
                    result.push_back('Y');
                    ++i; // Eat two characters
                }
            }
            // Replace 'w' preceded by certain vowels with 'W'
            else if (c1 == 'w')
            {
                if (c0 == 'a' || c0 == 'e' || c0 == 'o')
                {
                    result.push_back('W');
                    ++i; // Eat two characters
                }
            }

            // Replace "qu" with 'Q'
            if (c0 == 'q')
            {
                if (c1 == 'u')
                {
                    result.back() = 'Q'; // Replace 'q' that was already pushed with 'Q'. Forget the 'u'.
                    ++i;                 // Eat two characters
                }
            }
        }
    }

    return result;
}

} // anonymous namespace
