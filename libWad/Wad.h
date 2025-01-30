#include "Element.h"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <ctype.h>
#include <regex>
#include <algorithm>
#include <vector>
#include <string>
#include <cctype>
#include <stack>
#include <regex>

class Wad {
    private:
    Wad(const std::string &path);
    std::string wadPath = "";
    std::string magic = "";
    uint32_t numberofDescriptors = 0;
    uint32_t descriptorOffset = 0;
    std::vector<Element*> rootDirectory;
    Element* resolvePath(const std::string &path);
    Element* fileInFocus = nullptr;
    void propagateFileForward(int startOffset, int byteOffset);
    int findOffset(const std::string &path, const std::string &suffix = "");
    std::vector<std::string> splitString(const std::string& i_str, const std::string& i_delim);
    

    public:
    ~Wad();
    static Wad* loadWad(const std::string &path);
    std::string getMagic();
    bool isContent(const std::string &path);
    bool isDirectory(const std::string &path);
    int getSize(const std::string &path);
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    void createDirectory(const std::string &path);
    void createFile(const std::string &path);
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);
};
