#include "SubtlexImporter.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

// | Column               | Meaning                                                                                          |
// | ---------------------| ------------------------------------------------------------------------------------------------ |
// | FREQcount            | Raw count of how many times the word occurs in the subtitle corpus (token frequency).            |
// | CDcount              | Number of *Contextual Diversity* counts — i.e., the number of different subtitles (documents)
//                          in which the word appears at least once. This measures how widely the word occurs, not just how
//                          often.                                                                                           |
// | FREQlow              | Frequency count restricted to lowercase forms of the word (to avoid conflating proper nouns or
//                          sentence-initial capitalization).                                                                |
// | Cdlow                | Contextual diversity count for lowercase forms only.                                             |
// | SUBTLWF              | Frequency per million words (i.e., FREQcount normalized to a 1M-word scale). Often called
//                          “word frequency per million.”                                                                    |
// | Lg10WF               | Base-10 logarithm of the word frequency (log10 of SUBTLWF + 1) — smooths out large frequency
//                          differences.                                                                                     |
// | SUBTLCD              | Contextual diversity per million subtitles — CDcount normalized to 1 M documents.                |
// | Lg10CD               | Base-10 logarithm of the contextual diversity (log10 of SUBTLCD + 1).                            |
// | Dom_PoS_SUBTLEX      | The dominant part-of-speech tag for that word (e.g., noun, verb, adjective) as it most
//                          frequently appears in the corpus.                                                                |
// | Freq_dom_PoS_SUBTLEX | Frequency count for that dominant part-of-speech specifically.                                   |
// | Percentage_dom_PoS   | Proportion (in %) of the word’s total occurrences that belong to the dominant part-of-speech.    |
// | All_PoS_SUBTLEX      | List of all parts of speech that the word can take (comma-separated).                            |
// | All_freqs_SUBTLEX    | Corresponding list of frequencies for each part-of-speech listed in *All_PoS_SUBTLEX*.           |
// | Zipf-value           | A logarithmic frequency measure standardized to a Zipf scale:                                    |

namespace
{

enum class ColumnType
{
    Int,
    Double,
    String
};

std::unordered_map<std::string_view, ColumnType> const columnTypes = {{"Word", ColumnType::String},
                                                                      {"FREQcount", ColumnType::Int},
                                                                      {"CDcount", ColumnType::Int},
                                                                      {"FREQlow", ColumnType::Int},
                                                                      {"Cdlow", ColumnType::Int},
                                                                      {"SUBTLWF", ColumnType::Double},
                                                                      {"Lg10WF", ColumnType::Double},
                                                                      {"SUBTLCD", ColumnType::Double},
                                                                      {"Lg10CD", ColumnType::Double},
                                                                      {"Dom_PoS_SUBTLEX", ColumnType::String},
                                                                      {"Freq_dom_PoS_SUBTLEX", ColumnType::Int},
                                                                      {"Percentage_dom_PoS", ColumnType::Double},
                                                                      {"All_PoS_SUBTLEX", ColumnType::String},
                                                                      {"All_freqs_SUBTLEX", ColumnType::String},
                                                                      {"Zipf-value", ColumnType::Double}};

std::vector<std::string> splitCSVLine(std::string_view line);
void                     validateColumnNames(std::vector<std::string> const & names);

} // anonymous namespace

