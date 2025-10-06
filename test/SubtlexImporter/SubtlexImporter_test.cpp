#include <SubtlexImporter.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace fs = std::filesystem;

// Helper class to create temporary CSV files for testing
class TempCSVFile
{
public:
    TempCSVFile(std::string const & content)
        : path_(fs::temp_directory_path() / ("test_subtlex_" + std::to_string(++counter_) + ".csv"))
    {
        std::ofstream file(path_);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to create temp file: " + path_.string());
        }
        file << content;
        file.close();
    }

    ~TempCSVFile()
    {
        if (fs::exists(path_))
        {
            fs::remove(path_);
        }
    }

    std::string path() const { return path_.string(); }

private:
    fs::path          path_;
    static inline int counter_ = 0;
};

// Test fixture for SubtlexImporter tests
class SubtlexImporterTest : public ::testing::Test
{
protected:
    // Valid CSV header with all required columns
    static std::string validHeader()
    {
        return "Word,FREQcount,CDcount,FREQlow,Cdlow,SUBTLWF,Lg10WF,SUBTLCD,Lg10CD,Dom_PoS_SUBTLEX,"
               "Freq_dom_PoS_SUBTLEX,Percentage_dom_PoS,All_PoS_SUBTLEX,All_freqs_SUBTLEX,Zipf-value";
    }

    // Create a valid CSV with sample data
    static std::string validCSV()
    {
        std::ostringstream oss;
        oss << validHeader() << "\n";
        oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
        oss << "banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
        oss << "cherry,50,25,40,20,0.9,0.954,1.7,0.230,noun,45,0.9,noun,45,2.8\n";
        return oss.str();
    }
};

// ========== Constructor Tests ==========

TEST_F(SubtlexImporterTest, ConstructorWithValidFile)
{
    TempCSVFile tempFile(validCSV());
    EXPECT_NO_THROW(SubtlexImporter importer(tempFile.path()));
}

TEST_F(SubtlexImporterTest, ConstructorWithNonExistentFile)
{
    EXPECT_THROW(SubtlexImporter importer("non_existent_file.csv"), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithEmptyFile)
{
    TempCSVFile tempFile("");
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithMissingColumn)
{
    std::string csv = "Word,FREQcount,CDcount\napple,100,50\n";
    TempCSVFile tempFile(csv);
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithExtraColumn)
{
    std::string csv = validHeader() + ",ExtraColumn\napple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5,extra\n";
    TempCSVFile tempFile(csv);
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithDuplicateColumn)
{
    std::string csv = "Word,FREQcount,FREQcount,CDcount,FREQlow,Cdlow,SUBTLWF,Lg10WF,SUBTLCD,Lg10CD,Dom_PoS_SUBTLEX,"
                      "Freq_dom_PoS_SUBTLEX,Percentage_dom_PoS,All_PoS_SUBTLEX,All_freqs_SUBTLEX,Zipf-value\n";
    TempCSVFile tempFile(csv);
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithIncorrectColumnCount)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50\n"; // Too few columns
    TempCSVFile tempFile(oss.str());
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ConstructorWithReorderedColumns)
{
    // Columns in different order should still work (no order requirement)
    std::string csv = "FREQcount,Word,CDcount,FREQlow,Cdlow,SUBTLWF,Lg10WF,SUBTLCD,Lg10CD,Dom_PoS_SUBTLEX,"
                      "Freq_dom_PoS_SUBTLEX,Percentage_dom_PoS,All_PoS_SUBTLEX,All_freqs_SUBTLEX,Zipf-value\n"
                      "100,apple,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(csv);
    EXPECT_NO_THROW(SubtlexImporter importer(tempFile.path()));
}

// ========== get() Method Tests ==========

TEST_F(SubtlexImporterTest, GetValidIntegerColumn)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("FREQcount");

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<int>(result["apple"]), 100);
    EXPECT_EQ(std::get<int>(result["banana"]), 200);
    EXPECT_EQ(std::get<int>(result["cherry"]), 50);
}

