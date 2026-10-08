/*
 * CCL.c
 *
 *  Created on: 2026年9月18日
 *      Author: Lenovo
 */

#include "CCL.h"

Vec3b ConvertPixelRGB2HSV(uint16_t pixel){
    /* Expand RGB565 channels to the 8-bit range by bit replication. */
    uint8_t r = (uint8_t)(((pixel >> 8) & 0xF8U) |
                                      ((pixel >> 13) & 0x07U));
    uint8_t g = (uint8_t)(((pixel >> 3) & 0xFCU) |
                                      ((pixel >> 9) & 0x03U));
    uint8_t b = (uint8_t)(((pixel << 3) & 0xF8U) |
                                      ((pixel >> 2) & 0x07U));

    uint8_t max = r;
    uint8_t min = r;
    if (g > max) max = g;
    if (b > max) max = b;
    if (g < min) min = g;
    if (b < min) min = b;

    uint8_t delta = (uint8_t)(max - min);
    uint8_t h = 0U;

    if (delta != 0U) {
        uint16_t numerator;
        uint8_t offset;

        if (max == r) {
            /* H = 30 * (G - B) / delta in OpenCV's 0..179 scale. */
            if (g >= b) {
                numerator = (uint16_t)(30U * (uint16_t)(g - b));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = offset;
            } else {
                numerator = (uint16_t)(30U * (uint16_t)(b - g));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = (uint8_t)(180U - offset);
            }
        } else if (max == g) {
            /* H = 60 + 30 * (B - R) / delta. */
            if (b >= r) {
                numerator = (uint16_t)(30U * (uint16_t)(b - r));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = (uint8_t)(60U + offset);
            } else {
                numerator = (uint16_t)(30U * (uint16_t)(r - b));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = (uint8_t)(60U - offset);
            }
        } else {
            /* H = 120 + 30 * (R - G) / delta. */
            if (r >= g) {
                numerator = (uint16_t)(30U * (uint16_t)(r - g));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = (uint8_t)(120U + offset);
            } else {
                numerator = (uint16_t)(30U * (uint16_t)(g - r));
                offset = (uint8_t)((numerator + delta / 2U) / delta);
                h = (uint8_t)(120U - offset);
            }
        }

        if (h >= 180U) h = (uint8_t)(h - 180U);
    }

    return (Vec3b){h, (uint8_t)((max == 0U) ? 0U : (delta * 255U + max / 2U) / max), max};
}

#if 0
/**
 * @brief 将 RGB565 图像转换为 HSV 图像
 * 
 * @param src 源图像
 * 
 * @attention 该函数会将转换后的 HSV 图像存储在全局变量 hsvImage 中, 该变量的大小为 MaskHeight x MaskWidth, 且在每次调用前会将 hsvImage 清空
 */
void ConvertImg2HSV(const uint16_t src[MaskHeight][MaskWidth]){
    memset(hsvImage, 0, sizeof(hsvImage));
    uint8_t width = SCC8660_W;
    uint8_t height = SCC8660_H;

    for (uint8_t y = 0; y < height; y ++ ){
        for (uint8_t x = 0; x < width; x ++ ){
            uint8_t h, s, v;
            ConvertPixelRGB2HSV(src[y][x], &h, &s, &v);
            hsvImage[y][x].h = h;
            hsvImage[y][x].s = s;
            hsvImage[y][x].v = v;
        }
    }
}

/**
 * @brief 将 HSV 图像中在指定范围内的像素标记为 1, 其他像素标记为 0, 并将结果存储在全局变量 mask 中
 * 
 * @param src 源图像
 * @param lower HSV 范围下限
 * @param upper HSV 范围上限
 * @param PixelEps 
 * 
 * @attention 该函数会将结果存储在全局变量 mask 中, 该变量的大小为 MaskHeight x MaskWidth, 且在每次调用前会将 mask 清空
 */
