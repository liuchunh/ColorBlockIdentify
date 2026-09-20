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

// typedef unsigned short uint16;

#define MaskWidth SCC8660_W
#define MaskHeight SCC8660_H

#define MaxRunsPerRow ((MaskWidth + 1) / 2)
#define MaxComponents (MaskWidth * MaskHeight)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component{
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    // uint64_t SumX, SumY; // 该连通块目前累计的像素坐标和, 用于计算质心

    unsigned char minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
    // uint16_t SeedX, SeedY; // 该块内任何一个像素的坐标

    bool Used; // 标签槽位是否正在使用
    bool Active; // 处理完当前行后, 是否还有像素在该连通块中, 用于判断是否需要继续处理下一行
} Component;

// 对标 OpenCV 中的 Scalar 结构体, 用于表示 RGB 颜色空间中的一个像素点
typedef struct _scalar{
    unsigned char r, g, b;
} Scalar;

// 对标 OpenCV 中的 Vec3b 结构体, 用于表示 HSV 颜色空间中的一个像素点
typedef struct _vec3b{
    unsigned char h, s, v;
} Vec3b;

extern Vec3b hsvImage[MaskHeight][MaskWidth];
extern unsigned char mask[MaskHeight][MaskWidth];

void ConvertPixelRGB2HSV(unsigned short pixel, unsigned char* hue, unsigned char* sat, unsigned char* val);

void ConvertImg2HSV(const unsigned char src[MaskHeight][MaskWidth]);

void ImgInHsvRange(const Vec3b src[MaskHeight][MaskWidth], const Vec3b lower, const Vec3b upper, const Vec3b PixelEps);

bool ExtractRunsFromRow(const uint16* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount);

void UpdateComponent(Component* components, Run run, uint16 RunHeight);

uint16_t Find(Component* components, uint16_t Label);

uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2);

uint16_t AllocateLabel(Component* components, uint16_t Label);

int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, unsigned char Height, const unsigned char RunsCount[MaskHeight], const uint16 MaxLabelCount);

#endif /* CODE_CCL_H_ */
