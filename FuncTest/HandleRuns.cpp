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
    {1, 1, 1, 0, 1, 1, 1},
    {0, 0, 1, 0, 0, 1, 0},
    {0, 1, 1, 0, 1, 1, 0}
};

bool isPixelInRange(int pixel){
    // 这里仅做结构示意
    return pixel;
}

Run CurrentRowRun[MaxRunsPerRow], LastRowRun[MaxRunsPerRow];
size_t LastRowRunCount = 0; // Current Row Run Count 使用 RunIndex 即可

namespace LabelOperation{
    constexpr int MaxComponents = 64;

    /**
     * @brief 分配一个新的标签给连通块, 并初始化该连通块的属性
     * 
     * @param components 
     * @param Label 
     * @return uint16_t 
     * 
     * @attention 使用此函数前请先 Label++ 来使用新的连通块
     */
    uint16_t AllocateLabel(Component* components, uint16_t Label){
        if (Label >= MaxComponents){
            return 0;
        }
    
        components[Label].Used = true;
        components[Label].parent = Label;
        components[Label].area = 0;
        components[Label].minX = UINT16_MAX;
        components[Label].minY = UINT16_MAX;
        components[Label].maxX = 0;
        components[Label].maxY = 0;
        return Label;
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

        int length = run.EndX - run.StartX + 1;
        comp->area += length;
        
        comp->minX = std::min(comp->minX, run.StartX);
        comp->minY = std::min((int)comp->minY, RunHeight);
        comp->maxX = std::max(comp->maxX, run.EndX);
        comp->maxY = std::max((int)comp->maxY, RunHeight);
    }
}

void PrintRuns(Run* runs, int count, int height){
    for (int i = 0; i < count; i ++ ){
        std::cout << std::format("[({}, {}) -> ({}, {}), Label: {}]", runs[i].StartX, height, runs[i].EndX, height, runs[i].Label) << std::endl;
    }
}

void PrintComponents(const Component* components, int count){
    for (int i = 1; i <= count; i ++ ){
        const Component* comp = &components[i];
        if (comp->Used){
            if (comp->area == 0){
                std::cerr << std::format("Component {}: Area is 0, this should not happen!", i) << std::endl;
            }
            else if (comp->area > 0){
               std::cout << std::format("Component {}: Area: {}, Bounding Box: [({}, {}) -> ({}, {})], Parent: {}{}", 
                i, comp->area, comp->minX, comp->minY, comp->maxX, comp->maxY, comp->parent, comp->parent == i ? (", Root") : ", NOT Root, ignore.") << std::endl;
            }
        }
    }
}

Component components[MaxRunsPerRow * MaskHeight]; // 下标从 1 开始, 0 表示未分配

int main(){
    int Label = 0;

    // 相当于删除了 mask 数组, 直接使用元数据
    // 第一行我们只提取游程即可
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

            CurrentRowRun[RunIndex].StartX = x;

            while (x < MaskWidth && isPixelInRange(mask[y][x])){
                x ++;
            }

            CurrentRowRun[RunIndex].EndX = x - 1;
            CurrentRowRun[RunIndex].Label = 0; // 初始化为未标记,
            RunIndex ++;
        }
        if (y == 0){
            // 如果是第一行的话, 直接将当前行的游程复制到上一行, 因为第一行没有上一行    
            for (int i = 0; i < RunIndex; i ++ ){
                Run* curr = &CurrentRowRun[i];
                Label ++;
                curr->Label = LabelOperation::AllocateLabel(components, Label);
                LabelOperation::UpdateComponent(components, *curr, y);
            }
        }
        else{
            // Copied from MergeRunToComponent.cpp
            for (int index = 0; index < RunIndex; index ++ ){
                Run* curr = &CurrentRowRun[index];
                bool AllocatedLabel = false;

                for (int LastRowRunIndex = 0; LastRowRunIndex < static_cast<int>(LastRowRunCount); LastRowRunIndex ++ ){
                    Run* prev = &LastRowRun[LastRowRunIndex];
                    if (prev->EndX < curr->StartX) continue;
                    if (curr->EndX < prev->StartX) continue;
                    if (curr->Label == 0){
                        curr->Label = prev->Label;
                    }
                    else{
                        LabelOperation::Union(components, curr->Label, prev->Label);    
                    }
                    AllocatedLabel = true;
                }

                if (!AllocatedLabel){
                    Label ++;
                    curr->Label = LabelOperation::AllocateLabel(components, Label);
                }
            }

            for (int index = 0; index < RunIndex; index ++ ){
                Run* curr = &CurrentRowRun[index];
                if (curr->Label != 0){
                    curr->Label = LabelOperation::Find(components, curr->Label);
                    LabelOperation::UpdateComponent(components, *curr, y);
                }
            }
        }
        // PrintRuns(CurrentRowRun, RunIndex, y);
        std::memcpy(LastRowRun, CurrentRowRun, sizeof(CurrentRowRun));
        LastRowRunCount = RunIndex;
        std::memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
    }

    PrintComponents(components, Label);
    return 0;
}