#include "CCL.h"

namespace ImgHandle {
    /**
     * @brief 将 BGR 像素值转换为 HSV 像素值
     *
     * @param pixel BGR 像素值
     * @return cv::Vec3b HSV 像素值
     */
    cv::Vec3b ConvertPixelBGR2HSV(const cv::Vec3b& pixel) {
        float b = pixel[0] / 255.0f;
        float g = pixel[1] / 255.0f;
        float r = pixel[2] / 255.0f;

        float max = std::max({ r, g, b });
        float min = std::min({ r, g, b });
        float delta = max - min;

        float h = 0.0f;
        if (delta != 0) {
            if (max == r) {
                h = 60.0f * (fmod(((g - b) / delta), 6.0f));
            }
            else if (max == g) {
                h = 60.0f * (((b - r) / delta) + 2.0f);
            }
            else if (max == b) {
                h = 60.0f * (((r - g) / delta) + 4.0f);
            }
        }

        if (h < 0) {
            h += 360.0f;
        }

        float s = (max == 0) ? 0 : (delta / max);
        float v = max;

        return cv::Vec3b(static_cast<uchar>(h / 2), static_cast<uchar>(s * 255), static_cast<uchar>(v * 255));
    }

    /**
     * @brief 在 HSV 色彩空间中查找指定范围内的像素
     *
     * @param src 原图像, cv::Mat type
     * @param lower HSV 色彩空间的下界
     * @param upper HSV 色彩空间的上节
     * @param mask [out] 输出的二值掩码图像, 255 表示在范围内, 0 表示不在范围内
     * @param PixelEps 允许的像素误差, 默认为 (2, 2, 2)
     */
    static void ImgInHsvRange(const cv::Mat& src, const cv::Scalar& lower, const cv::Scalar& upper, cv::Mat& mask, const cv::Scalar& PixelEps = cv::Scalar(2, 2, 2)) {
        int width = src.cols;
        int height = src.rows;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                cv::Vec3b pixel = src.at<cv::Vec3b>(y, x);
                if (pixel[0] >= lower[0] - PixelEps[0] && pixel[0] <= upper[0] + PixelEps[0] &&
                    pixel[1] >= lower[1] - PixelEps[1] && pixel[1] <= upper[1] + PixelEps[1] &&
                    pixel[2] >= lower[2] - PixelEps[2] && pixel[2] <= upper[2] + PixelEps[2]) {
                    mask.at<uchar>(y, x) = 255;
                }
                else {
                    mask.at<uchar>(y, x) = 0;
                }
            }
        }

    }

    /**
     * @brief 将 BGR 图像转换为 HSV 图像
     *
     * @param src
     * @return cv::Mat
     */
    static cv::Mat ConvertImg2HSV(const cv::Mat& src) {
        cv::Mat res = src.clone();
        int width = src.cols;
        int height = src.rows;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                res.at<cv::Vec3b>(y, x) = ConvertPixelBGR2HSV(src.at<cv::Vec3b>(y, x));
            }
        }

        return res;
    }
}

namespace FindComponent {
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
    static bool ExtractRunsFromRow(const int* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount) {
        uint16_t count = 0;
        uint16_t CurrX = 0;

        if (MaskPerRow == nullptr || runs == nullptr || RunsCount == nullptr) {
            return false;
        }

        *RunsCount = 0;

        while (CurrX < width) {
            while (CurrX < width && MaskPerRow[CurrX] == 0) {
                CurrX++;
            }

            if (CurrX >= width) {
                break;
            }

            if (count >= RunsCapacity) {
                return false;
            }

            runs[count].StartX = CurrX;

            while (CurrX < width && MaskPerRow[CurrX] != 0) {
                CurrX++;
            }

            runs[count].EndX = CurrX - 1;
            runs[count].Label = 0; // 初始化为未标记, 在本函数中不修改 label 标签
            count++;
        }

        *RunsCount = count;
        return true;
    }


