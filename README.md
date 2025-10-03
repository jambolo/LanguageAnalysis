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
    - **Arguments:**
        - **Options:**
            - `-k`: Number of top n-grams to display (ignored if --json is enabled, default: 10).
            - `--json`: Output results in JSON format.
    - `<path>` (positional): Path to the dictionary file.

- **Example Usage:**  
    ```
    ngram_analyzer --json dictionary.txt
    ngram_analyzer -k 20 dictionary.txt
    ```

## Dependencies
  - [CLI11](https://github.com/CLIUtils/CLI11) for command-line parsing.
  - [nlohmann/json](https://github.com/nlohmann/json) for JSON output.

## Build System
  - Uses CMake (minimum version 3.23) with Ninja generator.
  - Requires C++17.
