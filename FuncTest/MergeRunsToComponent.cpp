#include <iostream>
#include <cstdint>
#include <format>

// 游程定义, 用于存储每行的连续像素段信息
typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component{
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    // uint64_t SumX, SumY; // 该连通块目前累计的像素坐标和, 用于计算质心

    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
    // uint16_t SeedX, SeedY; // 该块内任何一个像素的坐标

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

static bool ExtractRunsFromCol(const int* MaskPerCol, const uint16_t height, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount){
    uint16_t count = 0;
    uint16_t CurrY = 0;
    
    if (MaskPerCol == nullptr || runs == nullptr || RunsCount == nullptr){
        return false;
    }

    *RunsCount = 0;

    while (CurrY < height){
        while (CurrY < height && MaskPerCol[CurrY] == 0){
            CurrY ++;
        }

        if (CurrY >= height){
            break;
        }

        if (count >= RunsCapacity){
            return false;
        }

        runs[count].StartX = CurrY;

        while (CurrY < height && MaskPerCol[CurrY] != 0){
            CurrY ++;
        }

        runs[count].EndX = CurrY - 1;
        runs[count].Label = 0; // 初始化为未标记, 在本函数中不修改 label 标签
        count ++;
    }

    *RunsCount = count;
    return true;
}

#define MaxComponents 64

#define MaxRunsPerCol 3
#define MaxRunsPerRow 4
#define HEIGHT 4

/**
 * @brief 将 Run 的信息更新到 Component 中, 用于计算连通块的面积、质心、边界框等信息
 * 
 * @param components 
 * @param run 
 * 
 * @attention 函数在修改 components 时, 请确保 run->Label 已经被正确分配, 且 components[run->Label] 已经被初始化
 */
void UpdateComponent(Component* components, Run run, int RunHeight){
    uint16_t Label = run.Label;
    if (Label == 0 || Label >= MaxComponents){
        return;
    }

    Component* comp = &components[Label];

    // if (comp->area == 0){
    //     comp->SeedX = run.StartX;
    //     comp->SeedY = RunHeight;
    // }

    int length = run.EndX - run.StartX + 1;
    comp->area += length;
    
    // comp->SumX += (run.StartX + run.EndX) * length / 2; // 累加游程的中点坐标
    // comp->SumY += length * RunHeight; // 累加游程的行坐标
    
    comp->minX = std::min(comp->minX, run.StartX);
    comp->minY = std::min((int)comp->minY, RunHeight);
    comp->maxX = std::max(comp->maxX, run.EndX);
    comp->maxY = std::max((int)comp->maxY, RunHeight);
}

uint16_t Find(Component* components, uint16_t Label){
    if (Label >= MaxComponents){
        return 0;
    }

    uint16_t root = Label;
    while (components[root].parent != root){
        root = components[root].parent;
    }

    while (components[Label].parent != root){
        uint16_t temp = components[Label].parent;
        components[Label].parent = root;
        Label = temp;
    }

    return root;
}

uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2){
    if (Label1 >= MaxComponents || Label2 >= MaxComponents){
        return 0;
    }

    uint16_t root1 = Find(components, Label1);
    uint16_t root2 = Find(components, Label2);

    if (root1 != root2){
        components[root1].parent = root2;

        components[root2].area += components[root1].area;
        // components[root2].SumX += components[root1].SumX;
        // components[root2].SumY += components[root1].SumY;
        components[root2].minX = std::min(components[root2].minX, components[root1].minX);
        components[root2].minY = std::min(components[root2].minY, components[root1].minY);
        components[root2].maxX = std::max(components[root2].maxX, components[root1].maxX);
        components[root2].maxY = std::max(components[root2].maxY, components[root1].maxY);
    }

    return root2;
}

uint16_t AllocateLabel(Component* components, uint16_t Label){
    if (Label >= MaxComponents){
        return 0;
    }

    components[Label].parent = Label;
    components[Label].area = 0;
    // components[Label].SumX = 0;
    // components[Label].SumY = 0;
    components[Label].minX = UINT16_MAX;
    components[Label].minY = UINT16_MAX;
    components[Label].maxX = 0;
    components[Label].maxY = 0;
    // components[Label].SeedX = 0;
    // components[Label].SeedY = 0;
    components[Label].Used = true;
    components[Label].Active = true;
    return Label;
}

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
        runs[0][i].Label = AllocateLabel(components, Label);
    }

    for (int h = 1; h < Height; h ++ ){
        int count = RunsCount[h];
        for (int index = 0; index < count; index ++ ){
            Run* curr = &runs[h][index];
            bool AllocatedLabel = false;

            // 显然左右不可能再合并了, 不然就进到同一游程了
            // 因此我们考虑上下合并, 即当前游程的上部是否存在游程, 如果存在则使用同一标签, 如果不就新开标签
            for (int LastRowRunIndex = 0; LastRowRunIndex < RunsCount[h - 1]; LastRowRunIndex ++ ){
                Run* prev = &runs[h - 1][LastRowRunIndex];
                if (prev->EndX < curr->StartX) continue; // 上一行游程结束位置比当前 (curr) 的开始位置还早, 即无重叠
                if (curr->EndX < prev->StartX) continue; // 上一行游程开始位置比当前的结束位置还早
                if (curr->Label == 0){ // 当前游程还没有分配标签, 则使用上一行的游程标签
                    curr->Label = prev->Label;
                }
                else{
                    // 之前已经分配有标签了
                    // 此时我们考虑使用并查集合并标签
                    Union(components, curr->Label, prev->Label);
                }
                AllocatedLabel = true;
            }

            // 如果当前游程没有与上一行的游程重叠, 则分配一个新的标签
            if (!AllocatedLabel){
                Label ++;
                curr->Label = AllocateLabel(components, Label);
            }
        }
    }

    // 最后我们需要将所有游程的标签更新为最终的根标签, 并更新连通块的信息
    for (int h = 0; h < Height; h ++ ){
        for (int w = 0; w < RunsCount[h]; w ++ ){
            Run* curr = &runs[h][w];
            if (curr->Label != 0){
                curr->Label = Find(components, curr->Label);
                UpdateComponent(components, *curr, h);
            }
        }
    }

    return Label;
}

