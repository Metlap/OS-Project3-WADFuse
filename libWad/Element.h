#include <string>
#include <vector>

class Element{
    public:
    uint32_t offset;
    uint32_t length;
    bool isDirectory;
    bool isMapMarker;
    std::string name;
    std::vector<Element*> children;
    Element(uint32_t _offset, uint32_t _length, std::string _name);
    ~Element();
};