TEST_F(SubtlexImporterTest, GetValidDoubleColumn)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("SUBTLWF");

    EXPECT_EQ(result.size(), 3);
    EXPECT_DOUBLE_EQ(std::get<double>(result["apple"]), 1.5);
    EXPECT_DOUBLE_EQ(std::get<double>(result["banana"]), 2.8);
    EXPECT_DOUBLE_EQ(std::get<double>(result["cherry"]), 0.9);
}

TEST_F(SubtlexImporterTest, GetValidStringColumn)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Dom_PoS_SUBTLEX");

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "noun");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "noun");
    EXPECT_EQ(std::get<std::string>(result["cherry"]), "noun");
}

TEST_F(SubtlexImporterTest, GetWordColumn)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "banana");
    EXPECT_EQ(std::get<std::string>(result["cherry"]), "cherry");
}

TEST_F(SubtlexImporterTest, GetInvalidColumn)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("NonExistentColumn");

    EXPECT_TRUE(result.empty());
}

TEST_F(SubtlexImporterTest, GetAllIntegerColumns)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    // Test all integer columns
    std::vector<std::string> intColumns = {"FREQcount", "CDcount", "FREQlow", "Cdlow", "Freq_dom_PoS_SUBTLEX"};

    for (auto const & col : intColumns)
    {
        auto result = importer.get(col);
        EXPECT_EQ(result.size(), 3) << "Column: " << col;
        EXPECT_TRUE(std::holds_alternative<int>(result["apple"])) << "Column: " << col;
    }
}

TEST_F(SubtlexImporterTest, GetAllDoubleColumns)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    // Test all double columns
    std::vector<std::string> doubleColumns = {"SUBTLWF", "Lg10WF", "SUBTLCD", "Lg10CD", "Percentage_dom_PoS", "Zipf-value"};

    for (auto const & col : doubleColumns)
    {
        auto result = importer.get(col);
        EXPECT_EQ(result.size(), 3) << "Column: " << col;
        EXPECT_TRUE(std::holds_alternative<double>(result["apple"])) << "Column: " << col;
    }
}

TEST_F(SubtlexImporterTest, GetAllStringColumns)
{
    TempCSVFile     tempFile(validCSV());
    SubtlexImporter importer(tempFile.path());

    // Test all string columns
    std::vector<std::string> stringColumns = {"Word", "Dom_PoS_SUBTLEX", "All_PoS_SUBTLEX", "All_freqs_SUBTLEX"};

    for (auto const & col : stringColumns)
    {
        auto result = importer.get(col);
        EXPECT_EQ(result.size(), 3) << "Column: " << col;
        EXPECT_TRUE(std::holds_alternative<std::string>(result["apple"])) << "Column: " << col;
    }
}

// ========== Data Type Parsing Tests ==========

TEST_F(SubtlexImporterTest, ParseInvalidIntegerValue)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,abc,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n"; // "abc" is not a valid int
    TempCSVFile tempFile(oss.str());
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ParseInvalidDoubleValue)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,invalid,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n"; // "invalid" is not a valid double
    TempCSVFile tempFile(oss.str());
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, ParseNegativeIntegerValue)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,-100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("FREQcount");
    EXPECT_EQ(std::get<int>(result["apple"]), -100);
}

TEST_F(SubtlexImporterTest, ParseNegativeDoubleValue)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,-1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("SUBTLWF");
    EXPECT_DOUBLE_EQ(std::get<double>(result["apple"]), -1.5);
}

TEST_F(SubtlexImporterTest, ParseZeroValues)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,0,0,0,0,0.0,0.0,0.0,0.0,noun,0,0.0,noun,0,0.0\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto intResult = importer.get("FREQcount");
    EXPECT_EQ(std::get<int>(intResult["apple"]), 0);

    auto doubleResult = importer.get("SUBTLWF");
    EXPECT_DOUBLE_EQ(std::get<double>(doubleResult["apple"]), 0.0);
}

