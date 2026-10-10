#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <chrono>

namespace ImgHandle{
    /**
     * @brief 将 BGR 像素值转换为 HSV 像素值
     * 
     * @param pixel BGR 像素值
     * @return cv::Vec3b HSV 像素值
     */
    cv::Vec3b ConvertPixelBGR2HSV(const cv::Vec3b& pixel){
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
    
        for (int y = 0; y < height; y ++) {
            for (int x = 0; x < width; x ++) {
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

constexpr int MaskWidth = 1280;
constexpr int MaskHeight = 720;
constexpr int ComponentDisabled = -1;

typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _component{
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
} Component;

typedef struct _component2{
    uint16_t parent;

    uint16_t area; // 该连通块目前累计的像素数
    uint8_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
    
    bool Used;
} Component2;

typedef struct _component3{
    uint16_t parent;

    uint16_t area; // 该连通块目前累计的像素数
    uint8_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
} Component3;

namespace FindComponent{
    constexpr int MaxRunsPerRow = (MaskWidth + 1) / 2;
    constexpr int MaxComponents = 1e3; // 先做测试

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

    uint16_t Find(Component* components, uint16_t Label){
        if (Label >= MaxComponents){
            assert(false && "Label exceeds MaxComponents");
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
			assert(false && "Label exceeds MaxComponents");
            return 0;
        }

        uint16_t root1 = Find(components, Label1);
        uint16_t root2 = Find(components, Label2);

        uint16_t areaRoot1 = components[root1].area;
        uint16_t areaRoot2 = components[root2].area;

        if (root1 != root2){
            // 把面积更小的连通块 Root1 挂到面积更大的连通块 Root2 上, 以减少树的高度
            if (areaRoot1 <= areaRoot2){
                components[root1].parent = root2;

                components[root2].area += components[root1].area;
                // components[root2].SumX += components[root1].SumX;
                // components[root2].SumY += components[root1].SumY;
                components[root2].minX = std::min(components[root2].minX, components[root1].minX);
                components[root2].minY = std::min(components[root2].minY, components[root1].minY);
                components[root2].maxX = std::max(components[root2].maxX, components[root1].maxX);
                components[root2].maxY = std::max(components[root2].maxY, components[root1].maxY);
            }
            else{
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
    int MergeRunsToComponent(Run runs[][MaxRunsPerRow], Component* components, int Height, const int RunsCount[MaskHeight], const int MaxLabelCount){
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
            if (comp->area != ComponentDisabled){
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
}

#if 0
/**
 * @brief 将 cv::Mat 类型的二值掩码图像转换为 int 数组, 1 表示在范围内, 0 表示不在范围内
 * 
 * @param Matmask 
 */
static void MatrixMask2IntArray(const cv::Mat& Matmask){
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

const cv::Scalar HSVPixelEps(5, 5, 5); // HSV 像素误差范围, 用于判断像素是否在指定范围内
const cv::Scalar HSVLower(100, 43, 46);
const cv::Scalar HSVUpper(124, 255, 255);
const int MinRunLength = 8; // 最小游程长度, 小于该长度的游程不分配标签, 直接忽略

Run LastRowRun[FindComponent::MaxRunsPerRow], CurrentRowRun[FindComponent::MaxRunsPerRow];
size_t LastRowRunCount = 0;

int RunsCount[MaskHeight];
Component components[FindComponent::MaxComponents];

typedef struct coord{
    uint16_t x, y;
} Coordinate;

// 最大连通块的两个角. 用于画出外接矩形
Coordinate LeftTop, RightBottom;

bool IsPixelInRange(const cv::Vec3b& pixel, const cv::Scalar& lower, const cv::Scalar& upper, const cv::Scalar& PixelEps = cv::Scalar(2, 2, 2)) {
    // TODO: 如果在所有的调用中, lower 和 upper 都是固定的, 那么可以考虑只计算一次
    static cv::Scalar lowerBound = lower - PixelEps;
    static cv::Scalar upperBound = upper + PixelEps;
    return (pixel[0] >= lowerBound[0] && pixel[0] <= upperBound[0] &&
            pixel[1] >= lowerBound[1] && pixel[1] <= upperBound[1] &&
            pixel[2] >= lowerBound[2] && pixel[2] <= upperBound[2]);
}

#define TEST_MODE_ENABLED 0

constexpr int TestImgCount = 4;
static double SumDuration = 0.0;

int main() {
#if TEST_MODE_ENABLED
    std::string FileFolder = "C:\\Users\\Lenovo\\Pictures\\CCLTestImg\\";
    for (int i = 0; i < TestImgCount; i ++ ){
        std::string FilePath = std::format("{}{}.png", FileFolder, i + 1);
        cv::Mat src = cv::imread(FilePath);
        std::cout << std::format("Processing image: {}", FilePath) << std::endl;

#else
    cv::Mat src = cv::imread("C:\\Users\\Lenovo\\Downloads\\output-image.png");
#endif    

    if (src.empty()) {
        std::cerr << "Error: Could not open or find the image!" << std::endl;
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // std::cout << std::format("sizeof(component2) = {}, sizeof(component3) = {}", sizeof(Component2), sizeof(Component3)) << std::endl;

    LeftTop.x = LeftTop.y = RightBottom.x = RightBottom.y = 0;
    std::memset(RunsCount, 0, sizeof(RunsCount));
    std::memset(components, 0, sizeof(components));
    components[0].area = ComponentDisabled;
    std::memset(LastRowRun, 0, sizeof(LastRowRun));
    std::memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
    LastRowRunCount = 0;

    int Label = 0;

	int width = src.cols;

    for (int y = 0; y < src.rows; y ++ ){
        int RunIndex = 0;
        int x = 0;

        while (x < width){
            while (x < width && !IsPixelInRange( ImgHandle::ConvertPixelBGR2HSV( src.at<cv::Vec3b>(y, x) ), HSVLower, HSVUpper, HSVPixelEps) ){
                x ++;
            }

            if (x >= width){
                break;
            }

            CurrentRowRun[RunIndex].StartX = x;

            while (x < width && IsPixelInRange( ImgHandle::ConvertPixelBGR2HSV( src.at<cv::Vec3b>(y, x) ), HSVLower, HSVUpper, HSVPixelEps)){
                x ++;
            }

            CurrentRowRun[RunIndex].EndX = x - 1;
            CurrentRowRun[RunIndex].Label = 0; // 初始化为未标记
            RunIndex ++;
        }

        if (y == 0){
            for (int i = 0; i < RunIndex; i ++ ){
                Run* curr = &CurrentRowRun[i];
                Label ++;
                curr->Label = FindComponent::AllocateLabel(components, Label);
                FindComponent::UpdateComponent(components, *curr, y);
            }
        }
        else{
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
                        FindComponent::Union(components, curr->Label, prev->Label);    
                    }
                    AllocatedLabel = true;
                }

                if (!AllocatedLabel){
                    Label ++;
                    curr->Label = FindComponent::AllocateLabel(components, Label);
                }
            }

            for (int index = 0; index < RunIndex; index ++ ){
                Run* curr = &CurrentRowRun[index];
                if (curr->Label != 0){
                    curr->Label = FindComponent::Find(components, curr->Label);
                    FindComponent::UpdateComponent(components, *curr, y);
                }
            }
        }
		std::memcpy(LastRowRun, CurrentRowRun, sizeof(CurrentRowRun));
        LastRowRunCount = RunIndex;
		std::memset(CurrentRowRun, 0, sizeof(CurrentRowRun));
    }

    // FindComponent::PrintComponents(components, Label);

    size_t maxArea = 0;
    for (int i = 1; i <= Label; i ++ ){
        if (components[i].area >= maxArea && components[i].parent == i) {
            maxArea = components[i].area;
        
            LeftTop.x = components[i].minX;
            LeftTop.y = components[i].minY;
            RightBottom.x = components[i].maxX;
            RightBottom.y = components[i].maxY;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << std::format("Duration: {:.6f} seconds\n", duration.count());
    SumDuration += duration.count();

    if (LeftTop.x == 0 && LeftTop.y == 0 && RightBottom.x == 0 && RightBottom.y == 0){
        // 没有找到任何连通块
        std::cout << "No connected component found!" << std::endl;
    }
    else{
        std::cout << std::format("Final Component: Area: {}, Bounding Box: [({}, {}) -> ({}, {})]", maxArea, LeftTop.x, LeftTop.y, RightBottom.x, RightBottom.y) << std::endl;
        // cv::imshow("Source Image", src);
        // cv::imshow("Mask Image", Matmask);
        // cv::imshow("Source Mask Image", sMask);
#if TEST_MODE_ENABLED
#else
        cv::imshow("Source Image", src);
        cv::rectangle(src, cv::Point(LeftTop.x, LeftTop.y), cv::Point(RightBottom.x, RightBottom.y), cv::Scalar(0, 0, 255), 2);
        cv::imshow("Result Image", src);
        cv::waitKey(0); 
#endif
    }
#if TEST_MODE_ENABLED
    }

    std::cout << std::format("Average Duration: {:.6f} seconds\n", SumDuration / static_cast<double>(TestImgCount));
#endif

    return 0;
}