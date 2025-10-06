#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

//! Abstract base class for importing word dataset files.
//!
//! DatasetImporter provides a common interface for loading and querying word datasets where each row contains a word and
//! associated data values of various types.
class DatasetImporter
{
public:
    //! Variant type for column values (int, double, or string).
    using Value = std::variant<int, double, std::string>;

    //! Virtual destructor for proper cleanup of derived classes.
    virtual ~DatasetImporter() = default;

    //! Returns the value for each word in the specified column.
    //!
    //! @param columnName Name of the column to retrieve values for.
    //!
    //! @return Map of (word, column value) pairs. Returns empty map if the column name is invalid.
    virtual std::unordered_map<std::string, Value> get(std::string_view columnName) const = 0;

protected:
    //! Protected constructor - only derived classes can instantiate.
    DatasetImporter() = default;

    //! Prevent copying.
    DatasetImporter(DatasetImporter const &)             = delete;
    DatasetImporter & operator=(DatasetImporter const &) = delete;

    //! Allow moving.
    DatasetImporter(DatasetImporter &&)             = default;
    DatasetImporter & operator=(DatasetImporter &&) = default;
};