//! @param  path    Path to the CSV file to import.
//!
//! @throws std::runtime_error if the file cannot be opened, parsed, or if the columns are invalid (missing, unexpected, or
//!         duplicated), or if there are duplicate words in the data.
SubtlexImporter::SubtlexImporter(std::string_view path)
{
    // Open the file
    std::ifstream input{std::string(path)};
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open file: " + std::string(path));
    }

    // Parse the header line to get column names
    std::string headerLine;
    if (!std::getline(input, headerLine))
    {
        throw std::runtime_error("File is empty: " + std::string(path));
    }
    std::vector<std::string> columnNames = splitCSVLine(headerLine);

    // Validate columns
    validateColumnNames(columnNames);

    // Map column names to their indices. The order of names in columnNames determines the index.
    for (size_t i = 0; i < columnNames.size(); ++i)
    {
        columnIndices[columnNames[i]] = i;
    }

    // Find the index of the Word column for duplicate checking
    size_t wordIdx = columnIndices.at("Word");

    // Track words we've seen to detect duplicates
    std::unordered_set<std::string> seenWords;

    // Load the data
    std::string line;
    while (std::getline(input, line))
    {
        auto rawRow = splitCSVLine(line);
        if (rawRow.size() != columnNames.size())
        {
            throw std::runtime_error("Row has incorrect number of columns.");
        }

        // Force the word to lowercase
        std::transform(rawRow[wordIdx].begin(),
                       rawRow[wordIdx].end(),
                       rawRow[wordIdx].begin(),
                       [](char x) { return static_cast<unsigned char>(::tolower(x)); });

        std::string const & word = rawRow[wordIdx];

        // Ensure that the word is not empty and does not contain non-alphabetic characters
        if (word.empty() || !std::all_of(word.begin(), word.end(), [](char c) { return std::isalpha(c); }))
        {
            throw std::runtime_error("Invalid word in data: " + word);
        }

        // Check for duplicate word
        if (seenWords.find(word) != seenWords.end())
        {
            throw std::runtime_error("Duplicate word in data: " + word);
        }
        seenWords.insert(word);

        // Parse each value in the row according to its column type
        std::vector<Value> typedRow;
        typedRow.reserve(rawRow.size());
        for (size_t i = 0; i < rawRow.size(); ++i)
        {
            typedRow.push_back(parseValue(rawRow[i], columnNames[i]));
        }

        // Store the row in the table
        table.emplace_back(std::move(typedRow));
    }
}

//! @param  columnName  Name of the column to retrieve values for.
//!
//! @return std::unordered_map<std::string, Value>  Map of (word, value) pairs, or empty if the column name is invalid.
std::unordered_map<std::string, SubtlexImporter::Value> SubtlexImporter::get(std::string_view columnName) const
{
    std::unordered_map<std::string, Value> result;

    // Get the index of the requested column.
    auto it = columnIndices.find(std::string(columnName));

    // Build the map if the column exists. Otherwise return empty map.
    if (it != columnIndices.end())
    {
        assert(columnIndices.find("Word") != columnIndices.end());
        size_t wordIdx = columnIndices.at("Word");
        size_t colIdx  = it->second;
        for (auto const & row : table)
        {
            result.emplace(std::get<std::string>(row[wordIdx]), row[colIdx]);
        }
    }

    return result;
}

SubtlexImporter::Value SubtlexImporter::parseValue(std::string_view value, std::string_view columnName) const
{
    auto it = columnTypes.find(columnName);
    if (it == columnTypes.end())
    {
        throw std::runtime_error("Unknown column type: " + std::string(columnName));
    }

    try
    {
        switch (it->second)
        {
        case ColumnType::Int:
            return std::stoi(std::string(value));
        case ColumnType::Double:
            return std::stod(std::string(value));
        case ColumnType::String:
            return std::string(value);
        }
    }
    catch (std::exception const & e)
    {
        throw std::runtime_error("Failed to parse value '" + std::string(value) + "' for column '" + std::string(columnName) +
                                 "': " + e.what());
    }

    return std::string(value); // Should never reach here
}

namespace
{

std::vector<std::string> splitCSVLine(std::string_view line)
{
    std::vector<std::string> result;
    std::istringstream       ss{std::string(line)};
    std::string              field;
    while (std::getline(ss, field, ','))
    {
        result.push_back(field);
    }
    return result;
}

void validateColumnNames(std::vector<std::string> const & names)
{
    if (names.size() != columnTypes.size())
    {
        throw std::runtime_error("CSV header has incorrect number of columns.");
    }
    std::unordered_set<std::string_view> nameSet(names.begin(), names.end());
    for (auto const & [key, _] : columnTypes)
    {
        if (nameSet.find(key) == nameSet.end())
        {
            throw std::runtime_error("Missing required column: " + std::string(key));
        }
    }
    for (auto const & name : names)
    {
        if (columnTypes.find(name) == columnTypes.end())
        {
            throw std::runtime_error("Unexpected column in CSV: " + name);
        }
    }
}

} // anonymous namespace
