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

// 65 * 65 这个阈值可以慢慢调
#define HSVDistanceThreshold 4225

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

Run LastRowRun[MaxRunsPerRow], CurrentRowRun[MaxRunsPerRow];
uint16_t LastRowRunCount = 0;

Component component[TotalMaxComponent];

unsigned char LeftTopX, LeftTopY, RightBottomX, RightBottomY;

// 当前设置的 HSV 范围是蓝色范围
const Vec3b BlueHSVLower = {26, 43, 46}; // HSV 范围下限
const Vec3b BlueHSVUpper = {34, 255, 255}; // HSV 范围上限
const Vec3b PixelEps = {8, 3, 3}; // HSV 范围容差

// 可以内置几种已知颜色的 HSV 范围, 再维护一个颜色已知/未知的标志位, 如果找不到的话就使用三维距离

// 黄色
const Vec3b YellowHSVLower = {26, 43, 46}; // HSV 范围下限
const Vec3b YellowHSVUpper = {34, 255, 255}; // HSV 范围上限

// 绿色
const Vec3b GreenHSVLower = {35, 43, 46}; // HSV 范围下限
const Vec3b GreenHSVUpper = {77, 255, 255}; // HSV 范围上限

const Vec3b HSVConfirmThreshold = {4, 12, 12}; // 当确认中心颜色是否已知时, 使用此值作为 HSV 范围容差
extern Vec3b TargetHSVLower, TargetHSVUpper;

extern bool ColorKnown;

Vec3b ConvertPixelRGB2HSV(uint16_t pixel);

bool IsPixelInDistanceRange(const Vec3b pixel, const Vec3b target, const uint16_t threshold);

bool IsPixelInRange(const Vec3b pixel, const Vec3b lower, const Vec3b upper, const Vec3b PixelEps, bool ColorKnown);

void UpdateComponent(Component* components, Run run, uint16_t RunHeight);

uint16_t Find(Component* components, uint16_t Label);

uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2);

uint16_t AllocateLabel(Component* components, uint16_t Label);

bool ColorBlockIdentify(Component* result, uint16_t* maxArea);

#endif /* CODE_CCL_H_ */