TEST_F(SubtlexImporterTest, ParseEmptyStringValue)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,,90,0.9,noun,90,3.5\n"; // Empty string in Dom_PoS_SUBTLEX
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Dom_PoS_SUBTLEX");
    EXPECT_EQ(std::get<std::string>(result["apple"]), "");
}

// ========== Edge Cases and Special Scenarios ==========

TEST_F(SubtlexImporterTest, SingleRowCSV)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
}

TEST_F(SubtlexImporterTest, HeaderOnlyCSV)
{
    TempCSVFile     tempFile(validHeader());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_TRUE(result.empty());
}

TEST_F(SubtlexImporterTest, LargeDataSet)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    for (int i = 0; i < 1000; ++i)
    {
        // Generate alphabetic-only words by converting number to letters
        // Use 'a' + (i % 26) pattern to create unique words like "worda", "wordb", etc.
        std::string word = "word";
        for (auto k = i; k > 0; k /= 26)
        {
            word += char('a' + (k % 26));
        }

        oss << word << "," << i << ",50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    }
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("FREQcount");
    EXPECT_EQ(result.size(), 1000);

    // Test a specific word from the middle
    // For i=500: 500/26 = 19, 500%26 = 6
    // so append 'a' + 19 = 't', and 'a' + 6 = 'g'
    // Result: "wordgt"
    EXPECT_EQ(std::get<int>(result["wordgt"]), 500);
}

TEST_F(SubtlexImporterTest, DuplicateWordInData)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "apple,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n"; // Duplicate "apple"
    TempCSVFile tempFile(oss.str());

    // Constructor should throw an error for duplicate words
    try
    {
        SubtlexImporter importer(tempFile.path());
        FAIL() << "Expected std::runtime_error to be thrown";
    }
    catch (std::runtime_error const & e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Duplicate word") != std::string::npos) << "Error message: " << errorMsg;
        EXPECT_TRUE(errorMsg.find("apple") != std::string::npos) << "Error message should contain the duplicate word";
    }
}

TEST_F(SubtlexImporterTest, MultipleDifferentWordsNoDuplicates)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    oss << "cherry,50,25,40,20,0.9,0.954,1.7,0.230,noun,45,0.9,noun,45,2.8\n";
    TempCSVFile tempFile(oss.str());

    // Should not throw - all words are unique
    EXPECT_NO_THROW(SubtlexImporter importer(tempFile.path()));
}

TEST_F(SubtlexImporterTest, DuplicateWordLaterInFile)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    oss << "cherry,50,25,40,20,0.9,0.954,1.7,0.230,noun,45,0.9,noun,45,2.8\n";
    oss << "banana,300,100,200,80,3.5,0.544,4.0,0.602,noun,270,0.9,noun,270,5.0\n"; // Duplicate "banana"
    TempCSVFile tempFile(oss.str());

    // Constructor should throw an error for duplicate words
    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                EXPECT_TRUE(std::string(e.what()).find("banana") != std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

// ========== Lowercase Conversion Tests ==========

TEST_F(SubtlexImporterTest, UppercaseWordConvertedToLowercase)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "APPLE,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 1);
    // Word should be stored as lowercase
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    // Original uppercase key should not exist
    EXPECT_EQ(result.find("APPLE"), result.end());
}

TEST_F(SubtlexImporterTest, MixedCaseWordConvertedToLowercase)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "ApPlE,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "BaNaNa,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "banana");

    auto freqResult = importer.get("FREQcount");
    EXPECT_EQ(std::get<int>(freqResult["apple"]), 100);
    EXPECT_EQ(std::get<int>(freqResult["banana"]), 200);
}

TEST_F(SubtlexImporterTest, DifferentCasesOfSameWordAreDuplicates)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "Apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "APPLE,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n"; // Same word, different case
    TempCSVFile tempFile(oss.str());

    // Constructor should throw an error for duplicate words (after lowercase conversion)
    try
    {
        SubtlexImporter importer(tempFile.path());
        FAIL() << "Expected std::runtime_error to be thrown for case-insensitive duplicate";
    }
    catch (std::runtime_error const & e)
    {
        std::string errorMsg = e.what();
        EXPECT_TRUE(errorMsg.find("Duplicate word") != std::string::npos) << "Error message: " << errorMsg;
        EXPECT_TRUE(errorMsg.find("apple") != std::string::npos) << "Error message should contain the lowercase word";
    }
}

