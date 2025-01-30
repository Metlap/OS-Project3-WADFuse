#include "Wad.h"

Wad::Wad(const std::string &path){
    wadPath = path;
    int file_fd = open(wadPath.c_str(), O_RDONLY, 0777);
    if(file_fd == -1){
        perror("Failed to open file.");
    }
    char* buffer = new char[8];

    read(file_fd, buffer, 4);
    magic.append(buffer, 4);

    read(file_fd, buffer, 4);
    memcpy(&numberofDescriptors, buffer, 4);

    read(file_fd, buffer, 4);
    memcpy(&descriptorOffset, buffer, 4);

    uint32_t elemOffset = 0;
    uint32_t elemLength = 0;
    std::string elemName = "";
    lseek(file_fd, descriptorOffset, SEEK_SET);

    std::stack<std::vector<Element*>*> s;
    s.push(&rootDirectory);

    char regex_Map[] = "[E][0-9][M][0-9]";
    char regex_DirStart[] = "(.*)(_START)";
    char regex_DirEND[] = "(.*)(_END)";

    for(int i = 0; i < numberofDescriptors; i++){
        read(file_fd, buffer, 4);
        memcpy(&elemOffset, buffer, 4);
        read(file_fd, buffer, 4);
        memcpy(&elemLength, buffer, 4);
        elemName.clear();
        read(file_fd, buffer, 8);
        elemName.append(buffer, 8);
        elemName.erase(remove(elemName.begin(), elemName.end(), (char)0), elemName.end());

        if(elemLength == 0){
            if(elemName.find("_END") != std::string::npos){
                s.pop();
                continue;
            }
        }

        Element* elem = new Element(elemOffset, elemLength, elemName);

        //if(elemLength == 0){
        if(std::regex_match(elemName, std::regex(regex_Map)) 
        || std::regex_match(elemName, std::regex(regex_DirStart)) 
        || std::regex_match(elemName, std::regex(regex_DirEND)))
        {
            if(elemName.at(0) == 'E' && elemName.at(2) == 'M' && isdigit(elemName.at(1)) && isdigit(elemName.at(3))){
                elem->isDirectory = true;
                elem->isMapMarker = true;
                s.top()->push_back(elem);
                int counter = 0;
                while(counter < 10){
                    i++;
                    read(file_fd, buffer, 4);
                    memcpy(&elemOffset, buffer, 4);

                    read(file_fd, buffer, 4);
                    memcpy(&elemLength, buffer, 4);

                    elemName.clear();
                    read(file_fd, buffer, 8);
                    elemName.append(buffer, 8);
                    elemName.erase(remove(elemName.begin(), elemName.end(), (char)0), elemName.end());

                    Element* mapElement = new Element(elemOffset, elemLength, elemName);
                    mapElement->isDirectory = false;
                    mapElement->isMapMarker = false;
                    elem->children.push_back(mapElement);
                    counter++;
                }
            }
            else{
                elem->name = elemName.substr(0, elemName.find("_"));
                elem->isDirectory = true;
                elem->isMapMarker = false;
                s.top()->push_back(elem);
                s.push(&(elem->children));
            }
        }
        else{
            elem->isDirectory = false;
            elem->isMapMarker = false;
            s.top()->push_back(elem);
        }
    }
    delete[] buffer;
}

Wad::~Wad(){
    for(Element* elem : rootDirectory){
        delete elem;
    }
}

Wad* Wad::loadWad(const std::string &path){
    Wad *obj = new Wad(path);
    return obj;
}

std::string Wad::getMagic(){
    return magic;
}

Element* Wad::resolvePath(const std::string &path){
    std::string givenPath = path;
    if(givenPath.length() <= 1)
        return nullptr;

    if(givenPath.at(givenPath.length() - 1) == '/')
        givenPath.erase(givenPath.length() - 1, 1);

    if(givenPath.at(0) == '/')
        givenPath.erase(0, 1);
    
    if(givenPath.length() < 1)
        return nullptr;
    
    bool isLastFile;
    std::string searchFile;
    if(givenPath.find('/') == std::string::npos){
        isLastFile = true;
        searchFile = givenPath;
        givenPath.clear();
    }
    else{
        isLastFile = false;
        searchFile = givenPath.substr(0, givenPath.find('/'));
        givenPath = givenPath.substr(givenPath.find('/') + 1, givenPath.length() - givenPath.find('/') - 1);
    }

    std::vector<Element*> currentDirectory = rootDirectory;
    for(int i = 0; i < currentDirectory.size(); i++){
        if(currentDirectory.at(i)->name == searchFile){
            if(isLastFile)
                return currentDirectory.at(i);

            if(givenPath.find('/') == std::string::npos){
                isLastFile = true;
                searchFile = givenPath;
                givenPath.clear();
                currentDirectory = currentDirectory.at(i)->children;
                i = -1;
                continue;
            }
            else{
                isLastFile = false;
                searchFile = givenPath.substr(0, givenPath.find('/'));
                givenPath = givenPath.substr(givenPath.find('/') + 1, givenPath.length() - givenPath.find('/') - 1);
                currentDirectory = currentDirectory.at(i)->children;
                i = -1;
                continue;
            }
        }
    }
    return nullptr;
}

