#pragma once

#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <format>

typedef struct _run {
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component {
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
} Component;

struct Coordinate {
    uint16_t x, y;

    Coordinate() : x(0), y(0) {}
};

struct Color {
	cv::Vec3b BGR = { 0, 0, 0 };
	cv::Vec3b HSVLower = { 0, 0, 0 };
    cv::Vec3b HSVUpper = { 0, 0, 0 };
	std::string colorName = "Unknown";

	Color() = default;
    Color(const cv::Vec3b BGR, const cv::Vec3b HSVLower, const cv::Vec3b HSVUpper, const std::string& color)
        : BGR(BGR), HSVLower(HSVLower), HSVUpper(HSVUpper), colorName(color) {}

    // 这种情况只能用于未知颜色
    Color(const cv::Vec3b BGR, const cv::Vec3b HSV, const std::string& color)
		: BGR(BGR), HSVLower(HSV), colorName(color) {
        assert(color == "Unknown");
	}
};

constexpr int MaskWidth = 1280;
constexpr int MaskHeight = 720;
constexpr int ComponentDisabled = -1;

// HSV 距离阈值, 用于判断像素是否在指定范围内
// 这个值必须要比 PixelEps 大, 因为 PixelEps 已知颜色了, 有明确的值范围
// 而 HSVDistanceThreshold 是未知颜色的, 只能通过三维距离来判断, 所以需要更大的容差
const unsigned int HSVDistanceThreshold = 68 * 68; 

// 当前设置的 HSV 范围是蓝色范围
const cv::Vec3b BlueHSVLower = { 100, 43, 46 }; // HSV 范围下限
const cv::Vec3b BlueHSVUpper = { 124, 255, 255 }; // HSV 范围上限

const cv::Vec3b DefaultPixelEps = { 4, 12, 12 }; // HSV 范围容差
// 针对 HSV 色彩空间
// Hue 允许的误差不能太大, 而 Saturation 和 Value 可以适当放宽, 因为在不同光照条件下, 颜色的饱和度和亮度会有较大变化
const cv::Vec3b PixelEps = DefaultPixelEps; // HSV 范围容差

// 可以内置几种已知颜色的 HSV 范围, 再维护一个颜色已知/ 未知的标志位, 如果找不到的话就使用三维距离

// 黄色
const cv::Vec3b YellowHSVLower = { 26, 43, 46 }; // HSV 范围下限
const cv::Vec3b YellowHSVUpper = { 34, 255, 255 }; // HSV 范围上限

// 绿色
const cv::Vec3b GreenHSVLower = { 35, 43, 46 }; // HSV 范围下限
const cv::Vec3b GreenHSVUpper = { 77, 255, 255 }; // HSV 范围上限

const cv::Vec3b HSVConfirmThreshold = DefaultPixelEps; // 当确认中心颜色是否已知时, 使用此值作为 HSV 范围容差

const Color Blue = { {255, 0, 0}, {100, 43, 46}, {124, 255, 255}, "Blue" };
const Color Yellow = { {0, 255, 255}, {26, 43, 46}, {34, 255, 255}, "Yellow" };
const Color Green = { {0, 255, 0}, {35, 43, 46}, {77, 255, 255}, "Green" };

namespace ImgHandle {
    cv::Vec3b ConvertPixelBGR2HSV(const cv::Vec3b& pixel);
}

namespace FindComponent {
    constexpr int MaxRunsPerRow = (MaskWidth + 1) / 2;
    constexpr int MaxComponents = 1e3; // 先做测试

    uint16_t AllocateLabel(Component* components, uint16_t Label);

    void UpdateComponent(Component* components, Run run, int RunHeight);

    uint16_t Find(Component* components, uint16_t Label);

    uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2);

    int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, int Height, const int RunsCount[MaskHeight], const int MaxLabelCount);

    bool ExtractRunsFromRow(const int* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount);

	void PrintComponents(const Component* components, int count);
}

bool IsPixelInHSVDistance(const cv::Vec3b& pixel, const cv::Vec3b& target, const unsigned int threshold);

bool IsPixelInRange(const cv::Vec3b& pixel, const cv::Scalar& lower, const cv::Scalar& upper, const cv::Scalar& PixelEps = cv::Scalar(2, 2, 2), bool ColorKnown = true);

#define TEST_MODE_ENABLED 0