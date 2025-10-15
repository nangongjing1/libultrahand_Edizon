/********************************************************************************
 * File: list_funcs.cpp
 * Author: ppkantorski
 * Description:
 *   This source file contains function declarations and utility functions related
 *   to working with lists and vectors of strings. These functions are used in the
 *   Ultrahand Overlay project to perform various operations on lists, such as
 *   removing entries, filtering, and more.
 *
 *   For the latest updates and contributions, visit the project's GitHub repository.
 *   (GitHub Repository: https://github.com/ppkantorski/Ultrahand-Overlay)
 *
 *   Note: Please be aware that this notice cannot be altered or removed. It is a part
 *   of the project's documentation and must remain intact.
 * 
 *  Licensed under both GPLv2 and CC-BY-4.0
 *  Copyright (c) 2024 ppkantorski
 ********************************************************************************/

#include <list_funcs.hpp>
#include <mutex>

namespace ult {
    static constexpr const char* UNABLE_TO_OPEN_FILE = "Unable to open file: ";

    // Thread-safe file access mutex
    static std::mutex file_access_mutex;
    
    std::vector<std::string> splitIniList(const std::string& value) {
        std::vector<std::string> result;
        std::string trimmed = value;
        trim(trimmed);
        if (trimmed.size() > 2 && trimmed.front() == '(' && trimmed.back() == ')') {
            trimmed = trimmed.substr(1, trimmed.size() - 2);
            ult::StringStream ss(trimmed);
            std::string token;
            while (ss.getline(token, ',')) {
                trim(token);
                result.push_back(token);
            }
        }
        return result;
    }
    
    std::string joinIniList(const std::vector<std::string>& list) {
        std::string result = "";
        for (size_t i = 0; i < list.size(); ++i) {
            result += list[i];
            if (i + 1 < list.size()) {
                result += ", ";
            }
        }
        return result;
    }


    /**
     * @brief Removes entries from a vector of strings that match a specified entry.
     *
     * This function removes entries from the `itemsList` vector of strings that match the `entry`.
     *
     * @param entry The entry to be compared against the elements in `itemsList`.
     * @param itemsList The vector of strings from which matching entries will be removed.
     */
    void removeEntryFromList(const std::string& entry, std::vector<std::string>& itemsList) {
        itemsList.erase(std::remove_if(itemsList.begin(), itemsList.end(), [&](const std::string& item) {
            return item.compare(0, entry.length(), entry) == 0;
        }), itemsList.end());
    }
    
    /**
     * @brief Filters a list of strings based on a specified filter list.
     *
     * This function filters a list of strings (`itemsList`) by removing entries that match any
     * of the criteria specified in the `filterList`. It uses the `removeEntryFromList` function
     * to perform the removal.
     *
     * @param filterList The list of entries to filter by. Entries in `itemsList` matching any entry in this list will be removed.
     * @param itemsList The list of strings to be filtered.
     */
    void filterItemsList(const std::vector<std::string>& filterList, std::vector<std::string>& itemsList) {
        for (const auto& entry : filterList) {
            removeEntryFromList(entry, itemsList);
        }
    }
    
        
    // Function to read file into a vector of strings with optional cap
    std::vector<std::string> readListFromFile(const std::string& filePath, size_t maxLines) {
        std::lock_guard<std::mutex> lock(file_access_mutex);
        std::vector<std::string> lines;
    
    #if !USING_FSTREAM_DIRECTIVE
        FILE* file = fopen(filePath.c_str(), "r");
        if (!file) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return lines;
        }
    
        static constexpr size_t BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
        size_t len;
        
        while (fgets(buffer, BUFFER_SIZE, file)) {
            // Check cap before processing
            if (maxLines > 0 && lines.size() >= maxLines) {
                break;
            }
            
            // More efficient newline removal
            len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                --len;
                // Also remove carriage return if present
                if (len > 0 && buffer[len - 1] == '\r') {
                    buffer[len - 1] = '\0';
                }
            }
            
