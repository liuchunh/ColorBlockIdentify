#include <cstdint>

typedef struct _component{
    uint16_t parent;
    uint32_t area; // 该连通块目前累计的像素数
    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形

    bool Used; // 标签槽位是否正在使用
} Component;

typedef Component ElementType;

constexpr size_t DefaultCapacity = 16;

typedef struct _dynamic_array{
    ElementType* data;
    size_t size;
    size_t capacity;
    bool Inited = false;
} DynamicArray;

void Init(DynamicArray* array, size_t initialCapacity);

bool Append(DynamicArray* array, ElementType element);

void Clear(DynamicArray* array);

void Free(DynamicArray* array);