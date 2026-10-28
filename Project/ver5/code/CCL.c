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

/**
 * @brief 判断像素是否在指定的 HSV 距离范围内, 计算公式为 (pixel.h - target.h)^2 + (pixel.s - target.s)^2 + (pixel.v - target.v)^2 <= threshold^2
 * 
 * @param pixel 指定的像素
 * @param target 目标像素
 * @param threshold 阈值
 * @return true 
 * @return false 
 * 
 * @attention 当被 IsPixelInRange 函数调用时, target 即为 lower. 可单独调用.
 */
bool IsPixelInDistanceRange(const Vec3b pixel, const Vec3b target, const uint16_t threshold){
    int8_t dh = (int8_t)pixel.h - (int8_t)target.h;
    int8_t ds = (int8_t)pixel.s - (int8_t)target.s;
    int8_t dv = (int8_t)pixel.v - (int8_t)target.v;

    uint32_t distanceSquared = (uint32_t)(dh * dh + ds * ds + dv * dv);
    return distanceSquared <= (uint32_t)(threshold * threshold);
}

/**
 * @brief 判断指定的像素是否符合要求, ColorKnown 为 true 时, 判断像素是否在指定的 HSV 范围内, ColorKnown 为 false 时, 判断像素是否在指定的三维 HSV 距离范围内
 * 
 * @param pixel 要判定的像素
 * @param lower HSV 范围下限
 * @param upper HSV 范围上限
 * @param PixelEps 允许的误差范围
 * @param ColorKnown 颜色是否已知的标志位
 * @return true 
 * @return false 
 * 
 * @attention 函数有两种形式, 是根据 ColorKnown 的值来判定的, 当 ColorKnown 为 true 时, lower, upper 是 HSV 范围的上下限, 当 ColorKnown 为 false 时, lower 是目标颜色, upper 写 0 即可, 当 PixelEps 为 0 的时候, 函数会使用默认的 HSVDistanceThreshold 来判断像素是否在指定的 HSV 距离范围内, 否则会根据 PixelEps 三维和的平方来计算阈值
 */
bool IsPixelInRange(const Vec3b pixel, const Vec3b lower, const Vec3b upper, const Vec3b PixelEps, bool ColorKnown){
    if (ColorKnown){
        return (pixel.h >= (int16_t)(lower.h - PixelEps.h) && pixel.h <= (int16_t)(upper.h + PixelEps.h) &&
            pixel.s >= (int16_t)(lower.s - PixelEps.s) && pixel.s <= (int16_t)(upper.s + PixelEps.s) &&
            pixel.v >= (int16_t)(lower.v - PixelEps.v) && pixel.v <= (int16_t)(upper.v + PixelEps.v));
    }
    else{
        if (PixelEps.h == 0 && PixelEps.s == 0 && PixelEps.v == 0)
            return IsPixelInDistanceRange(pixel, lower, HSVDistanceThreshold);
        else
            return IsPixelInDistanceRange(pixel, lower, (uint16_t)(PixelEps.h * PixelEps.h + PixelEps.s * PixelEps.s + PixelEps.v * PixelEps.v));
    }
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
    if (Label == 0){
        return;
    }
    if (Label >= TotalMaxComponent){
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

bool ColorBlockIdentify(Component* result, uint16_t* maxArea){
    LeftTopX = LeftTopY = RightBottomX = RightBottomY = 0;
    memset(component, 0, sizeof(component));
    LastRowRunCount = 0;

    uint8_t Label = 0;
    uint8_t width = SCC8660_W;
    uint8_t height = SCC8660_H;

    for (uint8_t y = 0; y < height; y ++ ){
        uint8_t RunIndex = 0;
        uint8_t x = 0;

        while (x < width){
            while (x < width && !IsPixelInRange( ConvertPixelRGB2HSV(scc8660_image[y][x]), TargetHSVLower, TargetHSVUpper, PixelEps, ColorKnown) ){
                x ++;
            }

            if (x >= width){
                break;
            }

            CurrentRowRun[RunIndex].StartX = x;

            while (x < width && IsPixelInRange( ConvertPixelRGB2HSV(scc8660_image[y][x]), TargetHSVLower, TargetHSVUpper, PixelEps, ColorKnown) ){
                x ++;
            }

            CurrentRowRun[RunIndex].EndX = x - 1;
            CurrentRowRun[RunIndex].Label = 0; // 初始化标签为 0
            RunIndex ++;
        }

        // 第一行没有上一行的游程信息, 直接为每个游程分配一个新的标签
        if (y == 0){
            for (uint8_t i = 0; i < RunIndex; i ++ ){
                Run* curr = &CurrentRowRun[i];
                Label ++;
                curr->Label = AllocateLabel(component, Label);
                UpdateComponent(component, *curr, y);
            }
        }
        else{
            for (uint8_t index = 0; index < RunIndex; index ++ ){
                Run* curr = &CurrentRowRun[index];
                bool AllocatedLabel = false;

                for (uint8_t LastRowRunIndex = 0; LastRowRunIndex < LastRowRunCount; LastRowRunIndex ++ ){
                    Run* prev = &LastRowRun[LastRowRunIndex];
                    if (prev->EndX < curr->StartX) continue;
                    if (curr->EndX < prev->StartX) continue;
                    if (curr->Label == 0){
                        curr->Label = prev->Label;
                    }
                    else{
                        Union(component, curr->Label, prev->Label);    
                    }
                    AllocatedLabel = true;
                }

                if (!AllocatedLabel){
                    Label ++;
                    curr->Label = AllocateLabel(component, Label);
                }

                UpdateComponent(component, *curr, y);
            }

            for (uint8_t index = 0; index < RunIndex; index ++ ){
                Run* curr = &CurrentRowRun[index];
                if (curr->Label != 0){
                    curr->Label = Find(component, curr->Label);
                    UpdateComponent(component, *curr, y);
                }
            }
        }

        memcpy(LastRowRun, CurrentRowRun, sizeof(CurrentRowRun));
        LastRowRunCount = RunIndex;
        memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
    }

    uint16_t MaxArea = 0;
    uint8_t ResultComponentIndex = 0;
    for (uint8_t i = 1; i <= Label; i ++ ){
        if (component[i].area >= MaxArea && component[i].parent == i) {
            MaxArea = component[i].area;
            ResultComponentIndex = i;

            LeftTopX = component[i].minX;
            LeftTopY = component[i].minY;
            RightBottomX = component[i].maxX;
            RightBottomY = component[i].maxY;
        }
    }

    if (MaxArea > 0 || (LeftTopX == 0 && LeftTopY == 0 && RightBottomX == 0 && RightBottomY == 0)){
        *maxArea = MaxArea;
        if (result != NULL){
            *result = component[ResultComponentIndex];
        }
        return true;
    }
    else{
        return false;
    }
}