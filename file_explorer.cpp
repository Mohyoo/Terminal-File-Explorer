#include <algorithm>
#include <cstddef>
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

#define B_BG  "\033[44m"    // Blue Background.
#define G     "\033[32m"    // Green.
#define R     "\033[31m"    // Red.
#define BD    "\033[1m"     // Bold.
#define RF    "\033[0m"     // Reset Font.


static string parent_path;
static vector<string> path_elements;
static vector<string> folders;
static vector<string> files;

void help();
void trim(string& str);
void remove_slashes(string& option);
int  check_option(string& option);
void correct_quotes(string& option);
bool open_file(string path);
void open_dir(string path);
int  show_dir(string path = getenv("HOME"), bool show_path = true);
int  up();
void mkfile(string file);
int  mkdir(string dir);
int  mkdirs(vector<string> dirs);
void rmdir_rmfile(string element);
void rmdirs_rmfiles(vector<string> items);
void rm_all();

int main() {
    printf("-Enter a path, or leave it empty to access the home folder.\n"
           "-After choosing a path, use numbers or names for access.\n"
           "-Please, type %s%s'-h'%s for instructions, just (2) min.\n\n", BD, G, RF);
    while (true) {
        try {
            // Get, trim & check the input, then show the result.
            string option;
            printf("%s%s-Option:%s ", B_BG, BD, RF);
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
        } catch(const char) {
            printf("%s%s-Path doesn't exists!%s\n", BD, R, RF);
        } /*catch(...) {
            printf("%s%s-Invalid option!%s\n", BD, R, RF);
        }*/
    }
}

