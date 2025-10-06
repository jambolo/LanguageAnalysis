# LanguageAnalysis

## Overview

LanguageAnalysis is a set of C++ utilities for analyzing language data, with a focus on n-gram analysis.

## Utilities

### ngram_analyzer

- **Purpose:**  
  Analyzes a dictionary file and generates frequency statistics for all possible n-grams (substrings of length N) found in the
  words. It supports advanced sequence normalization for linguistic analysis.

- **Features:**  
  - Reads a dictionary file where each line contains a word and its weight (frequency).
  - Generates n-grams for all possible lengths.
  - Handles special linguistic sequences (e.g., "qu" is treated as a single letter, certain vowel/consonant combinations are
    normalized).
  - Calculates and displays the most frequent n-grams for each length.
  - Separately analyzes vowel-only and consonant-only n-grams.
  - Supports output in both human-readable and JSON formats.

- **Command-line Syntax:** `ngram_analyzer [options] <path>`
    - **Options:**
        - `--subtlex <path>`: Path to the SUBTLEX CSV file. (required)
        - `-k`: Number of top n-grams to display (ignored if --json is enabled, default: 10).
        - `--json`: Output results in JSON format.
*Example Usage:**  
    ```
    ngram_analyzer --json --subtlex SUBTLEX-US_2025-04-29.csv
    ngram_analyzer -k 20 --subtlex SUBTLEX-US_2025-04-29.csv
    ```

## Dependencies
  - [CLI11](https://github.com/CLIUtils/CLI11) for command-line parsing.
  - [nlohmann/json](https://github.com/nlohmann/json) for JSON output.

## Build System
  - Uses CMake (minimum version 3.23) with Ninja generator.
  - Requires C++17.

## Dataset Notes

### SUBTLEX-US Dataset
  - The SUBTLEX-US dataset is a word frequency list derived from American English subtitles, so it is biased towards contemporary spoken language use.
  - Several words have numerical values of "#N/A" that were converted to zero.
  - Several words have to be manually (and partially) corrected: don, dont, haven, havent. 