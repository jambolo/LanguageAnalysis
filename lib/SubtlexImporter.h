#pragma once

#include "DatasetImporter.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

//! Imports and parses a SUBTLEX CSV file, providing typed access to its columns.
//!
//! The SubtlexImporter class loads a CSV file containing SUBTLEX data, validates the header for required columns, and parses
//! each row into typed values (int, double, or string) according to the column definitions.
//!
//! Example usage:
//! @code
//! SubtlexImporter importer("subtlex.csv");
//! auto freqMap = importer.get("FREQcount");
//! for (const auto& [word, value] : freqMap) {
//!     // word is std::string, value is int/double/string (std::variant)
//! }
//! @endcode
class SubtlexImporter : public DatasetImporter
{
public:
    //! Constructs a SubtlexImporter and loads the specified CSV file.
    SubtlexImporter(std::string_view filename);

    //! Returns the value for each word in the specified column.
    std::unordered_map<std::string, Value> get(std::string_view columnName) const override;

private:
    Value parseValue(std::string_view value, std::string_view columnName) const;

    std::unordered_map<std::string, size_t> columnIndices; // Maps column names (views into columnNames) to their indices
    std::vector<std::vector<Value>>         table;         // Table of parsed CSV data
};
