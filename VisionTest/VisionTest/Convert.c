static void ConvertPixelRGB2HSV(unsigned short pixel,
                                 unsigned char* hue,
                                 unsigned char* sat,
                                 unsigned char* val)
{
    /* Expand RGB565 channels to the 8-bit range by bit replication. */
    unsigned char r = (unsigned char)(((pixel >> 8) & 0xF8U) |
                                      ((pixel >> 13) & 0x07U));
    unsigned char g = (unsigned char)(((pixel >> 3) & 0xFCU) |
                                      ((pixel >> 9) & 0x03U));
    unsigned char b = (unsigned char)(((pixel << 3) & 0xF8U) |
                                      ((pixel >> 2) & 0x07U));

    unsigned char max = r;
    unsigned char min = r;
    if (g > max) max = g;
    if (b > max) max = b;
    if (g < min) min = g;
    if (b < min) min = b;

    unsigned char delta = (unsigned char)(max - min);
    unsigned char h = 0U;

    if (delta != 0U) {
        unsigned short numerator;
        unsigned char offset;

        if (max == r) {
            /* H = 30 * (G - B) / delta in OpenCV's 0..179 scale. */
            if (g >= b) {
                numerator = (unsigned short)(30U * (unsigned short)(g - b));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = offset;
            } else {
                numerator = (unsigned short)(30U * (unsigned short)(b - g));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = (unsigned char)(180U - offset);
            }
        } else if (max == g) {
            /* H = 60 + 30 * (B - R) / delta. */
            if (b >= r) {
                numerator = (unsigned short)(30U * (unsigned short)(b - r));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = (unsigned char)(60U + offset);
            } else {
                numerator = (unsigned short)(30U * (unsigned short)(r - b));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = (unsigned char)(60U - offset);
            }
        } else {
            /* H = 120 + 30 * (R - G) / delta. */
            if (r >= g) {
                numerator = (unsigned short)(30U * (unsigned short)(r - g));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = (unsigned char)(120U + offset);
            } else {
                numerator = (unsigned short)(30U * (unsigned short)(g - r));
                offset = (unsigned char)((numerator + delta / 2U) / delta);
                h = (unsigned char)(120U - offset);
            }
        }

        if (h >= 180U) h = (unsigned char)(h - 180U);
    }

    *hue = h;
    *sat = (unsigned char)((max == 0U) ? 0U :
                           (delta * 255U + max / 2U) / max);
    *val = max;
}