TEST_F(SubtlexImporterTest, AllUppercaseWordsConvertedToLowercase)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "APPLE,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "BANANA,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    oss << "CHERRY,50,25,40,20,0.9,0.954,1.7,0.230,noun,45,0.9,noun,45,2.8\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "banana");
    EXPECT_EQ(std::get<std::string>(result["cherry"]), "cherry");
}

TEST_F(SubtlexImporterTest, TitleCaseWordConvertedToLowercase)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "Apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "Banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "banana");
}

TEST_F(SubtlexImporterTest, LowercaseWordsRemainUnchanged)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    auto result = importer.get("Word");
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(std::get<std::string>(result["apple"]), "apple");
    EXPECT_EQ(std::get<std::string>(result["banana"]), "banana");
}

TEST_F(SubtlexImporterTest, CaseInsensitiveAccessToAllColumns)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "APPLE,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile     tempFile(oss.str());
    SubtlexImporter importer(tempFile.path());

    // Test that all column types work with lowercase word key
    auto freqResult = importer.get("FREQcount");
    EXPECT_EQ(std::get<int>(freqResult["apple"]), 100);

    auto doubleResult = importer.get("SUBTLWF");
    EXPECT_DOUBLE_EQ(std::get<double>(doubleResult["apple"]), 1.5);

    auto stringResult = importer.get("Dom_PoS_SUBTLEX");
    EXPECT_EQ(std::get<std::string>(stringResult["apple"]), "noun");
}

// ========== Non-Alphabetic Character Validation Tests ==========

TEST_F(SubtlexImporterTest, WordWithNumbersThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "word123,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithHyphenThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "test-word,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                EXPECT_TRUE(errorMsg.find("test-word") != std::string::npos) << "Error message should contain the invalid word";
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithApostropheThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "don't,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithSpaceThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "hello world,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    // Note: This will actually fail during CSV parsing because the comma in "hello world" will be treated as a delimiter
    // But we still want to ensure invalid words are caught
    EXPECT_THROW(SubtlexImporter importer(tempFile.path()), std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithUnderscoreThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "test_word,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithPunctuationThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "word!,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, EmptyWordThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << ",100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, WordWithMixedInvalidCharactersThrowsError)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "test123-word,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    TempCSVFile tempFile(oss.str());

    EXPECT_THROW(
        {
            try
            {
                SubtlexImporter importer(tempFile.path());
            }
            catch (std::runtime_error const & e)
            {
                std::string errorMsg = e.what();
                EXPECT_TRUE(errorMsg.find("Invalid word") != std::string::npos) << "Error message: " << errorMsg;
                throw;
            }
        },
        std::runtime_error);
}

TEST_F(SubtlexImporterTest, ValidAlphabeticWordsOnlyAccepted)
{
    std::ostringstream oss;
    oss << validHeader() << "\n";
    oss << "apple,100,50,80,40,1.5,0.176,2.3,0.362,noun,90,0.9,noun,90,3.5\n";
    oss << "banana,200,75,150,60,2.8,0.447,3.1,0.491,noun,180,0.9,noun,180,4.2\n";
    oss << "CHERRY,50,25,40,20,0.9,0.954,1.7,0.230,noun,45,0.9,noun,45,2.8\n";
    TempCSVFile tempFile(oss.str());

    // Should not throw - all words are valid alphabetic characters
    EXPECT_NO_THROW(SubtlexImporter importer(tempFile.path()));

    SubtlexImporter importer(tempFile.path());
    auto            result = importer.get("Word");
    EXPECT_EQ(result.size(), 3);
}

// ========== Main function ==========

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
