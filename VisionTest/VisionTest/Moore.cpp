#include <iostream>
#include <opencv2/opencv.hpp>

constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

// 逆时针
const int RelativePos[][2] = {
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}
};
const int RelativePosSize = sizeof(RelativePos) / sizeof(RelativePos[0]);

int sign(int x){
    return (x > 0) - (x < 0);
}

bool IsTarget(cv::Mat& src, int x, int y, const cv::Vec3b& TargetColor){
    if (x < 0 || y < 0 || x >= src.cols || y >= src.rows) {
        return false;
    }

    return src.at<cv::Vec3b>(y, x) == TargetColor;
}

int main(){
    cv::Mat src = cv::imread("square.png");
    if (src.empty()) {
        assert(0 && "Failed to read image");
        return 1;
    }

    cv::Mat res = src.clone();

    const cv::Vec3b TargetColor(0, 0, 255); // 红色
    const cv::Vec3b BorderColor(0, 255, 0); // 绿色

    int BorderX = 0, BorderY = 0;
    int BacktraceX = 0, BacktraceY = 0;

    bool FirstRedPixelFound = false;

    // 第一帧先全局遍历
    // 往后就采用第一帧的结果使用八邻域来分辨边界
    // 先从左下角找吧, 这样按照逆时针的八邻域搜索顺序就可以找到边界了
    for (int x = 0; x < src.cols; x ++) {
        for (int y = src.rows - 1; y >= 0; y --) {
            if (src.at<cv::Vec3b>(y, x) == TargetColor) {
                BorderX = x;
                BorderY = y;
                res.at<cv::Vec3b>(BorderY, BorderX) = BorderColor;
                // 找到红色像素点，开始八邻域搜索
                FirstRedPixelFound = true;
                break;
            }
        }

        if (FirstRedPixelFound) {
            break;
        }
    }

    std::cout << std::format("First Red pixel found at: ({}, {})", BorderX, BorderY) << std::endl;

    int StartX = BorderX;
    int StartY = BorderY;

    BacktraceX = BorderX - 1;
    BacktraceY = BorderY;

    do{
        int dx = sign(BacktraceX - BorderX);
        int dy = sign(BacktraceY - BorderY);
        int PosIndex = 0;

        // 确认方向
        for (int index = 0; index < RelativePosSize; index ++){
            if (RelativePos[index][0] == dx && RelativePos[index][1] == dy) {
                PosIndex = index;
                break;
            }
        }
        
        for (int step = 0; step < RelativePosSize; step ++){
            int dir = (PosIndex - step + 8) % 8; // 顺时针搜索

            int CurPosX = BorderX + RelativePos[dir][0];
            int CurPosY = BorderY + RelativePos[dir][1];

            if (IsTarget(src, CurPosX, CurPosY, TargetColor)) {
                // 更新回溯点
                int PrevDir = (dir + 1) % 8; // 逆时针方向
                BacktraceX = BorderX + RelativePos[PrevDir][0];
                BacktraceY = BorderY + RelativePos[PrevDir][1];

                BorderX = CurPosX;
                BorderY = CurPosY;
                res.at<cv::Vec3b>(BorderY, BorderX) = BorderColor;
                break;
            }
        }

    } while (BorderX > 0 && BorderY > 0 && BorderX <= WIDTH - 1 && BorderY <= HEIGHT - 1 && !(StartX == BorderX && StartY == BorderY));

    cv::imshow("Target", res);
    cv::imwrite("squareBorder.png", res);
    cv::waitKey(0);
    
    return 0;
}