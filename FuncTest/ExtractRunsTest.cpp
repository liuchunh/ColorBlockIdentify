#include <iostream>
#include <cstring>
#include <ctime>
#include <fstream>
#include <cstdint>

constexpr int WIDTH = 320;
constexpr int HEIGHT = 200;

int** mask;

std::ofstream Mask("mask.txt");

void Generate(){
    srand(static_cast<unsigned int>(time(nullptr)));
    mask = new int*[HEIGHT + 1];
    for (int i = 0; i < HEIGHT; i++){
        mask[i] = new int[WIDTH + 1];
        memset(mask[i], 0, sizeof(int) * (WIDTH + 1));
    }
    
    for (int row = 0; row < HEIGHT; row ++ ){
        int index = 0;
        while (index < WIDTH){
            int runLength = rand() % 10 + 1; // 生成游程的随机长度为 1 - 10
            runLength = std::min(runLength, WIDTH - index); // 确保不会超过数组边界
    
            int runValue = rand() % 2; // 生成游程的随机值为 0 或 1
            for (int i = index; i < index + runLength; i ++ ){
                mask[row][i] = runValue;
            }
    
            index += runLength;
        }
    }

    for (int i = 0; i < HEIGHT; i ++ ){
        for (int j = 0; j < WIDTH; j ++ ){
            Mask << mask[i][j] << " ";
        }
    }
    Mask << std::endl;

    // delete[] mask;
}

constexpr int MaxRunsPerRow = (WIDTH + 1) / 2;

// 游程定义, 用于存储每行的连续像素段信息
typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _conponent{
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    uint64_t SumX, SumY; // 该连通块目前累计的像素坐标和, 用于计算质心

    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
    uint16_t SeedX, SeedY; // 该块内任何一个像素的坐标

    bool Used; // 标签槽位是否正在使用
    bool Active; // 处理完当前行后, 是否还有像素在该连通块中, 用于判断是否需要继续处理下一行
} Component;

static bool ExtractRunsFromRow(const int* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount){
    uint16_t count = 0;
    uint16_t CurrX = 0;
    
    if (MaskPerRow == nullptr || runs == nullptr || RunsCapacity < 0 || RunsCount == nullptr){
        return false;
    }

    *RunsCount = 0;

    while (CurrX < width){
        while (CurrX < width && MaskPerRow[CurrX] == 0){
            CurrX ++;
        }

        if (CurrX >= width){
            break;
        }

        if (count >= RunsCapacity){
            return false;
        }

        runs[count].StartX = CurrX;

        while (CurrX < width && MaskPerRow[CurrX] == 1){
            CurrX ++;
        }

        runs[count].EndX = CurrX - 1;
        runs[count].Label = 0; // 初始化为未标记, 在本函数中不修改 label 标签
        count ++;
    }

    *RunsCount = count;
    return true;
}

Run runs[HEIGHT][MaxRunsPerRow]; // 假设最多有 MaxRunsPerRow 个游程 <-

int main(){
    Generate();

    // 针对 mask 进行游程的提取
    uint16_t RunsCount = 0;

    std::ofstream Extract("Extract.txt");

    for (int row = 0; row < HEIGHT; row ++ ){
        Extract << "Extracting runs from row " << row << "..." << std::endl;
        if (ExtractRunsFromRow(mask[row], WIDTH, runs[row], MaxRunsPerRow, &RunsCount)){
            if (RunsCount > 0){
                std::cout << "Successfully extracted " << RunsCount << " runs." << std::endl;
                for (uint16_t i = 0; i < RunsCount; i ++ ){
                    Extract << "Run " << i + 1 << ": StartX = " << runs[row][i].StartX << ", EndX = " << runs[row][i].EndX << ", Label = " << runs[row][i].Label << std::endl;
                }
            }
            else{
                std::cout << "No runs found in the mask." << std::endl;
            }
        } 
        else {
            Extract << "Failed to extract runs: Exceeded maximum capacity." << std::endl;
        }
    }

    return 0;
}