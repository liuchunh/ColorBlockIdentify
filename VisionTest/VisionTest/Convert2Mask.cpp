#include <iostream>
#include <opencv2/opencv.hpp>

static cv::Mat ConvertImg2HSV(const cv::Mat& src) {
	int width = src.cols;
	int height = src.rows;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

		}
	}
}

int main() {
	cv::Mat src = cv::imread("CCLTest.jpg");
	if (src.empty()) {
		std::cerr << "Error: Could not open or find the image!" << std::endl;
		return 1;
	}

	cv::Mat hsv;
	cv::Mat dst = src.clone();
	cv::cvtColor(dst, dst, cv::COLOR_BGR2HSV);
}