void PrintComponents(const Component* components, int count){
    for (int i = 1; i <= count; i ++ ){
        const Component* comp = &components[i];
        if (comp->Used){
            if (comp->area == 0){
                std::cerr << std::format("Component {}: Area is 0, this should not happen!", i) << std::endl;
            }
            else if (comp->area > 0){
                /*
                std::cout << std::format("Component {}: SumX: {}, SumY: {}, Area: {}, Centroid: ({}, {}), Bounding Box: [({}, {}) -> ({}, {})], Seed: ({}, {}), Parent: {}", 
                i, comp->SumX, comp->SumY, comp->area, comp->SumX / comp->area, comp->SumY / comp->area, 
                comp->minX, comp->minY, comp->maxX, comp->maxY,
                comp->SeedX, comp->SeedY, comp->parent) << std::endl;
                */
               std::cout << std::format("Component {}: Area: {}, Bounding Box: [({}, {}) -> ({}, {})], Parent: {}", 
                i, comp->area, comp->minX, comp->minY, comp->maxX, comp->maxY, comp->parent) << std::endl;
            }
        }
    }
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

typedef struct coord{
    uint16_t x, y;
} Coordinate;

// 最大连通块的两个角. 用于画出外接矩形
Coordinate LeftTop, RightBottom;

int main(){
    uint16_t RunsCountRow = 0;
    // for (int i = 0; i < MaskHeight; i ++ ){
    //     if (ExtractRunsFromRow(mask[i], MaskWidth, run[i], MaxRunsPerRow, &RunsCountRow)){
    //         std::cout << std::format("Detected {} runs in row {} ", RunsCountRow, i) << std::endl;
    //         RunsCount[i] = RunsCountRow;
    //         for (int j = 0; j < RunsCountRow; j ++ ){
    //             std::cout << std::format("[{} -> {}]", run[i][j].StartX, run[i][j].EndX) << std::endl;
    //         }
    //     }
    //     else{
    //         std::cout << "No Runs detected in " << i << " row." << std::endl;
    //     }
    // }

    uint16_t RunsCountCol = 0;
    for (int i = 0; i < MaskWidth; i ++ ){
        if (ExtractRunsFromCol(mask[i], MaskHeight, run[i], MaxRunsPerCol, &RunsCountCol)){
            std::cout << std::format("Detected {} runs in col {} ", RunsCountCol, i) << std::endl;
            RunsCount[i] = RunsCountCol;
            for (int j = 0; j < RunsCountCol; j ++ ){
                std::cout << std::format("[{} -> {}]", run[i][j].StartX, run[i][j].EndX) << std::endl;
            }
        }
        else{
            std::cout << "No Runs detected in " << i << " col." << std::endl;
        }
    }

    int Label = MergeRunsToComponent(run, component, MaskHeight, RunsCount, MaxRunsPerRow * MaskHeight);

    for (int i = 0; i < MaskHeight; i ++ ){
        int count = RunsCount[i];
        for (int j = 0; j < count; j ++ ){
            std::cout << std::format("[({}, {}) -> ({}, {}), Label: {}]", run[i][j].StartX, i, run[i][j].EndX, i, run[i][j].Label) << std::endl;
        }
    }

    PrintComponents(component, Label);

    size_t maxArea = 0;
    for (int i = 1; i <= Label; i ++ ){
        if (component[i].area > maxArea) {
            maxArea = component[i].area;
        
            LeftTop.x = component[i].minX;
            LeftTop.y = component[i].minY;
            RightBottom.x = component[i].maxX;
            RightBottom.y = component[i].maxY;
        }
    }

    std::cout << std::format("Final Component: Area: {}, Bounding Box: [({}, {}) -> ({}, {})]", maxArea, LeftTop.x, LeftTop.y, RightBottom.x, RightBottom.y) << std::endl;

    return 0;
}