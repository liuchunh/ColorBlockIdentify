#include <iostream>
#include <opencv2/opencv.hpp>

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

cv::Scalar HSVLower(26, 43, 46);
cv::Scalar HSVUpper(34, 255, 255);

int main() {
	cv::Mat src = cv::imread("D:\\Executive\\Ordinary\\Identify\\001.png");
	if (src.empty()) {
		std::cerr << "Error: Could not open or find the image!" << std::endl;
		return 1;
	}

	cv::Mat hsv = ConvertImg2HSV(src);
	cv::Mat dst = src.clone();
	cv::cvtColor(dst, dst, cv::COLOR_BGR2HSV);    
    cv::Mat mask = cv::Mat::zeros(src.size(), CV_8UC1);
    cv::Mat ownMask = cv::Mat::zeros(src.size(), CV_8UC1);

    cv::inRange(dst, HSVLower, HSVUpper, mask);
    ImgInHsvRange(hsv, HSVLower, HSVUpper, ownMask, cv::Scalar(0, 0, 0));

	cv::imshow("Original", src);
    cv::imshow("OpenCV Mask", mask);
    cv::imshow("Own Mask", ownMask);

    cv::waitKey(0);
	return 0;
}