void ImgInHsvRange(const Vec3b src[MaskHeight][MaskWidth], const Vec3b lower, const Vec3b upper, const Vec3b PixelEps){
    uint8_t width = SCC8660_W;
    uint8_t height = SCC8660_H;
    memset(mask, 0, sizeof(mask));

    for (uint8_t y = 0; y < height; y ++ ){
        for (uint8_t x = 0; x < width; x ++ ){
            if (src[y][x].h >= (int16_t)(lower.h - PixelEps.h) && src[y][x].h <= (int16_t)(upper.h + PixelEps.h) &&
                src[y][x].s >= (int16_t)(lower.s - PixelEps.s) && src[y][x].s <= (int16_t)(upper.s + PixelEps.s) &&
                src[y][x].v >= (int16_t)(lower.v - PixelEps.v) && src[y][x].v <= (int16_t)(upper.v + PixelEps.v)){
                mask[y][x] = 1;
            }
            else{
                mask[y][x] = 0;
            }
        }
    }
}
#endif

bool IsPixelInRange(const Vec3b pixel, const Vec3b lower, const Vec3b upper, const Vec3b PixelEps){
    return (pixel.h >= (int16_t)(lower.h - PixelEps.h) && pixel.h <= (int16_t)(upper.h + PixelEps.h) &&
            pixel.s >= (int16_t)(lower.s - PixelEps.s) && pixel.s <= (int16_t)(upper.s + PixelEps.s) &&
            pixel.v >= (int16_t)(lower.v - PixelEps.v) && pixel.v <= (int16_t)(upper.v + PixelEps.v));
}


/**
 * @brief 将 Run 的信息更新到 Component 中, 用于计算连通块的面积、质心、边界框等信息
 * 
 * @param components 
 * @param run 
 * 
 * @attention 函数在修改 components 时, 请确保 run->Label 已经被正确分配, 且 components[run->Label] 已经被初始化
 */
void UpdateComponent(Component* components, Run run, uint16_t RunHeight){
    uint16_t Label = run.Label;
    if (Label == 0 || Label >= TotalMaxComponent){
        return;
    }

    Component* comp = &components[Label];

    uint8_t length = run.EndX - run.StartX + 1;
    comp->area += length;
    comp->minX = min(comp->minX, run.StartX);
    comp->minY = min((uint16_t)comp->minY, RunHeight);
    comp->maxX = max(comp->maxX, run.EndX);
    comp->maxY = max((uint16_t)comp->maxY, RunHeight);
}

uint16_t Find(Component* components, uint16_t Label){
    if (Label >= TotalMaxComponent){
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
    if (Label1 >= TotalMaxComponent || Label2 >= TotalMaxComponent){
        return 0;
    }

    uint16_t root1 = Find(components, Label1);
    uint16_t root2 = Find(components, Label2);

    if (root1 != root2){
        components[root1].parent = root2;

        components[root2].area += components[root1].area;
        // components[root2].SumX += components[root1].SumX;
        // components[root2].SumY += components[root1].SumY;
        components[root2].minX = min(components[root2].minX, components[root1].minX);
        components[root2].minY = min(components[root2].minY, components[root1].minY);
        components[root2].maxX = max(components[root2].maxX, components[root1].maxX);
        components[root2].maxY = max(components[root2].maxY, components[root1].maxY);
    }

    return root2;
}

uint16_t AllocateLabel(Component* components, uint16_t Label){
    if (Label >= TotalMaxComponent){
        return 0;
    }

    components[Label].parent = Label;
    components[Label].area = 0;
    components[Label].minX = UINT16_MAX;
    components[Label].minY = UINT16_MAX;
    components[Label].maxX = 0;
    components[Label].maxY = 0;
    return Label;
}

#if 0
/**
 * @brief 按行提取游程信息, 将二值图像的每一行中连续的 1 的区域提取为游程, 并记录其起始和结束位置
 * 
 * @param MaskPerRow 单行的二值图数组
 * @param width 二值图的宽度
 * @param runs [out] 输出的游程数组
 * @param RunsCapacity 游程数组的容量/允许游程的最大数量
 * @param RunsCount [out] 得到的单行游程数量
 * @return true 
 * @return false 当游程数量超过 RunsCapacity 时返回 false, 表示提取失败
 */
bool ExtractRunsFromRow(const uint8_t* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint8_t* RunsCount){
    uint8_t count = 0;
    uint8_t CurrX = 0;
    
    if (MaskPerRow == NULL || runs == NULL || RunsCount == NULL){
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
int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, uint8_t Height, const uint8_t RunsCount[MaskHeight], const uint16 MaxLabelCount){
    if (runs == NULL || components == NULL || MaxLabelCount < 0){
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
#endif