void help() {
    printf( "%s--------------------------- Instructions ---------------------------\n"
            "-Enter a path to access its elements, type %s%s'/'%s%s only for the root\n"
            " directory, or you can leave it empty to access the 'home' folder.\n"
            "-In the terminal, You can access an element by its name or its index\n"
            "-Backslash %s%s(/)%s%s exists only to identify folders, no need to type it.\n"
            "-To use the GUI file manager, type 'open' to open the current folder,\n"
            " or type 'open' + the index/name of an element.\n\n"
            "-Additional commands:\n"
            "\t-'clear': clear the terminal.\n"
            "\t-'up': go back.\n"
            "\t-'where': show the current directory.\n"
            "\t-'mkfile: create one file at a time.'"
            "\t-'mkdir': create a new folder in the current directory; multiple\n"
            "\t          folders can be separated with a comma (,).\n"
            "\t-'remove': move one element or more to Trash; if it's a protected\n"
            "\t           element, it'll be %s%sPERMANENTLY%s%s deleted!\n"
            "\t-'Ctrl+C' or '-q' to quit.\n"
            "\t-'About'\n"
            "--------------------------------------------------------------------%s\n",
            G, BD, R, RF, G, BD, R, RF, G, BD, R, RF, G, RF );
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

void remove_slashes(string& str) {
    // Clean the given string from extra slashes from the beginning & the end.
    while (str[0] == '/') str.erase(0, 1);

    int len = str.length();
    while (str[len - 1] == '/') {
        str.pop_back();
        len--;
    }
}

int check_option(string& option) {
    // (0) = system path.
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
    } else if (command == "where") {
        if (parent_path != "") {
            printf("-Current directory: %s%s%s%s\n", BD, G, parent_path.c_str(), RF);
        } else {
            printf("%s%s-No directory was specified.%s\n", BD, R, RF);
        }
        return 1;
    } else if (command == "refresh") {
        if (parent_path != "") show_dir(parent_path);
        else show_dir(getenv("HOME"), false);
        return 1;
    } else if (command == "open") {
        open_dir(parent_path);
        return 1;
    } else if (command.substr(0, 4) == "open") {
        if (path_elements.size() == 0) {
            printf("-Please, specify a directory.\n");
            return 1;
        }
        string dir = command.substr(5);
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
    } else if (command.substr(0, 6) == "mkfile") {
        string file = command.substr(7);
        mkfile(file);
        return 1;
    } else if (command.substr(0, 5) == "mkdir") {
        string dir = command.substr(6);
        mkdir(dir);
        return 1;
    } else if (command.substr(0, 6) == "remove") {
        string element = command.substr(7);
        if (element == "all") rm_all();
        else rmdir_rmfile(element);
        return 1;
    } else if (command == "about") {
        printf("%s%s-Copyright (c) 2024 Mohyeddine Didouna.\n"
               "-Under the MIT License, you can reuse it :)%s\n", BD, G, RF);
        return 1;
    }

    // A quick clean-up.
    remove_slashes(option);
    if (parent_path == "/") parent_path.clear();

    // Check if it's a name or an index.
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

int show_dir(string path, bool show_path) {
    // First of all, if it's a file, ignore the rest of the code.
    if (open_file(path)) return 1;

    // Define the path & the iterator.
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

    if (i == 0 && show_path) {
        printf("%s%s-Empty directory.%s\n", BD, G, RF);
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
        printf("%s%s%s%s\n", BD, G, folder.c_str(), RF);
        i++;
    }

    int j = 0;
    for (string file : files) {
        path_elements[files_indices[j]] = to_string(j + i) + ") " + file;
        file = to_string(j + i) + ") " + file;
        printf("%s%s%s\n", BD, file.c_str(), RF);
        j++;
    }

    printf("\n");
    if (show_path) {
        if (parent_path != "/") printf("-Now in: %s%s%s%s\n", BD, G, parent_path.c_str(), RF);
        else printf("-Now in the %s%sroot%s directory.\n", BD, G, RF);
    }
    return 1;
}

int up() {
    // Return to the parent path of the current directory.
    if (strlen(parent_path.c_str()) > 1) {
        string new_path;
        auto max = count(parent_path.begin(), parent_path.end(), '/');
        char* path = const_cast<char*>(parent_path.c_str());
        char* dir = strtok(path, "/");
        for (int i=0; i<max-1; i++) {
            new_path += "/";
            new_path += dir;
            dir = strtok(NULL, "/");
        }

        parent_path = new_path;
        if (parent_path.empty()) goto root;
        show_dir(parent_path);
        return 0;

    } else if (parent_path == "/") {
        printf("-Already in the root directory.\n");
        return 1;
    }
    root:
        parent_path = "/";
        show_dir(parent_path);
        return 0;
}

void mkfile(string file) {
    // Check if it already exists.
    trim(file);
    remove_slashes(file);
    bool already_exist = false;
    for (string element : path_elements) {
        char* c_element = const_cast<char*>(element.c_str());
        char* index = strtok(c_element, ") ");
        char* name = strtok(NULL, ") ");
        if (name == file) {
            already_exist = true;
            break;
        }
    }

    if (already_exist) {
        printf("%s%s'%s'%s already exists, override it? (y/n):%s ", BD, G, file.c_str(), R, RF);
        string confirm;
        getline(cin, confirm);
        trim(confirm);
        if (confirm != "y") {
            printf("-Cancelled.\n");
            return;
        }
    }

    // Create the file.
    ofstream File(parent_path + "/" + file);
    File << "#";
    File.close();
    show_dir(parent_path, false);
    printf("%s%s-Created successfully.\n", BD, G);
    for (string element : path_elements) {
        string original = element;
        char* c_element = const_cast<char*>(element.c_str());
        char* index = strtok(c_element, ") ");
        char* name = strtok(NULL, ") ");
        if (name == file) {
            printf("%s\n", original.c_str());
            break;
        }
    }
}

int mkdir(string dir) {
    // Check if it's a list of dirs.
    vector<string> dirs;
    char* c_dir = const_cast<char*>(dir.c_str());
    char* element = strtok(c_dir, ", ");
    while (element != NULL) {
        dirs.push_back(element);
        element = strtok(NULL, ", ");
    }

    if (dirs.size() > 1) {
        mkdirs(dirs);
        return 0;
    }

    // A quick clean-up:
    remove_slashes(dir);

    // Check if it already exits.
    if (filesystem::is_directory(parent_path + "/" + dir)) {
        printf("%s%s-Directory already exists!%s\n", BD, R, RF);
        return 0;
    }

    // Create the dir.
    try {
        filesystem::create_directories(parent_path + "/" + dir);
    } catch (...) {
        printf("%s%s-Permission denied!%s\n", BD, R, RF);
        return 0;
    }

    // Refresh & Display the new dir's index.
    show_dir(parent_path, false);
    printf("%s%s-Successfully created.%s\n", BD, G, RF);

    for (string element : path_elements) {
        char* c_element = const_cast<char*>(element.c_str());
        char* index = strtok(c_element, ") ");
        char* name = strtok(NULL, ") ");
        if (name == dir) {
            printf("%s%s%s) /%s%s\n", BD, G, index, name, RF);
            break;
        }
    }
    return 1;
}   

int mkdirs(vector<string> dirs) {
    // Check for the dirs that already exists & reject duplication.
    vector<string> already_exist;
    vector<string> to_create;

    for (string& dir : dirs) {
        trim(dir);
        remove_slashes(dir);
        bool duplicated = false;
        for (string tc : to_create) {
            if (tc == dir) {
                duplicated = true;
                break;
            }
        } for (string ae : already_exist) {
            if (ae == dir) {
                duplicated = true;
                break;
            }
        }
        if (!duplicated) {
            string full_path = parent_path + "/" + dir;
            if (filesystem::is_directory(full_path)) {
                already_exist.push_back(dir);
            } else {
                to_create.push_back(dir);
            }
        }
    }

    // Create the dirs & check for permission.
    vector<string> denied;
    int n_denied = 0;
    for (string dir : to_create) {
        try {
            filesystem::create_directories(parent_path + "/" + dir);
        } catch (...) {
            denied.push_back(dir);
            n_denied++;
        }
    }
    
    // A final message.
    show_dir(parent_path, false);
    if (already_exist.size() == 0 && denied.size() == 0) {
        printf("%s%s-All were successfully created.%s\n", BD, G, RF);
        for (string dir : to_create) {
            for (string element : path_elements) {
                char* c_element = const_cast<char*>(element.c_str());
                char* index = strtok(c_element, ") ");
                char* name = strtok(NULL, ") ");
                if (name == dir) {
                    printf("%s%s%s) /%s%s\n", BD, G, index, name, RF);
                }
            }
        }
        return 1;
    } else if (already_exist.size() > 0 || denied.size() > 0) {
        if (already_exist.size() > 0) {
            printf("%s%s-These directories already exists:\n", BD, R);
            string rejected = "-";
            for (string dir : already_exist) {
                rejected += "/";
                rejected += dir.c_str();
                rejected += ", ";
            };
            rejected.erase(rejected.length() - 2);
            printf("%s\n", rejected.c_str());
        } if (denied.size() > 0) {
            string rejected = "-";
            printf("%s%s-Permission was denied for these directories:\n", BD, R);
            for (string dir : denied) {
                rejected += "/";
                rejected += dir.c_str();
                rejected += ", ";
            }
            rejected.erase(rejected.length() - 2);
            printf("%s\n", rejected.c_str());    
        }
    } if (to_create.size() - n_denied > 0) {
        printf("%s%s-Successfully created:%s\n", BD, G, RF);
        for (string dir : to_create) {
            for (string element : path_elements) {
                char* c_element = const_cast<char*>(element.c_str());
                char* index = strtok(c_element, ") ");
                char* name = strtok(NULL, ") ");
                if (name == dir) {
                    printf("%s%s%s) /%s%s\n", BD, G, index, name, RF);
                }
            }
        }
        return 1;
    } else {
        printf("%s%s-No directory was created.%s\n", BD, R, RF);
        return 0;
    }
}

void rmdir_rmfile(string element) {
    // Check if it's a list of dirs.
    vector<string> items;
    char* c_element = const_cast<char*>(element.c_str());
    char* dir = strtok(c_element, ", ");
    while (dir != NULL) {
        items.push_back(dir);
        dir = strtok(NULL, ", ");
    }

    if (items.size() > 1) {
        rmdirs_rmfiles(items);
        return;
    }

    // Check if it's an index
    for (string item : path_elements) {
        char* c_item = const_cast<char*>(item.c_str());
        char* index = strtok(c_item, ") ");
        char* name = strtok(NULL, ") ");
        if (index == element) element = name;
    }

    // Check if it's a valid path.
    remove_slashes(element);
    string full_path = parent_path + "/" + element;
    if (!filesystem::is_directory(full_path) && !ifstream(full_path)) {
        printf("%s%s-Path doesn't exist!%s\n", BD, R, RF);
        return;
    }

    // Confirm & Remove.
    string confirm;
    printf("%s%s-Are you sure you want to remove '%s'? (y/n):%s ", BD, R, element.c_str(), RF);
    getline(cin, confirm);
    if (confirm == "y") {
        try {
            remove(full_path.c_str());
            printf("\n");
            show_dir(parent_path, false);
            printf("%s%s-Removed successfully.%s\n", BD, G, RF);
        } catch (...) {
            printf("%s%s-Permission denied!%s\n", BD, R, RF);
        }
    }
    else printf("-Cancelled.\n");
}

void rmdirs_rmfiles(vector<string> items) {
    // Organize everything, trim the element to-delete, check for duplication,
    // and if it's an index, it'll be converted to a normal name.
    vector<string> to_delete;
    string list;
    for (string item : items) {
        trim(item);
        remove_slashes(item);
        bool duplicated = false;
        for (string td : to_delete) {
            if (td == item) duplicated = true;
        }
        if (duplicated) continue;
        for (string element : path_elements) {
            char* c_element = const_cast<char*>(element.c_str());
            char* index = strtok(c_element, ") ");
            char* name = strtok(NULL, ") ");
            if (item == index) item = name;
        }
        to_delete.push_back(item);
        if (filesystem::is_directory(parent_path + "/" + item)) list += "/" + item + ", ";
        else list += item + ", ";
    }

    // Confirm.
    list.erase(list.length() - 2);
    string confirm;
    int total = to_delete.size();
    printf("%s%s-Are you sure you want to delete (%d elements):\n", BD, R, total);
    printf("-%s? (y/n):%s ", list.c_str(), RF);
    getline(cin, confirm);
    trim(confirm);
    if (confirm != "y") {
        return;
        printf("-Cancelled.\n");
    }

    // Start removing (moving to Trash).
    vector<string> denied;
    for (string item : to_delete) {
        remove((parent_path + "/" + item).c_str());
    }

    printf("%s%s-All were removed successfully.\n%s", G, BD, RF);
}

void rm_all() {
    // Confirm.
    printf("%s%s-You are about to delete everything in %s%s%s !\n", BD, R, G, parent_path.c_str(), R);
    printf("%s-Are you sure? (y/n):%s ", BD, RF);
    string confirm;
    getline(cin, confirm);
    trim(confirm);
    if (confirm != "y") {
        printf("-Cancelled.\n");
        return;
    }

    // Start removing.
    for (string element : path_elements) {
        char* c_element = const_cast<char*>(element.c_str());
        char* index = strtok(c_element, ") ");
        char* name = strtok(NULL, ") ");
        remove((parent_path + "/" + name).c_str());
    }

    printf("%s%s-All were removed successfully.\n%s", G, BD, RF);
}