bool Wad::isContent(const std::string &path){
    if(resolvePath(path) == nullptr){
        return false;
    }
    return !resolvePath(path)->isDirectory;
}

bool Wad::isDirectory(const std::string &path){
    if(path == "/"){
        return true;
    }
    if(resolvePath(path) ==     ){
        return false;
    }
    return resolvePath(path)->isDirectory;
}

int Wad::getSize(const std::string &path){
    if(resolvePath(path) == nullptr || !isContent(path)){
        return -1;
    }
    else{
        Element* elem = resolvePath(path);
        return elem->length;
    }
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset){
    if(resolvePath(path) == nullptr || !isContent(path)){
        return -1;
    }

    Element* file = resolvePath(path);
    int readLength = std::min((uint32_t)length, file->length);

    if(offset + readLength > file->length){
        readLength = file->length - offset;
    }

    if(readLength <= 0){
        return 0;
    }

    int file_fd = open(wadPath.c_str(), O_RDONLY | O_CREAT, 0777);
    lseek(file_fd, file->offset + offset, SEEK_SET);
    read(file_fd, buffer, readLength);
    close(file_fd);
    return readLength;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory){
    if(path == "/"){
        for(Element* elem : rootDirectory){
            directory->push_back(elem->name);
        }
        return directory->size();
    }
    if(resolvePath(path) == nullptr){
        return -1;
    }
    if(isDirectory(path)){
        std::vector<Element*> iter = resolvePath(path)->children;
        for(Element* elem : iter){
            directory->push_back(elem->name);
        }
        return directory->size();
    }
    else{
        return -1;
    }
}

void Wad::createDirectory(const std::string &path){
    std::string _path = path;
    if(resolvePath(_path) != nullptr){return;}
    if(_path.back() == '/'){_path.pop_back();}
    std::string fileName = _path.substr(_path.find_last_of('/') + 1, _path.find_last_of('/') - _path.length());
    std::string parentPath = _path.substr(0, _path.find_last_of('/') + 1);
    Element* parentDir = resolvePath(parentPath);

    bool parentIsRoot = false;;
    if(parentPath == "/"){
        parentIsRoot = true;
    }
    if(fileName.length() > 2 || !parentIsRoot && (parentDir == nullptr || !parentDir->isDirectory || parentDir->isMapMarker)){
        return;
    }

    std::vector<Element*>* childrenVect;

    if(parentIsRoot){
        childrenVect = &rootDirectory;
    }
    else{
        childrenVect = &parentDir->children;
    }

    for(Element* elem : *childrenVect){
        if(elem->name == fileName){
            return;
        }
    }

    Element* newDir = new Element(0, 0, fileName);
    newDir->isDirectory = true;
    newDir->isMapMarker = false;
    
    int insertOffset = findOffset(parentPath, "_END");
    if(insertOffset == -1){
        return;
    }
    childrenVect->push_back(newDir);
    propagateFileForward(insertOffset, 32);

    int file_fd = open(wadPath.c_str(), O_RDWR, 0777);
    if(file_fd == -1){
        perror("Failed to open file.");
    }
    char cfileName[9];
    lseek(file_fd, insertOffset, SEEK_SET);
    memset(cfileName, 0, 8);
    write(file_fd, &cfileName, 8);
    strcpy(cfileName, (fileName + "_START").c_str());
    write(file_fd, &cfileName, 8);
    memset(cfileName, 0, 8);
    write(file_fd, &cfileName, 8);
    strcpy(cfileName, (fileName + "_END").c_str());
    write(file_fd, &cfileName, 8);
    numberofDescriptors+=2;
    lseek(file_fd, 4, SEEK_SET);
    write(file_fd, &numberofDescriptors, 4);
    return;
}

