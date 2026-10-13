/*
 * CCL.h
 *
 *  Created on: 2026年9月18日
 *      Author: Lenovo
 */

#ifndef CODE_CCL_H_
#define CODE_CCL_H_

#include "zf_device/zf_device_scc8660.h"
#include <stdbool.h>

// typedef uint16_t uint16;

#define MaskWidth SCC8660_W
#define MaskHeight SCC8660_H

#define MaxRunsPerRow ((MaskWidth + 1) / 2)
#if 0
#define MaxComponents (MaskWidth * MaskHeight)
#endif

// 先取代 MaxComponents, 减少内存, 不够再说
#define TotalMaxComponent 96

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define ComponentDisabled -1

#define HSVDistanceThreshold 70

extern bool ColorKnown;

// Run 结构体占用 2 + 2 + 2 = 6 字节, Component 结构体占用 2 + 2 + 1 + 1 + 1 + 1 + 1 = 9 字节 ---> 内存对齐 -> 12 字节
// 所以删去 Component 结构体中的 bool Used, 使其占用 2 + 2 + 1 + 1 + 1 + 1 = 8 字节, 内存对齐 -> 8 字节

typedef struct _run{
    uint8_t StartX, EndX;
    uint8_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component{
    // parent 仍为 uint16_t, 不修改成 uint8_t 是因为结构体的内存对齐, 修改后占用内存未改变, 故不做修改
    uint16_t parent; // 并查集的父节点, 用于合并连通块, 初始值为自身的标签

    uint16_t area; // 该连通块目前累计的像素数
    uint8_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
} Component;

// 对标 OpenCV 中的 Scalar 结构体, 用于表示 RGB 颜色空间中的一个像素点
typedef struct _scalar{
    uint8_t r, g, b;
} Scalar;

// 对标 OpenCV 中的 Vec3b 结构体, 用于表示 HSV 颜色空间中的一个像素点
typedef struct _vec3b{
    uint8_t h, s, v;
} Vec3b;

Vec3b ConvertPixelRGB2HSV(uint16_t pixel);

#if 0
void ConvertPixelRGB2HSV(uint16_t pixel, uint8_t* hue, uint8_t* sat, uint8_t* val);

void ConvertImg2HSV(const uint16_t src[MaskHeight][MaskWidth]);

void ImgInHsvRange(const Vec3b src[MaskHeight][MaskWidth], const Vec3b lower, const Vec3b upper, const Vec3b PixelEps);

bool ExtractRunsFromRow(const uint8_t* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint8_t* RunsCount);
#endif

bool IsPixelInDistanceRange(const Vec3b pixel, const Vec3b target, const uint16_t threshold);

bool IsPixelInRange(const Vec3b pixel, const Vec3b lower, const Vec3b upper, const Vec3b PixelEps, bool ColorKnown);

void UpdateComponent(Component* components, Run run, uint16_t RunHeight);

uint16_t Find(Component* components, uint16_t Label);

uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2);

uint16_t AllocateLabel(Component* components, uint16_t Label);

#if 0
int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, uint8_t Height, const uint8_t RunsCount[MaskHeight], const uint16 MaxLabelCount);
#endif

#endif /* CODE_CCL_H_ */
