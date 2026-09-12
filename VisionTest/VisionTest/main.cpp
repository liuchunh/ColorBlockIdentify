#include <iostream>
#include <format>
#include <opencv2/opencv.hpp>
#include <fstream>

constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;

class Coordinate {
private:
    size_t x, y;

public:
    Coordinate(): x(0), y(0) {}

	Coordinate(int m_x, int m_y) : x(m_x), y(m_y) {}

    std::string GetCoordinateString() const {
        return std::format("[{}, {}]", x, y);
	}

    bool operator==(const Coordinate& other) const {
        return x == other.x && y == other.y;
	}
};

template<>
struct std::formatter<Coordinate>: std::formatter<std::string> {
    auto format(const Coordinate& coord, format_context& ctx) const{
        return std::formatter<std::string>::format(coord.GetCoordinateString(), ctx);
    }
};

static bool Generate(size_t width, size_t height, size_t k, size_t b) {
    cv::Mat src(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int lineY = k * x + b;

            if (y < lineY) {
                src.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
            }
        }
    }

    return cv::imwrite("output.png", src);
}

// 逆时针
const int RelativePos[][2] = {
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}
};
const int RelativePosSize = sizeof(RelativePos) / sizeof(RelativePos[0]);

int EightNeighborhood() {
    if (!Generate(WIDTH, HEIGHT, -3, 1200)) {
        std::cerr << "Failed to generate image" << std::endl;
        return 1;
    }

    // 使用八邻域来分辨分界线
	cv::Mat src = cv::imread("output.png");
    if (src.empty()) {
        assert(0 && "Failed to read image");
        return 1;
    }
    
    int height = src.rows;
    int width = src.cols;

    // 从底部右边开始扫描，找到第一个黑色像素点，记录其坐标
	int FirstBlackX = WIDTH - 1;
    while (FirstBlackX >= 0 && src.at<cv::Vec3b>(height - 1, FirstBlackX) == cv::Vec3b(255, 255, 255)) {
        FirstBlackX --;
	}

	std::cout << "First black pixel found at: (" << FirstBlackX << ", " << HEIGHT - 1 << ")" << std::endl;

    int BorderX = FirstBlackX;
    int BorderY = HEIGHT - 1;


    while (BorderX >= 0 && BorderY >= 0 && BorderX <= WIDTH - 1 && BorderY <= HEIGHT - 1) {
        bool Found = false;

        for (int i = 0; i < RelativePosSize - 1; i ++ ){
            int CurX = BorderX + RelativePos[i][0];
            int CurY = BorderY + RelativePos[i][1];

            int NxtX = BorderX + RelativePos[i + 1][0];
            int NxtY = BorderY + RelativePos[i + 1][1];

            if (CurX > 0 && CurY > 0 && CurX <= WIDTH - 1 && CurY <= HEIGHT - 1) {
                // 如果当前像素点是黑色，且下一个像素点是白色，或者当前像素点是白色，且下一个像素点是黑色，则说明找到了边界
                if ( (src.at<cv::Vec3b>(CurY, CurX) == cv::Vec3b(0, 0, 0) && src.at<cv::Vec3b>(NxtY, NxtX) == cv::Vec3b(255, 255, 255)) ||
                     (src.at<cv::Vec3b>(CurY, CurX) == cv::Vec3b(255, 255, 255) && src.at<cv::Vec3b>(NxtY, NxtX) == cv::Vec3b(0, 0, 0))  ) {
                    Found = true;
                    BorderX = CurX;
                    BorderY = CurY;
                    break;
                }
            }
        }

        if (!Found) {
            break;
        }
        else{
            src.at<cv::Vec3b>(BorderY, BorderX) = cv::Vec3b(0, 0, 255);
        }
    }
    cv::imshow("Result", src);
    cv::imwrite("result.png", src);
    cv::waitKey(0);
	return 0;
}

