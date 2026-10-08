#include "DynamicArray.h"
#include <cstring>

void Init(DynamicArray* array, size_t initialCapacity = DefaultCapacity){
    if (array->Inited){
        return;
    }
    array->data = new ElementType[initialCapacity];
    array->size = 0;
    array->capacity = initialCapacity;
    array->Inited = true;
}

bool Append(DynamicArray* array, ElementType element){
    if (!array->Inited){
        Init(array);
    }

    if (array->size >= array->capacity){
        size_t newCapacity = array->capacity * 2;
        ElementType* newData = new ElementType[newCapacity];
        if (!newData){
            return false; // ÄÚ´æ·ÖÅäÊ§°Ü
        }
        std::memcpy(newData, array->data, array->size * sizeof(ElementType));
        delete[] array->data;
        array->data = newData;
        array->capacity = newCapacity;
    }
    array->data[array->size++] = element;
    return true;
}

void Clear(DynamicArray* array){
    if (array->Inited){
        delete[] array->data;
        array->data = nullptr;
        array->size = 0;
        array->capacity = 0;
        array->Inited = false;
    }
}

void Free(DynamicArray* array){
    delete[] array->data;
    array->data = nullptr;
    array->size = 0;
    array->capacity = 0;
}