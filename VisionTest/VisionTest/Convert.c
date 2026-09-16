static void ConvertPixelRGB2HSV(unsigned short pixel,
                                 unsigned char* hue,
                                 unsigned char* sat,
                                 unsigned char* val)
{
    int r5 = (pixel >> 11) & 0x1F;
    int g6 = (pixel >> 5) & 0x3F;
    int b5 = pixel & 0x1F;

    /* Expand RGB565 channels to the 8-bit range by bit replication. */
    int r = (r5 << 3) | (r5 >> 2);
    int g = (g6 << 2) | (g6 >> 4);
    int b = (b5 << 3) | (b5 >> 2);

    int max = r;
    int min = r;
    if (g > max) max = g;
    if (b > max) max = b;
    if (g < min) min = g;
    if (b < min) min = b;

    int delta = max - min;
    int h = 0;

    if (delta != 0) {
        int numerator;

        if (max == r) {
            /* H = 30 * (G - B) / delta in OpenCV's 0..179 scale. */
            numerator = 30 * (g - b);
            if (numerator >= 0) {
                h = (numerator + delta / 2) / delta;
            } else {
                h = (numerator - delta / 2) / delta;
            }
        } else if (max == g) {
            /* H = 60 + 30 * (B - R) / delta. */
            numerator = 30 * (b - r);
            if (numerator >= 0) {
                h = 60 + (numerator + delta / 2) / delta;
            } else {
                h = 60 + (numerator - delta / 2) / delta;
            }
        } else {
            /* H = 120 + 30 * (R - G) / delta. */
            numerator = 30 * (r - g);
            if (numerator >= 0) {
                h = 120 + (numerator + delta / 2) / delta;
            } else {
                h = 120 + (numerator - delta / 2) / delta;
            }
        }

        if (h < 0) h += 180;
        if (h >= 180) h -= 180;
    }

    *hue = (unsigned char)h;
    *sat = (unsigned char)((max == 0) ? 0 : (delta * 255 + max / 2) / max);
    *val = (unsigned char)max;
}
