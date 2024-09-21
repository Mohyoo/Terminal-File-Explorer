// mkdir, rmdir...
// sort numbers names files using stoi().

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
using namespace std;

#define BLUE_BG "\033[44m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define BOLD    "\033[1m"
#define RESET   "\033[0m"


static string parent_path;
static vector<string> path_elements;
static vector<string> folders;
static vector<string> files;

void help();
void trim(string& str);
bool check_option(string& option);
void correct_quotes(string& option);
bool open_file(string path);
void open_dir(string path);
int show_dir(string path = getenv("HOME"), bool show_path = true);
int up();

int main() {
    printf("-Enter a path, or leave it empty to access the home folder.\n"
           "-After choosing a path, use numbers or names for access.\n"
           "-Type '-h' for instructions.\n\n");
    while (true) {
        try {
            // Get, trim & check the input, then show the result.
            string option;
            printf("%s%s-Option:%s ", BLUE_BG, BOLD, RESET);
            getline(cin, option);
            bool empty = option.empty();
            if(!empty) {
                trim(option);
                if (check_option(option)) continue;
                show_dir(option);
            } else {
                parent_path = getenv("HOME");
                show_dir();
            }
        } catch(const char* error) {
            printf("%s%s-Path doesn't exists!%s\n", BOLD, RED, RESET);
        } catch(...) {
            printf("%s%s-Invalid option!%s\n", BOLD, RED, RESET);
        }
    }
}

int up() {
    // Return to the parent path of the current directory.
    if (strlen(parent_path.c_str()) > 1) {
        string new_path;
        auto dirs = count(parent_path.begin(), parent_path.end(), '/');
        char* path = const_cast<char*>(parent_path.c_str());
        char* dir = strtok(path, "/");
        for (int i=0; i<dirs-1; i++) {
            new_path += "/";
            new_path += dir;
            dir = strtok(NULL, "/");
        }

        parent_path = new_path;
        if (parent_path.empty()) goto root;
        show_dir(parent_path, false);
        printf("-Now in: %s\n", parent_path.c_str());
        return 0;

    } else if (parent_path == "/") {
        printf("-Already in the root directory.\n");
        return 1;
    }
    root:
        parent_path = "/";
        show_dir(parent_path, false);
        printf("-Now in the root directory.\n");
        return 0;
}

void help() {
    const char* text = "-------------------- Instructions -------------------\n"
                       "-Welcome to our file explorer!\n"
                       "-Enter a path to access its elements, type '/' for the root\n"
                       " directory, or you can leave it empty to access the 'home' folder.\n"
                       "-In the terminal, You can access a folder by its name or its index\n"
                       "-Backslash (/) exists only to identify folders, no need to type it.\n"
                       "-To use the GUI file manager, type 'open' within a folder to open\n"
                       " it, or type 'open' then the index or the name of an element.\n\n"
                       "-Additional commands:\n"
                       "\t-'clear' to clear the terminal.\n"
                       "\t-'up' to go back.\n"
                       "\t-'-q' or 'Ctrl+C' to quit.\n"
                       "\t-'About'\n"
                       "-----------------------------------------------------\n";
    printf("%s%s%s", GREEN, text, RESET);
}

void trim(string& str) {
    while(str.substr(0, 1) == " ") {
        str.erase(0, 1);
    }

    int len = strlen(str.c_str());
    while(str.substr(len-1, len) == " ") {
        str.erase(len-1, len);
        len--;
    }
}

bool check_option(string& option) {
    // (0) = path.
    // (1) = program-defined command.
     
    // Convert to lowercase & Check for program-defined commands.
    string command;
    for (char c : option) command += tolower(c);

    if (command == "-q") {
        printf("\n-Quitting..."
               "\n-Thanks for using our file manager!\n");
        exit(0);
    } else if (command == "-h") {
        help();
        return 1;
    } else if (command == "clear") {
        system("clear");
        return 1;
    } else if (command == "up") {
        up();
        return 1;
    } else if(command == "open") {
        open_dir(parent_path);
        return 1;
    } else if (command.substr(0, 4) == "open") {
        if (path_elements.size() == 0) {
            printf("-Please, specify a path.\n");
            return 1;
        }
        string dir = command;
        dir.erase(0, 4);
        trim(dir);
        for (auto element : path_elements) {
            char* index = const_cast<char*>(element.c_str());
            index = strtok(index, ")");
            string name = strtok(NULL, ")");
            trim(name);
            // printf("%s - ", index);
            if (dir == index || dir == name) {
                open_dir(parent_path + "/" + name);
                break;
            }
        }
        return 1;
    } else if (command == "about") {
        printf("%s%s-Copyright (c) 2024 Mohyeddine Didouna.\n"
               "-MIT License, you can reuse it :)%s\n", BOLD, GREEN, RESET);
        return 1;
    }

    // A quick clean-up.
    while (option.substr(0, 1) == "/") {
        option.erase(0, 1);
    }

    int len = strlen(option.c_str());
    if (len > 0) {
        while (option.substr(len - 1, len) == "/") {
            option.pop_back();
            len--;
        }
    }

    // Check if it's a name or index.
    bool found = false;
    for (string element : path_elements) {
        char* c_element = const_cast<char*>(element.c_str());
        string index = strtok(c_element, ")");
        string name = strtok(NULL, ")");
        trim(name);
        if (option == index || option == name) {
            option = parent_path + "/" + name;
            found = true;
            break;
        }
    }

    if (!found) option = "/" + option;
    
    // Whatever the option was, check if it's a valid path.
    // If it's a valid path, but for a file, the parent path won't change.
    bool dir = filesystem::is_directory(option);
    ifstream file(option);
    if (dir) {
        parent_path = option;
        return 0;
    }
    else if (file) return 0;
    else throw "FileNotFound";
}