            lines.emplace_back(buffer);
        }
    
        fclose(file);
    #else
        std::ifstream file(filePath);
        if (!file.is_open()) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return lines;
        }
    
        std::string line;
        while (std::getline(file, line)) {
            // Check cap before adding
            if (maxLines > 0 && lines.size() >= maxLines) {
                break;
            }
            
            // Remove carriage return if present (getline removes \n but not \r)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            lines.emplace_back(std::move(line));
        }
    
        file.close();
    #endif
    
        return lines;
    }
    
        
    // Function to get an entry from the list based on the index
    std::string getEntryFromListFile(const std::string& listPath, size_t listIndex) {
        std::lock_guard<std::mutex> lock(file_access_mutex);
        
    #if !USING_FSTREAM_DIRECTIVE
        FILE* file = fopen(listPath.c_str(), "r");
        if (!file) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + listPath);
            #endif
            return "";
        }
    
        static constexpr size_t BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
    
        // Skip lines until reaching the desired index
        for (size_t i = 0; i < listIndex; ++i) {
            if (!fgets(buffer, BUFFER_SIZE, file)) {
                fclose(file);
                return ""; // Index out of bounds
            }
        }
        
        // Read the target line
        if (!fgets(buffer, BUFFER_SIZE, file)) {
            fclose(file);
            return ""; // Index out of bounds
        }
        
        fclose(file);
        
        // Efficiently remove newline character
        const size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            if (len > 1 && buffer[len - 2] == '\r') {
                buffer[len - 2] = '\0';
            }
        }
        
        return std::string(buffer);
    
    #else
        std::ifstream file(listPath);
        if (!file.is_open()) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + listPath);
            #endif
            return "";
        }
    
        std::string line;
        
        // Skip lines until reaching the desired index
        for (size_t i = 0; i < listIndex; ++i) {
            if (!std::getline(file, line)) {
                return ""; // Index out of bounds
            }
        }
        
        // Read the target line
        if (!std::getline(file, line)) {
            return ""; // Index out of bounds
        }
    
        file.close();
        return line;
    #endif
    }


    /**
     * @brief Splits a string into a vector of strings using a delimiter.
     *
     * This function splits the input string into multiple strings using the specified delimiter.
     *
     * @param str The input string to split.
     * @return A vector of strings containing the split values.
     */
    std::vector<std::string> stringToList(const std::string& str) {
        std::vector<std::string> result;
        
        if (str.empty()) {
            return result;
        }
        
        // Check if the input string starts and ends with '(' and ')' or '[' and ']'
        if ((str.front() == '(' && str.back() == ')') || (str.front() == '[' && str.back() == ']')) {
            // Work directly with the original string using indices instead of creating substring
            size_t start = 1; // Skip opening bracket/paren
            size_t end = 1;
            const size_t values_end = str.size() - 1; // Skip closing bracket/paren
            
            // Pre-declare item string to avoid repeated allocations
            std::string item;
            
            // Iterate through the string manually to split by commas
            while ((end = str.find(',', start)) != std::string::npos && end < values_end) {
                // Extract item directly from original string without creating substring
                item.assign(str, start, end - start);
                
                // Trim leading and trailing spaces
                trim(item);
                
                // Remove quotes from each token if necessary
                removeQuotes(item);
                
                result.push_back(std::move(item));
                start = end + 1;
            }
            
            // Handle the last item after the last comma
            if (start < values_end) {
                item.assign(str, start, values_end - start);
                trim(item);
                removeQuotes(item);
                result.push_back(std::move(item));
            }
        }
        
        return result;
    }

    
    
    // Function to read file into a set of strings
    std::unordered_set<std::string> readSetFromFile(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(file_access_mutex);
        std::unordered_set<std::string> lines;
    
    #if !USING_FSTREAM_DIRECTIVE
        FILE* file = fopen(filePath.c_str(), "r");
        if (!file) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return lines;
        }
    
        static constexpr size_t BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
        size_t len;
        while (fgets(buffer, BUFFER_SIZE, file)) {
            // Remove trailing newline character if it exists
            len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            lines.insert(buffer);
        }
    
        fclose(file);
    #else
        std::ifstream file(filePath);
        if (!file.is_open()) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return lines;
        }
    
        std::string line;
        while (std::getline(file, line)) {
            lines.insert(std::move(line));
        }
    
        file.close();
    #endif
    
        return lines;
    }
    
    
    // Function to write a set to a file
    void writeSetToFile(const std::unordered_set<std::string>& fileSet, const std::string& filePath) {
        std::lock_guard<std::mutex> lock(file_access_mutex);
        
    #if !USING_FSTREAM_DIRECTIVE
        FILE* file = fopen(filePath.c_str(), "w");
        if (!file) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return;
        }
    
        for (const auto& entry : fileSet) {
            fprintf(file, "%s\n", entry.c_str());
        }
    
        fclose(file);
    #else
        std::ofstream file(filePath);
        if (!file.is_open()) {
            #if USING_LOGGING_DIRECTIVE
            logMessage(UNABLE_TO_OPEN_FILE + filePath);
            #endif
            return;
        }
        
        for (const auto& entry : fileSet) {
            file << entry << '\n';
        }
        
        file.close();
    #endif
    }

    // Helper function for streaming comparison
    void streamCompareAndWrite(const std::string& streamFilePath, 
                              const std::unordered_set<std::string>& compareSet, 
                              const std::string& outputTxtFilePath) {
        std::lock_guard<std::mutex> lock(file_access_mutex);
        
    #if !USING_FSTREAM_DIRECTIVE
        FILE* streamFile = fopen(streamFilePath.c_str(), "r");
        if (!streamFile) return;
        
        FILE* outputFile = fopen(outputTxtFilePath.c_str(), "w");
        if (!outputFile) {
            fclose(streamFile);
            return;
        }
        
        static constexpr size_t BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
        size_t len;
        
        while (fgets(buffer, BUFFER_SIZE, streamFile)) {
            len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            
            if (compareSet.count(std::string(buffer))) {
                fprintf(outputFile, "%s\n", buffer);
            }
        }
        
        fclose(streamFile);
        fclose(outputFile);
        
    #else
        std::ifstream streamFile(streamFilePath);
        if (!streamFile.is_open()) return;
        
        std::ofstream outputFile(outputTxtFilePath);
        if (!outputFile.is_open()) return;
        
        std::string line;
        while (std::getline(streamFile, line)) {
            if (compareSet.count(line)) {
                outputFile << line << '\n';
            }
        }
    #endif
    }
    
    // Function to compare two file lists and save duplicates to an output file
    void compareFilesLists(const std::string& txtFilePath1, const std::string& txtFilePath2, const std::string& outputTxtFilePath) {
        // Read files into sets
        std::unordered_set<std::string> fileSet1 = readSetFromFile(txtFilePath1);
        std::unordered_set<std::string> fileSet2 = readSetFromFile(txtFilePath2);
        
        // Always work with the smaller set for better performance
        if (fileSet1.size() <= fileSet2.size()) {
            // fileSet1 is smaller or equal - modify it
            for (auto it = fileSet1.begin(); it != fileSet1.end();) {
                if (fileSet2.count(*it) == 0) {
                    it = fileSet1.erase(it);
                } else {
                    ++it;
                }
            }
            writeSetToFile(fileSet1, outputTxtFilePath);
        } else {
            // fileSet2 is smaller - modify it instead
            for (auto it = fileSet2.begin(); it != fileSet2.end();) {
                if (fileSet1.count(*it) == 0) {
                    it = fileSet2.erase(it);
                } else {
                    ++it;
                }
            }
            writeSetToFile(fileSet2, outputTxtFilePath);
        }
    }
    
    void compareWildcardFilesLists(
        const std::string& wildcardPatternFilePath,
        const std::string& txtFilePath,
        const std::string& outputTxtFilePath
    ) {
        std::unordered_set<std::string> targetLines = readSetFromFile(txtFilePath);
        std::unordered_set<std::string> duplicates;
        
        auto wildcardFiles = getFilesListByWildcards(wildcardPatternFilePath);
        
        static constexpr size_t BUFFER_SIZE = 8192;

        for (auto& filePath : wildcardFiles) {
            if (filePath == txtFilePath || targetLines.empty()) {
                filePath = "";  // Clear early
                continue; 
            }
            
            std::lock_guard<std::mutex> lock(file_access_mutex);
            
    #if !USING_FSTREAM_DIRECTIVE
            FILE* file = fopen(filePath.c_str(), "r");
            if (!file) {
                #if USING_LOGGING_DIRECTIVE
                logMessage(UNABLE_TO_OPEN_FILE + filePath);
                #endif
                continue;
            }
    
            
            char buffer[BUFFER_SIZE];
            
            while (fgets(buffer, BUFFER_SIZE, file) && !targetLines.empty()) { // Early exit!
                char* newlinePos = strchr(buffer, '\n');
                if (newlinePos) *newlinePos = '\0';
                
                auto it = targetLines.find(std::string(buffer));
                if (it != targetLines.end()) {
                    duplicates.emplace(std::move(*it)); // Move instead of copy
                    targetLines.erase(it);
                }
            }
            fclose(file);
            
    #else
            std::ifstream file(filePath);
            if (!file.is_open()) {
                #if USING_LOGGING_DIRECTIVE
                logMessage(UNABLE_TO_OPEN_FILE + filePath);
                #endif
                continue;
            }
            
            std::string line;
            while (std::getline(file, line) && !targetLines.empty()) { // Early exit!
                auto it = targetLines.find(line);
                if (it != targetLines.end()) {
                    duplicates.emplace(std::move(*it)); // Move instead of copy
                    targetLines.erase(it);
                }
            }
    #endif
            filePath = "";  // Clear after processing - reduces vector memory footprint
        }
        
        writeSetToFile(duplicates, outputTxtFilePath);
    }
}