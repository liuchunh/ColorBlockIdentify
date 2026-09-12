#include <iostream>
#include <cstring>
#include <ctime>
#include <fstream>
#include <cstdint>

constexpr int WIDTH = 640;

int* mask;

void Generate(){
    std::ofstream Mask("mask.txt");
    
    srand(static_cast<unsigned int>(time(nullptr)));
    mask = new int[WIDTH + 1];
    memset(mask, 0, sizeof(mask));
    
    int index = 1;
    while (index < WIDTH){
        int runLength = rand() % 10 + 1; // 生成游程的随机长度为 1 - 10
        runLength = std::min(runLength, WIDTH - index); // 确保不会超过数组边界

        int runValue = rand() % 2; // 生成游程的随机值为 0 或 1
        for (int i = index; i < index + runLength; i ++ ){
            mask[i] = runValue;
        }

        index += runLength;
    }

    for (int i = 1; i <= WIDTH; i ++ ){
        Mask << mask[i] << " ";
    }
    Mask << std::endl;

    // delete[] mask;
    Mask.close();
}

constexpr int MaxRunsPerRow = (WIDTH + 1) / 2;

// 游程定义, 用于存储每行的连续像素段信息
typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

static bool ExtractRunsFromRow(const int* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount){
    uint16_t count = 0;
    uint16_t CurrX = 0;

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

int main(){
    Generate();

    // 针对 mask 进行游程的提取
    uint16_t RunsCount = 0;
    Run runs[MaxRunsPerRow]; // 假设最多有 MaxRunsPerRow 个游程

    std::ofstream Extract("Extract.txt");

    if (ExtractRunsFromRow(mask, WIDTH, runs, MaxRunsPerRow, &RunsCount)){
        if (RunsCount > 0){
            std::cout << "Successfully extracted " << RunsCount << " runs." << std::endl;
            for (uint16_t i = 0; i < RunsCount; i ++ ){
                Extract << "Run " << i + 1 << ": StartX = " << runs[i].StartX << ", EndX = " << runs[i].EndX << ", Label = " << runs[i].Label << std::endl;
            }
        }
        else{
            std::cout << "No runs found in the mask." << std::endl;
        }
    } 
    else {
        Extract << "Failed to extract runs: Exceeded maximum capacity." << std::endl;
    }

    return 0;
}