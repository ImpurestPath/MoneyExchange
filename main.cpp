#include <iostream>
#include <curl/curl.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml++/libxml++.h>
#include <glibmm/ustring.h>
#include <glibmm/convert.h>
#include <cstdio>
#include <cstring>
#include <iomanip>

using namespace std;

enum ERRORS{
    NO_ERROR,
    CURL_GET_ERROR,
    CURL_INITIALIZATION_ERROR,
    FILE_ERROR,
    EXCEPTION,
    PARSER_ERROR,
    VALUTE_ERROR,
    ARGUMENTS_ERROR
};

bool getData(const xmlpp::Node *node, const Glib::ustring &title, pair<int, double> &result){
    int nominal = -1;
    double value = -1;
    bool found = false;

    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter) {
        if ((*iter)->get_name() == "CharCode") {
            xmlpp::Node::NodeList list2 = (*iter)->get_children();
            for(xmlpp::Node::NodeList::iterator iter2 = list2.begin(); iter2 != list2.end(); ++iter2) {
                const xmlpp::TextNode *nodeText = dynamic_cast<const xmlpp::TextNode *>((*iter2));
                if (nodeText){
                    if (nodeText->get_content() == title) {
                         found = true;
                         break;
                    }
                }
            }
        }
    }

    if (found) {
        for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter) {
            if (nominal == -1 && (*iter)->get_name() == "Nominal") {
                xmlpp::Node::NodeList list2 = (*iter)->get_children();
                for(xmlpp::Node::NodeList::iterator iter2 = list2.begin(); iter2 != list2.end(); ++iter2) {
                    const xmlpp::TextNode *nodeText = dynamic_cast<const xmlpp::TextNode *>((*iter2));
                    if (nodeText){
                        nominal = stoi(nodeText->get_content());
                    }
                }
            }
            else if (value == -1 && (*iter)->get_name() == "Value") {
                xmlpp::Node::NodeList list2 = (*iter)->get_children();
                for(xmlpp::Node::NodeList::iterator iter2 = list2.begin(); iter2 != list2.end(); ++iter2) {
                    const xmlpp::TextNode *nodeText = dynamic_cast<const xmlpp::TextNode *>((*iter2));
                    if (nodeText) value = stod(nodeText->get_content());
                }
            }
        }
        if (nominal != -1 && value != -1) {
            result.first = nominal;
            result.second = value;
            return true;
        } else return false;
    } else return false;
}
void exit_with_status(const enum ERRORS &status = NO_ERROR){
    switch (status){
        case CURL_GET_ERROR:
            cout << "Failed to get xml" << endl;
            break;
        case CURL_INITIALIZATION_ERROR:
            cout << "Failed to initialize CURL" << endl;
            break;
        case FILE_ERROR:
            cout << "Failed to open temporary file" << endl;
            break;
        case ARGUMENTS_ERROR:
            cout << "Wrong arguments" << endl;
            cout << "Usage -data=<> -from=<valute> -to=<valute>" << endl;
            break;
        case PARSER_ERROR:
            cout << "Failed to parse file" << endl;
            break;
        case VALUTE_ERROR:
            cout << "Failed to find valute" << endl;
            break;
        default:
            break;
        }
    if (status != ARGUMENTS_ERROR) remove("temp.xml");
    exit(status);
}

bool init(CURL *&conn, FILE *&file) {
    CURLcode code;

    conn = curl_easy_init();
    if(conn == NULL) {
        cout << "Failed to create CURL connection" << endl;
        return false;
    }

    code = curl_easy_setopt(conn, CURLOPT_URL, "https://www.cbr-xml-daily.ru/daily_eng.xml");
    if(code != CURLE_OK) {
        cout << "Failed to set URL" << endl;
        return false;
    }

    curl_easy_setopt(conn, CURLOPT_WRITEDATA, file);
    if(code != CURLE_OK) {
        cout << "Failed to set write data" << endl;
        return false;
    }

#ifdef SKIP_PEER_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    if(code != CURLE_OK) {
        cout << "Failed to set ssl verify peer" << endl;
        return false;
    }
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    if(code != CURLE_OK) {
        cout << "Failed to set ssl verify host" << endl;
        return false;
    }
#endif

    return true;
}

bool findValute(const xmlpp::Node *node, const Glib::ustring &from, const Glib::ustring &to,
                pair<int, double> &from_values, pair<int, double> &to_values) {
    if(node->get_name() == "ValCurs") {
        bool from_found = false,to_found = false;
        if (from == "RUB"){
            from_values.first = 1;
            from_values.second = 1;
            from_found = true;
        }
        if (to == "RUB"){
            to_values.first = 1;
            to_values.second = 1;
            to_found = true;
        }
        xmlpp::Node::NodeList list = node->get_children();
        for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter) {
            if (!from_found) {
                if (getData((*iter), from, from_values)) from_found = true;
            }
            if (!to_found) {
                if (getData((*iter), to, to_values)) to_found = true;
            }
            if (from_found && to_found)
                return true;
        }
        return false;
    }
}

int main(int argc,char* argv[]) {

    setlocale(LC_ALL, "ru_RU.utf8"); //For function stod - stod use locale for choose decimal separator(comma or point)


    if (argc < 2 || argc > 4) {
        exit_with_status(ARGUMENTS_ERROR);
    }


    Glib::ustring from_string,to_string;
    bool from_found = false,
            to_found = false;
    for (int j = 0; j < argc; ++j) {
        if (!from_found && strstr(argv[j], "-from=") != nullptr) {
            from_string = argv[j]+6;
            from_found = true;
        }
        else if (!to_found && strstr(argv[j], "-to=") != nullptr) {
            to_string = argv[j]+4;
            to_found = true;
        }
    }
    if (from_string.size() != 3 || to_string.size() != 3 || !from_found || !to_found) exit_with_status(ARGUMENTS_ERROR);


    FILE *data;
    data = fopen("temp.xml","w+");
    if (data == nullptr) exit_with_status(FILE_ERROR);

    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (!init(curl,data)) exit_with_status(CURL_INITIALIZATION_ERROR);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if (res != CURLE_OK) exit_with_status(CURL_GET_ERROR);

    fclose(data);

    try {
        xmlpp::DomParser parser;
        parser.parse_file("temp.xml");
        if(parser) {
            pair<int,double> from,to; //First element - nominal, second element - value
            const xmlpp::Node* pNode = parser.get_document()->get_root_node();
            if (findValute(pNode, from_string, to_string, from, to)) {
                double exchange = (from.second / from.first) / (to.second / to.first);
                int nominal = 1;
                while (exchange < 1) {
                    exchange *= 10;
                    nominal *= 10;
                }
                cout << nominal << " " << from_string << " = " << exchange << " " << to_string << endl;
                exit_with_status();
            } else  exit_with_status(VALUTE_ERROR);
        } else exit_with_status(PARSER_ERROR);
    } catch (const std::exception& ex) {
        cout << "Exception caught: " << ex.what() << std::endl;
        exit_with_status(EXCEPTION);
    }
    return 0;
}