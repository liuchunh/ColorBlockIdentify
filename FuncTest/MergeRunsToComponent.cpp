#include <iostream>
#include <cstdint>
#include <format>

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
    
    if (MaskPerRow == nullptr || runs == nullptr || RunsCount == nullptr){
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

        while (CurrX < width && MaskPerRow[CurrX] != 0){
            CurrX ++;
        }

        runs[count].EndX = CurrX - 1;
        runs[count].Label = 0; // 初始化为未标记, 在本函数中不修改 label 标签
        count ++;
    }

    *RunsCount = count;
    return true;
}

size_t componentCount = 0;

#define MaxRunsPerRow 4
#define HEIGHT 4

/**
 * @brief 将游程合并为连通块
 * 
 * @param runs 整个 二值图 中的游程信息
 * @param components 合并出的连通块
 * @param Height 游程的高度/行数
 * @param RunsCount int 数组, 记录了每行的游程数
 * @return int 连通块的个数
 * 
 * @attention 函数会修改游程中的 Label 标签和 components (下标从 1 开始), 且请保证 runs, components, RunsCount 不为 null pointer
 */
int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, int Height, const int RunsCount[HEIGHT], const int MaxLabelCount){
    if (runs == nullptr || components == nullptr || MaxLabelCount < 0){
        return false;
    }

    int Label = 0;

    // 第一行单独分配, 显然第一行上方没有游程, 则一一分配
    int count = RunsCount[0];
    for (int i = 0; i < count; i ++){
        Label ++;
        runs[0][i].Label = Label;
    }

    for (int h = 1; h < Height; h ++ ){
        int count = RunsCount[h];
        for (int index = 0; index < count; index ++ ){
            Run* curr = &runs[h][index];
            bool AllocateLabel = false;

            // 显然左右不可能再合并了, 不然就进到同一游程了
            // 因此我们考虑上下合并, 即当前游程的上部是否存在游程, 如果存在则使用同一标签, 如果不就新开标签
            for (int LastRowRunIndex = 0; LastRowRunIndex < RunsCount[h - 1]; LastRowRunIndex ++ ){
                Run* Current = &runs[h - 1][LastRowRunIndex];
                if (Current->EndX < curr->StartX) continue; // 上一行游程结束位置比当前 (curr) 的开始位置还早, 即无重叠
                if (curr->EndX < Current->StartX) continue; // 上一行游程开始位置比当前的结束位置还早
                if (curr->Label == 0){
                    curr->Label = Current->Label;
                }
                else{
                    // 之前已经分配有标签了, 此时 belike:
                    // ----   ----
                    //   -------
                    if (curr->Label > Current->Label){
                        curr->Label = Current->Label;
                    }
                    else{
                        Current->Label = curr->Label;
                    }
                }
                AllocateLabel = true;
            }

            if (!AllocateLabel){
                Label ++;
                curr->Label = Label;
            }
        }
    }

    return Label;
}

#define MaskHeight 4
#define MaskWidth 7

int mask[MaskHeight][MaskWidth] = {
    {0, 1, 1, 0, 0, 1, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 1, 0}
};

Run run[MaskHeight][MaxRunsPerRow];
int RunsCount[MaskHeight];
Component component[MaxRunsPerRow * MaskHeight];

int main(){
    uint16_t RunsCountRow = 0;
    for (int i = 0; i < MaskHeight; i ++ ){
        if (ExtractRunsFromRow(mask[i], MaskWidth, run[i], MaxRunsPerRow, &RunsCountRow)){
            std::cout << std::format("Detected {} runs in row {} ", RunsCountRow, i) << std::endl;
            RunsCount[i] = RunsCountRow;
            for (int j = 0; j < RunsCountRow; j ++ ){
                std::cout << std::format("[{} -> {}]", run[i][j].StartX, run[i][j].EndX) << std::endl;
            }
        }
        else{
            std::cout << "No Runs detected in " << i << " row." << std::endl;
        }
    }

    MergeRunsToComponent(run, component, MaskHeight, RunsCount, MaxRunsPerRow * MaskHeight);

    for (int i = 0; i < MaskHeight; i ++ ){
        int count = RunsCount[i];
        for (int j = 0; j < count; j ++ ){
            std::cout << std::format("[({}, {}) -> ({}, {}), Label: {}]", run[i][j].StartX, i, run[i][j].EndX, i, run[i][j].Label) << std::endl;
        }
    }

    return 0;
}