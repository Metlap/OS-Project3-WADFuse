#include "Element.h"

Element::Element(uint32_t _offset, uint32_t _length, std::string _name){
    offset = _offset;
    length = _length;
    name = _name;
}

Element::~Element(){
    for(Element* elem : children){
        delete elem;
    }
}