int GenerateSquare(size_t Imgwidth, size_t Imgheight, cv::Rect rect, cv::Scalar color = cv::Scalar(0, 0, 255)) {
    const cv::Vec3b InterferenceColor(255, 0, 0); // 蓝色
    
    cv::Mat src(Imgheight, Imgwidth, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::rectangle(src, rect, color, -1);
    
    cv::Point2i iLT((rect.tl() + rect.br()) / 2); // i prefix for interference
    cv::Point2i iRB(rect.br() + cv::Point2i(rect.width / 2, rect.height / 2));
    cv::rectangle(src, cv::Rect(iLT, iRB), InterferenceColor, -1);

    return cv::imwrite("square.png", src);
}

std::ofstream RunTime("log.txt");

int sign(int x){
    return (x > 0) - (x < 0);
}

int main(){
    if (!GenerateSquare(WIDTH, HEIGHT, cv::Rect(100, 100, 200, 200))) {
        std::cerr << "Failed to generate square" << std::endl;
        return 1;
    }

    cv::Mat src = cv::imread("square.png");
    if (src.empty()) {
        assert(0 && "Failed to read image");
        return 1;    
    }

    const cv::Vec3b TargetColor(0, 0, 255); // 红色
    const cv::Vec3b BorderColor(0, 255, 0); // 绿色

    size_t BorderX = 0, BorderY = 0;

    bool FirstRedPixelFound = false;

    // 第一帧先全局遍历
    // 往后就采用第一帧的结果使用八邻域来分辨边界
    // 先从左下角找吧, 这样按照逆时针的八邻域搜索顺序就可以找到边界了
    for (int x = 0; x < src.cols; x ++) {
        for (int y = src.rows - 1; y >= 0; y --) {
            if (src.at<cv::Vec3b>(y, x) == TargetColor) {
                BorderX = x;
                BorderY = y;
                src.at<cv::Vec3b>(BorderY, BorderX) = BorderColor;
                // 找到红色像素点，开始八邻域搜索
                FirstRedPixelFound = true;
            }
        }

        if (FirstRedPixelFound) {
            break;
        }
    }

    assert(BorderX != 0 && BorderY != 0);
    std::cout << std::format("First Red pixel found at: ({}, {})", BorderX, BorderY) << std::endl;

    while (BorderX <= WIDTH - 1 && BorderY <= HEIGHT - 1 && BorderX > 0 && BorderY > 0){
        bool Found = false;

        for (int i = 0; i < RelativePosSize - 1; i ++ ){
            int CurX = BorderX + RelativePos[i][0];
            int CurY = BorderY + RelativePos[i][1];

            int NxtX = BorderX + RelativePos[i + 1][0];
            int NxtY = BorderY + RelativePos[i + 1][1];

            if (CurX > 0 && CurY > 0 && CurX <= WIDTH - 1 && CurY <= HEIGHT - 1) {
                if ( 
                //    (src.at<cv::Vec3b>(CurY, CurX) == TargetColor && src.at<cv::Vec3b>(NxtY, NxtX) != TargetColor) ||
                //    (src.at<cv::Vec3b>(CurY, CurX) != TargetColor && src.at<cv::Vec3b>(NxtY, NxtX) == TargetColor) || 
                    (src.at<cv::Vec3b>(CurY, CurX) == TargetColor && src.at<cv::Vec3b>(NxtY, NxtX) == TargetColor)
                 ) {
                    Found = true;
                    RunTime << std::format("Border Detected at: ({}, {})\n", BorderX, BorderY);
                    BorderX = CurX;
                    BorderY = CurY;
                    break;
                }
            }
        }

        if (!Found) {
            break;
        }
        else{
            src.at<cv::Vec3b>(BorderY, BorderX) = BorderColor;
        }
    }

    cv::imshow("Result", src);
    cv::imwrite("squareBorder.png", src);

    cv::waitKey(0);

    return 0;
}