int Wad::findOffset(const std::string &path, const std::string &suffix){
    int file_fd = open(wadPath.c_str(), O_RDWR, 0777);
    if(file_fd == -1)
        perror("Failed to open file.");

    if(path == "/"){
        if(suffix == "_START"){
            return descriptorOffset;
        }
        else if(suffix == "_END"){
            return descriptorOffset + (numberofDescriptors * 16);
        }
        else{
            return -1;
        }
    }
    lseek(file_fd, descriptorOffset, SEEK_SET);
    char buf[8];

    std::string currentDescriptor = "";
    std::string currentDirectory = "/";
    std::string _path = path;
    if(_path.at(_path.length() - 1) == '/'){
        _path.erase(_path.length() - 1, 1);
    }
    _path = _path + suffix;
    std::vector<std::string> tokens = splitString(_path, "/");
    int currentOffset = descriptorOffset;
    int tokenCount = 0;
    int mapDirectoryFileCounter = 0;
    bool insideMapMarker = false;

    char regex_Map[] = "[E][0-9][M][0-9]";
    char regex_DirStart[] = "(.*)(_START)";
    char regex_DirEND[] = "(.*)(_END)";

    for(int i = 0; i < numberofDescriptors && mapDirectoryFileCounter < 10; i++){
        //Skip past offset and length
        lseek(file_fd, 8, SEEK_CUR);
        read(file_fd, buf, 8);
        currentDescriptor.clear();
        currentDescriptor.append(buf, 8);
        currentDescriptor.erase(remove(currentDescriptor.begin(), currentDescriptor.end(), (char)0), currentDescriptor.end());

        //Found a match for a file or E#M# directory
        if(currentDescriptor == tokens[tokenCount]){

            //If final token
            if(tokenCount == tokens.size() - 1)
                return currentOffset;
            
            //If E#M# directory
            if(std::regex_match(currentDescriptor, std::regex(regex_Map))){
                if(insideMapMarker){
                    return -1;
                }
                else{
                    insideMapMarker = true;
                }
            }
            tokenCount++;
        }
        //Descriptor is a _START marker
        else if(std::regex_match(currentDescriptor, std::regex(regex_DirStart))){
            //Matches current token
            if(currentDescriptor == (tokens[tokenCount] + "_START")){

                //If final token
                if(tokenCount == tokens.size() - 1)
                    return currentOffset;
                
                currentDirectory = tokens[tokenCount];
                tokenCount++;
            }
            //Suffix edge case
            else if(tokenCount == tokens.size() - 1 && currentDescriptor == tokens[tokenCount]){
                return currentOffset;
            }
            //Miscellaneous directory
            else{
                std::string startDescriptor = currentDescriptor.substr(0, currentDescriptor.find("_"));
                while(currentDescriptor != startDescriptor + "_END"){
                    currentOffset+=16;
                    lseek(file_fd, 8, SEEK_CUR);
                    read(file_fd, buf, 8);
                    currentDescriptor.clear();
                    currentDescriptor.append(buf, 8);
                    currentDescriptor.erase(remove(currentDescriptor.begin(), currentDescriptor.end(), (char)0), currentDescriptor.end());
                    i++;
                    if(i > numberofDescriptors){
                        return -1;
                    }
                }
                //Searching for _END edge case
                if(currentDescriptor == tokens[tokenCount]){
                    return currentOffset;
                }
            }
        }
        //Descriptor is a _END marker
        else if(std::regex_match(currentDescriptor, std::regex(regex_DirEND))){
            //Reached premature end of directory
            if(currentDescriptor == (currentDirectory + "_END")){
                return -1;
            }
            //Suffix edge case
            else if(tokenCount == tokens.size() - 1 && currentDescriptor == tokens[tokenCount]){
                return currentOffset;
            }
            //Miscellaneous end marker
            else{
                return -1;
            }
        }


        if(insideMapMarker)
            mapDirectoryFileCounter++;

        currentOffset+=16;
    }

    return -1;
}

std::vector<std::string> Wad::splitString(const std::string& i_str, const std::string& i_delim)
{
    std::vector<std::string> result;
    size_t startIndex = 0;

    for (size_t found = i_str.find(i_delim); found != std::string::npos; found = i_str.find(i_delim, startIndex))
    {
        result.emplace_back(i_str.begin()+startIndex, i_str.begin()+found);
        startIndex = found + i_delim.size();
    }
    if (startIndex != i_str.size())
        result.emplace_back(i_str.begin()+startIndex, i_str.end());

    if (result.size() != 0 && result.at(0).length() == 0)
        result.erase(result.begin());
    return result;      
}