    /**
     * @brief 将 Run 的信息更新到 Component 中, 用于计算连通块的面积、质心、边界框等信息
     *
     * @param components
     * @param run
     *
     * @attention 函数在修改 components 时, 请确保 run->Label 已经被正确分配, 且 components[run->Label] 已经被初始化
     */
    void UpdateComponent(Component* components, Run run, int RunHeight) {
        uint16_t Label = run.Label;
        if (Label == 0)
            return;

        if (Label >= MaxComponents) {
            assert(false && "Label exceeds MaxComponents");
        }

        Component* comp = &components[Label];

        int length = run.EndX - run.StartX + 1;
        comp->area += length;

        comp->minX = std::min(comp->minX, run.StartX);
        comp->minY = std::min((int)comp->minY, RunHeight);
        comp->maxX = std::max(comp->maxX, run.EndX);
        comp->maxY = std::max((int)comp->maxY, RunHeight);
    }

    uint16_t Find(Component* components, uint16_t Label) {
        if (Label >= MaxComponents) {
            assert(false && "Label exceeds MaxComponents");
            return 0;
        }

        uint16_t root = Label;
        while (components[root].parent != root) {
            root = components[root].parent;
        }

        while (components[Label].parent != root) {
            uint16_t temp = components[Label].parent;
            components[Label].parent = root;
            Label = temp;
        }

        return root;
    }

    uint16_t Union(Component* components, uint16_t Label1, uint16_t Label2) {
        if (Label1 >= MaxComponents || Label2 >= MaxComponents) {
            assert(false && "Label exceeds MaxComponents");
            return 0;
        }

        uint16_t root1 = Find(components, Label1);
        uint16_t root2 = Find(components, Label2);

        uint16_t areaRoot1 = components[root1].area;
        uint16_t areaRoot2 = components[root2].area;

        if (root1 != root2) {
            // 把面积更小的连通块 Root1 挂到面积更大的连通块 Root2 上, 以减少树的高度
            if (areaRoot1 <= areaRoot2) {
                components[root1].parent = root2;

                components[root2].area += components[root1].area;
                // components[root2].SumX += components[root1].SumX;
                // components[root2].SumY += components[root1].SumY;
                components[root2].minX = std::min(components[root2].minX, components[root1].minX);
                components[root2].minY = std::min(components[root2].minY, components[root1].minY);
                components[root2].maxX = std::max(components[root2].maxX, components[root1].maxX);
                components[root2].maxY = std::max(components[root2].maxY, components[root1].maxY);
            }
            else {
                components[root2].parent = root1;

                components[root1].area += components[root2].area;
                components[root1].minX = std::min(components[root1].minX, components[root2].minX);
                components[root1].minY = std::min(components[root1].minY, components[root2].minY);
                components[root1].maxX = std::max(components[root1].maxX, components[root2].maxX);
                components[root1].maxY = std::max(components[root1].maxY, components[root2].maxY);
            }
        }

        return root2;
    }