void correct_quotes(string& str) {
    // If a double quote (") exists in the given path, replace it with (\").
    if (str.find("\"") != string::npos) {
        string new_str;
        char* c_str = const_cast<char*>(str.c_str());
        char* part = strtok(c_str, "\"");
        while (part != NULL) {
            new_str += part;
            new_str += "\\\"\\\"";
            part = strtok(NULL, "\"");
        }
        int len = strlen(new_str.c_str());
        new_str.erase(len-4, len);
        str = new_str;
    }
}

bool open_file(string path) {
    bool dir = filesystem::is_directory(path);           // A file != a directory.
    ifstream file(path);                                 // If it's a file, it must exist.
    if (!dir && file) {
        correct_quotes(path);
        system(("xdg-open \""+ path + "\"").c_str());
        usleep(1'000'000);
        printf("\n");
        return true;
    }
    return false;
}

void open_dir(string path) {
    if (path.empty()) path = getenv("HOME");
    correct_quotes(path);
    system(("xdg-open \""+ path + "\"").c_str());
    usleep(1'000'000);
    printf("\n");
}

int show_dir(string path, bool show_path) {       // Default path is: /home.
    // First of all, if it's a file, ignore the rest of the code.
    if (open_file(path)) return 1;

    // Define the path & the iterator.
    path = path;
    auto entries = filesystem::directory_iterator(path);

    // Get the number of entries (dirs).
    int i = 0;
    for (auto entry : entries) {
        i++;
    }

    // Reinitialize the iterator, as it reached its end.
    entries = filesystem::directory_iterator(path);
    filesystem::path directory[i];
    i = 0;
    for (auto entry : entries) {
        directory[i] = entry.path();
        i++;
    }

    if (i == 0) {
        printf("%s%s-Empty directory.%s\n", BOLD, GREEN, RESET);
        return 0;
    }

    // Keep only the file name instead of the full path.
    // Also, clear the old vectors.
    path_elements.clear();
    folders.clear();
    files.clear();
    for (string element : directory) {
        string element_copy = element;       // A copy to use after destroying the ptr and its pointed-to-obj.
        char* c_element = const_cast<char*>(element.c_str());   // (c_element) is a ptr, it changes (element).
        char* dir = strtok(c_element, "/");
        int x = 0;
        while (dir != NULL) {
            dir = strtok(NULL, "/");
            x++;
        }
        // Reinitialize (c_element) after strtok() has destroyed it.
        c_element = const_cast<char*>(element_copy.c_str());
        dir = strtok(c_element, "/");
        for (int y=0; y<x; y++) {
            if (y == x - 1) {
                path_elements.push_back(dir);
                break;
            }
            dir = strtok(NULL, "/");
        }
    }

    // Sort & Index & Display the directory.
    sort(begin(path_elements), end(path_elements));
    vector<int> folders_indices;
    vector<int> files_indices;
    i = 0;

    for (string element : path_elements) {
        bool is_dir = filesystem::is_directory(path + "/" + element);
        if (is_dir) {
            folders.push_back(element);
            folders_indices.push_back(i);
        } else {
            files.push_back(element);
            files_indices.push_back(i);
        }
        i++;
    }
    
    i = 1;
    for (string folder : folders) {
        path_elements[folders_indices[i - 1]] = to_string(i) + ") " + folder;
        folder = to_string(i) + ") /" + folder;
        printf("%s%s%s%s\n", BOLD, GREEN, folder.c_str(), RESET);
        i++;
    }

    int j = 0;
    for (string file : files) {
        path_elements[files_indices[j]] = to_string(j + i) + ") " + file;
        file = to_string(j + i) + ") " + file;
        printf("%s%s%s\n", BOLD, file.c_str(), RESET);
        j++;
    }

    printf("\n");
    if (show_path) printf("-Now in: %s\n", parent_path.c_str());
    return 1;
}

/*

-To avoid naming conflict, we avoid (using namespace std),
 but it doesn't affect runtime performance.
-getline() needs a source, like a (file) or (cin: console input).
-cin.getline() works only with char[], with length as (2nd) arg.
-strtok() will destroy the given string, we need to reinit it.
-sort() & begin() & end() work with: vectors, string arrays, but not char* arrays.
-string class doesn't accept NULL.
-string class accepts adding any string format (char*, char[]...), but others don't.
-A non-init string will return "", but char* will return an undefined value,
 or if it's braced-init, it returns NULL.
-We can a bare (return) to return void.
-If -e.g- we use the delimiter ") " in strtok(), it would split it with " " too.
-To separate a function definition from its declaration, we define it using
 the same args, if we have a default arg, we set it at declaration.
-A file and a folder can't have the same name.

*/