void Wad::createFile(const std::string &path){
    std::string _path = path;
    if(resolvePath(_path) != nullptr){return;}
    if(_path.back() == '/'){_path.pop_back();}

    std::string fileName = _path.substr(_path.find_last_of('/') + 1, _path.find_last_of('/') - _path.length());
    std::string parentPath = _path.substr(0, _path.find_last_of('/') + 1);
    Element* parentDir = nullptr;
    //If new file is in root directory
    bool parentIsRoot = false;
    if(_path.substr(0, _path.find_last_of('/') + 1) == "/"){
        parentIsRoot = true;
    }
    else{
        parentDir = resolvePath(_path.substr(0, _path.find_last_of('/') + 1));
    }

    //setup regex
    char regex_Map[] = "[E][0-9][M][0-9]";
    if(fileName.length() > 8 || !parentIsRoot && (parentDir == nullptr || !parentDir->isDirectory || parentDir->isMapMarker) 
    || std::regex_match(fileName, std::regex(regex_Map))){
        return;
    }

    std::vector<Element*> *childrenVect;

    if(parentIsRoot){
        childrenVect = &rootDirectory;
    }
    else{
        childrenVect = &parentDir->children;
    }

    for(Element* elem : *childrenVect){
        if(elem->name == fileName){
            return;
        }
    }

    Element* newFile = new Element(0, 0, fileName);
    newFile->isDirectory = false;
    newFile->isMapMarker = false;

    int insertOffset = -1;
    if(parentIsRoot){
        insertOffset = descriptorOffset + (numberofDescriptors * 16);
        rootDirectory.push_back(newFile);
    }
    else{
        insertOffset = findOffset(parentPath, "_END");
        if(insertOffset == -1){
            return;
        }
        else{
            parentDir->children.push_back(newFile);
        }
    }

    propagateFileForward(insertOffset, 16);


    int file_fd = open(wadPath.c_str(), O_RDWR, 0777);
    if(file_fd == -1){
        perror("Failed to open file.");
    }
    char cfileName[9];
    uint32_t num;
    lseek(file_fd, insertOffset, SEEK_SET);
    num = 0;
    write(file_fd, &num, 4);
    num = 0;
    write(file_fd, &num, 4);
    strcpy(cfileName, (fileName).c_str());
    write(file_fd, &cfileName, 8);
    numberofDescriptors+=1;
    lseek(file_fd, 4, SEEK_SET);
    write(file_fd, &numberofDescriptors, 4);

    //Keep track of most recently created file, to
    //allow for multiple write calls but prevent file
    //overwriting.
    fileInFocus = newFile;
    
    return;
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset){
    std::string _path = path;
    if(resolvePath(_path) == nullptr){
        createFile(_path);
    }
    Element* file = resolvePath(_path);
    if(file == nullptr){
        createFile(_path);
    }
    else if(file->isDirectory){
        return -1;
    }
    if(fileInFocus != file){
        return 0;
    }

    int previousLength = file->length;
    if(offset + length > file->length){
        file->length = offset + length;
    }
    if(file->offset == 0){
        file->offset = descriptorOffset;
    }
    int lengthDifference = file->length - previousLength;
    int file_fd = open(wadPath.c_str(), O_RDWR, 0777);
    if(file_fd == -1){
        perror("Failed to open file.");
    }

    //Move descriptor list forward
    propagateFileForward(descriptorOffset, lengthDifference);

    //Update new descriptor offset
    descriptorOffset += lengthDifference;
    lseek(file_fd, 8, SEEK_SET);
    write(file_fd, &descriptorOffset, 4);
    
    //Create write buffer from passed in parameter
    char* writeBuffer = new char[length];
    memcpy(writeBuffer, buffer, length);

    //Write to lump data
    int insertOffset = file->offset + offset;
    lseek(file_fd, insertOffset, SEEK_SET);
    write(file_fd, writeBuffer, length);
    delete[] writeBuffer;

    //Update file descriptor
    lseek(file_fd, findOffset(path), SEEK_SET);
    write(file_fd, &file->offset, 4);
    write(file_fd, &file->length, 4);
    return length;
}

void Wad::propagateFileForward(int startOffset, int byteOffset){
    int file_fd = open(wadPath.c_str(), O_RDWR, 0777);
    if(file_fd == -1){
        perror("Failed to open file.");
    }
    struct stat st;
    fstat(file_fd, &st);
    int fileSize = st.st_size;
    char* buffer = new char[fileSize - startOffset];
    lseek(file_fd, startOffset, SEEK_SET);
    int bytesRead = read(file_fd, buffer, fileSize - startOffset);
    lseek(file_fd, startOffset, SEEK_SET);
    lseek(file_fd, byteOffset, SEEK_CUR);
    write(file_fd, buffer, bytesRead);
    delete[] buffer;
}