    uint16_t AllocateLabel(Component* components, uint16_t Label) {
        if (Label >= MaxComponents) {
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
        components[Label].area = ComponentDisabled;
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
    int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, int Height, const int RunsCount[MaskHeight], const int MaxLabelCount) {
        if (runs == nullptr || components == nullptr || MaxLabelCount < 0) {
            return false;
        }

        int Label = 0;

        // 第一行单独分配, 显然第一行上方没有游程, 则一一分配
        int count = RunsCount[0];
        for (int i = 0; i < count; i++) {
            Label++;
            runs[0][i].Label = AllocateLabel(components, Label);
        }

        for (int h = 1; h < Height; h++) {
            int count = RunsCount[h];
            for (int index = 0; index < count; index++) {
                Run* curr = &runs[h][index];
                bool AllocatedLabel = false;

                // 显然左右不可能再合并了, 不然就进到同一游程了
                // 因此我们考虑上下合并, 即当前游程的上部是否存在游程, 如果存在则使用同一标签, 如果不就新开标签
                for (int LastRowRunIndex = 0; LastRowRunIndex < RunsCount[h - 1]; LastRowRunIndex++) {
                    Run* prev = &runs[h - 1][LastRowRunIndex];
                    if (prev->EndX < curr->StartX) continue; // 上一行游程结束位置比当前 (curr) 的开始位置还早, 即无重叠
                    if (curr->EndX < prev->StartX) continue; // 上一行游程开始位置比当前的结束位置还早
                    if (curr->Label == 0) { // 当前游程还没有分配标签, 则使用上一行的游程标签
                        curr->Label = prev->Label;
                    }
                    else {
                        // 之前已经分配有标签了
                        // 此时我们考虑使用并查集合并标签
                        Union(components, curr->Label, prev->Label);
                    }
                    AllocatedLabel = true;
                }

                // 如果当前游程没有与上一行的游程重叠, 则分配一个新的标签
                if (!AllocatedLabel) {
                    Label++;
                    curr->Label = AllocateLabel(components, Label);
                }
            }
        }

        // 最后我们需要将所有游程的标签更新为最终的根标签, 并更新连通块的信息
        for (int h = 0; h < Height; h++) {
            for (int w = 0; w < RunsCount[h]; w++) {
                Run* curr = &runs[h][w];
                if (curr->Label != 0) {
                    curr->Label = Find(components, curr->Label);
                    UpdateComponent(components, *curr, h);
                }
            }
        }

        return Label;
    }

    void PrintComponents(const Component* components, int count) {
        for (int i = 1; i <= count; i++) {
            const Component* comp = &components[i];
            if (comp->area != ComponentDisabled) {
                if (comp->area == 0) {
                    std::cerr << std::format("Component {}: Area is 0, this should not happen!", i) << std::endl;
                }
                else if (comp->area > 0) {
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
}

#if 0
/**
 * @brief 将 cv::Mat 类型的二值掩码图像转换为 int 数组, 1 表示在范围内, 0 表示不在范围内
 *
 * @param Matmask
 */
static void MatrixMask2IntArray(const cv::Mat& Matmask) {
    int width = Matmask.cols;
    int height = Matmask.rows;

    assert(MaskWidth >= width && MaskHeight >= height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (Matmask.at<uchar>(y, x) > 0) {
                mask[y][x] = 1;
            }
            else {
                mask[y][x] = 0;
            }
        }
    }
}
#endif

bool IsPixelInHSVDistance(const cv::Vec3b& pixel, const cv::Vec3b& target, const unsigned int threshold) {
    unsigned int HSVDistance =
        static_cast<unsigned int>(pixel[0] - target[0]) * static_cast<unsigned int>(pixel[0] - target[0]) +
        static_cast<unsigned int>(pixel[1] - target[1]) * static_cast<unsigned int>(pixel[1] - target[1]) +
        static_cast<unsigned int>(pixel[2] - target[2]) * static_cast<unsigned int>(pixel[2] - target[2]);
    return HSVDistance <= threshold;
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
bool IsPixelInRange(const cv::Vec3b& pixel, const cv::Scalar& lower, const cv::Scalar& upper, const cv::Scalar& PixelEps, bool ColorKnown) {
    if (ColorKnown || PixelEps != cv::Scalar(0, 0, 0)) {
        static cv::Scalar lowerBound = lower - PixelEps;
        static cv::Scalar upperBound = upper + PixelEps;
        return (pixel[0] >= lowerBound[0] && pixel[0] <= upperBound[0] &&
            pixel[1] >= lowerBound[1] && pixel[1] <= upperBound[1] &&
            pixel[2] >= lowerBound[2] && pixel[2] <= upperBound[2]);
    }
    else { // 颜色未知, 或 PixelEps 为 0, 则使用 HSV 距离来判断
        cv::Vec3b Lower4Distance(lower[0], lower[1], lower[2]);
        if (PixelEps == cv::Scalar(0, 0, 0)) {
            return IsPixelInHSVDistance(pixel, Lower4Distance, HSVDistanceThreshold);
        }
        else {
            return IsPixelInHSVDistance(pixel, Lower4Distance, static_cast<unsigned int>(PixelEps[0] * PixelEps[0] + PixelEps[1] * PixelEps[1] + PixelEps[2] * PixelEps[2]));
        }
    }
}