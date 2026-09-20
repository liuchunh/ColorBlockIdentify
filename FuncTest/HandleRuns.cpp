#include <iostream>
#include <cstdint>
#include <format>
#include <cstring>

typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component{
    uint16_t parent;
    uint32_t area; // 该连通块目前累计的像素数
    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形

    bool Used; // 标签槽位是否正在使用
} Component;

constexpr int MaskHeight = 4;
constexpr int MaskWidth = 7;

constexpr int MaxRunsPerRow = (MaskWidth + 1) / 2;

int mask[MaskHeight][MaskWidth] = {
    {0, 1, 1, 0, 0, 1, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 1, 0}
};

bool isPixelInRange(int pixel){
    // 这里仅做结构示意
    return pixel;
}

Run CurrentRow[MaxRunsPerRow], LastRow[MaxRunsPerRow];

void PrintRuns(Run* runs, int count, int height){
    for (int i = 0; i < count; i ++ ){
        std::cout << std::format("[({}, {}) -> ({}, {}), Label: {}]", runs[i].StartX, height, runs[i].EndX, height, runs[i].Label) << std::endl;
    }
}

int main(){
    // 相当于删除了 mask 数组, 直接使用元数据
    for (int y = 0; y < MaskHeight; y ++ ){
        int RunIndex = 0;
        int x = 0;
        while (x < MaskWidth){
            while (x < MaskWidth && !isPixelInRange(mask[y][x])){
                x ++;
            }
            
            if (x >= MaskWidth){
                break;
            }

            CurrentRow[RunIndex].StartX = x;

            while (x < MaskWidth && isPixelInRange(mask[y][x])){
                x ++;
            }

            CurrentRow[RunIndex].EndX = x - 1;
            CurrentRow[RunIndex].Label = 0; // 初始化为未标记,
            RunIndex ++;
        }
        PrintRuns(CurrentRow, RunIndex, y);
        std::swap(CurrentRow, LastRow);
        std::memset(CurrentRow, 0, sizeof(CurrentRow));
    }

    
    return 0;
}