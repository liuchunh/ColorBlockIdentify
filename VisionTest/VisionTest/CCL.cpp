#include <iostream>
#include <opencv2/opencv.hpp>

constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// MaxRunsPerRow: 每行最多的连续像素段数, 由于每个像素段至少包含一个像素, 因此每行最多有 (WIDTH + 1) / 2 个连续像素段
#define MaxRunsPerRow ((WIDTH + 1) / 2)
#define MaxLabels (WIDTH + 1)

static bool Generate(int width, int height){
    cv::Mat image(height, width, CV_8UC3);
    cv::rectangle(image, cv::Rect(200, 200, 100, 100), cv::Scalar(0, 255, 0), -1);
    return cv::imwrite("CCLTest.png", image);
}

/**
 * @brief 判断目标像素是否符合要求
 * 
 * @param pixel 
 * @return true 
 * @return false
 * 
 * @note  不直接使用在嵌入式方案的代码中, 在嵌入式中需要使用 HSV 色彩空间来判断
 */
bool IsTargetPixel(const cv::Vec3b& pixel){
    return pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 0;
}

// 游程定义, 用于存储每行的连续像素段信息
typedef struct _run{
    uint16_t StartX, EndX;
    uint16_t Label; // 所属连通块的标签, 0 表示未标记
} Run;

typedef struct _conponent{
    uint16_t parent;

    uint32_t area; // 该连通块目前累计的像素数
    uint64_t SumX, SumY; // 该连通块目前累计的像素坐标和, 用于计算质心

    uint16_t minX, minY, maxX, maxY; // 该连通块的边界框 -> 外接矩形
    uint16_t SeedX, SeedY; // 该块内任何一个像素的坐标

    bool Used; // 标签槽位是否正在使用
    bool Active; // 处理完当前行后, 是否还有像素在该连通块中, 用于判断是否需要继续处理下一行
} Component;

/**
 * @brief 并查集操作, 寻找根结点并进行路径压缩
 * 
 * @param components 
 * @param label 
 * @return uint16_t 
 */
static uint16_t FindRoot(Component* components, uint16_t label){
    uint16_t root = label;
    while (components[root].parent != root){
        root = components[root].parent;
    }

    // 路径压缩
    uint16_t current = label;
    while (components[current].parent != current){
        uint16_t next = components[current].parent;
        components[current].parent = root;
        current = next;
    }

    return root;
}

/**
 * @brief 并查集操作, 合并两个连通块
 * 
 * @param components 
 * @param label1 
 * @param label2 
 */
static void Union(Component* components, uint16_t label1, uint16_t label2){
    uint16_t root1 = FindRoot(components, label1);
    uint16_t root2 = FindRoot(components, label2);

    if (root1 != root2){
        components[root2].parent = root1;
        
        components[root1].area += components[root2].area;
        components[root1].SumX += components[root2].SumX;
        components[root1].SumY += components[root2].SumY;

        components[root1].minX = min(components[root1].minX, components[root2].minX);
        components[root1].minY = min(components[root1].minY, components[root2].minY);
        components[root1].maxX = max(components[root1].maxX, components[root2].maxX);
        components[root1].maxY = max(components[root1].maxY, components[root2].maxY);
    }
}

/**
 * @brief 用于判断两个连续像素段是否属于同一个连通块
 * 
 * @param current 
 * @param previous 
 * @return uint8_t 
 */
static uint8_t IsRunConnected(const Run *current, const Run *previous){
    return (uint32_t)current->StartX <= (uint32_t)previous->EndX + 1 && (uint32_t)current->EndX >= (uint32_t)previous->StartX - 1;
}

/**
 * @brief 从二值图的一行中提取出该行的连续像素段信息
 * 
 * @param MaskPerRow 单行的二值图, 1 为符合要求, 0 为不符合要求
 * @param width 二值图宽度, 推荐为 WIDTH
 * @param runs 用于存储提取出的连续像素段信息, 即游程
 * @param RunsCapacity 游程最多能保存多少, 即数组大小, 推荐为 MaxRunsPerRow
 * @param [out] RunsCount [这是一个输出值] 实际提取出的连续像素段数
 * @return true 
 * @return false 
 * 
 * @note 该函数不会修改 runs 中的 label 标签, 仅仅是提取出连续像素段的起始和结束位置
 */
static bool ExtractRunsFromRow(const uint8_t* MaskPerRow, const uint16_t width, Run* runs, const uint16_t RunsCapacity, uint16_t* RunsCount){
    uint16_t count = 0;
    uint16_t CurrX = 0;

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

        while (CurrX < width && MaskPerRow[CurrX] == 1){
            CurrX ++;
        }

        runs[count].EndX = CurrX - 1;
        runs[count].Label = 0; // 初始化为未标记, 在本函数中不修改 label 标签
        count ++;
    }

    *RunsCount = count;
    return true;
}
 
int main(){
    if (!Generate(WIDTH, HEIGHT)){
        std::cerr << "Failed to generate test image." << std::endl;
        return -1;
    }

    cv::Mat src = cv::imread("CCLTest.png");
    if (src.empty()){
        std::cerr << "Failed to read test image." << std::endl;
        return -1;
    }

    cv::Mat res = src.clone();

    return 0;
}