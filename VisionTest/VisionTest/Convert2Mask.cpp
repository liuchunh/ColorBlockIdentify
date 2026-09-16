#include <iostream>
#include <opencv2/opencv.hpp>

bool matEquals(const cv::Mat& a, const cv::Mat& b) {
	if (a.size() != b.size() || a.type() != b.type()) {
		return false;
	}
	cv::Mat diff;
	cv::compare(a, b, diff, cv::CMP_NE);
	return cv::countNonZero(diff.reshape(1)) == 0;
}


static cv::Vec3b ConvertPixelBGR2HSV(const cv::Vec3b& pixel){
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

int main() {
	cv::Mat src = cv::imread("C:\\Users\\Lenovo\\Pictures\\test4.jpg");
	if (src.empty()) {
		std::cerr << "Error: Could not open or find the image!" << std::endl;
		return 1;
	}

	cv::Mat hsv = ConvertImg2HSV(src);
	cv::Mat dst = src.clone();
	cv::cvtColor(dst, dst, cv::COLOR_BGR2HSV);

	if (matEquals(dst, hsv)){
		std::cout << "Match!" << std::endl;
	}

	cv::imshow("Original", src);
	cv::imshow("Custom HSV", hsv);
	cv::imshow("OpenCV HSV", dst);

	cv::waitKey(0);
	return 0;
}