#pragma once

#include <array>
#include <string>
#include <vector>
#include <curl/curl.h>

#include "./types.hpp"

class Program
{
private:
    static size_t write_callback(void *ptr, size_t size, size_t nmemb, std::vector<byte> *program)
    {
        program->reserve(program->size() + size * nmemb);
        for (int idx = 0; idx < size * nmemb; idx++)
        {
            program->push_back(static_cast<byte *>(ptr)[idx]);
        }
        return nmemb;
    }

public:
    const std::string name;
    const std::string path;
    std::vector<byte> program{};

    Program(std::string name, std::string path) : name(name), path(path) {}

    void get_program()
    {
        // If we already have the program, we can return
        if (program.size() != 0)
            return;

        // Download the program
        CURL *curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &program);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
    }
};