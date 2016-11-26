#define FORMAT_GRAY     1
#define FORMAT_RGB      3
#define FORMAT_BAYERGRBG 18

__kernel void transform (__global uchar* src, __global uchar* dst)
{
    int row = get_global_id(0);

#if DST_FLIP_H
#define ROW_OFFSET (SRC_HEIGHT - DST_YOFF - row)
#else
#define ROW_OFFSET (DST_YOFF + row)
#endif
#if DST_FLIP_V
#define COL_OFFSET DST_WIDTH + DST_XOFF
#define READ_STEP -1
#else
#define COL_OFFSET DST_XOFF
#define READ_STEP 1
#endif

    __global uchar* read = src + (SRC_WIDTH * ROW_OFFSET + COL_OFFSET) * SRC_FORMAT_PIXEL_SIZE;
    int read_step = READ_STEP;

    __global uchar* write = dst + (DST_WIDTH * row) * DST_FORMAT_PIXEL_SIZE;
    

#if SRC_FORMAT == FORMAT_BAYERGRBG
    __global uchar* read_second_line;

    if(row + DST_YOFF == SRC_HEIGHT - 1)
        read_second_line = read - SRC_WIDTH * SRC_FORMAT_PIXEL_SIZE;
    else
        read_second_line = read + SRC_WIDTH * SRC_FORMAT_PIXEL_SIZE;

    bool evenrow = row != 0 && row % 2 == 0;
    bool evencol = false;
#endif

    int X, R, G, B, G2;

    for(int i = 0; i < DST_WIDTH; i++)
    {


        #if SRC_FORMAT == DST_FORMAT
            for(int j = 0; j <= SRC_FORMAT_PIXEL_SIZE; j++)
            {
                *writer++ = *read;
                read += read_step;
            }

        #elif SRC_FORMAT == FORMAT_RGB && DST_FORMAT == FORMAT_GRAY
            R = *read; read += read_step;
            G = *read; read += read_step;
            B = *read; read += read_step;

            *write++ = (R * 77 + G * 151 + B * 28) >> 8;
        #elif SRC_FORMAT == FORMAT_GRAY && DST_FORMAT == FORMAT_RGB

                X = *read;
                read += read_step;
                *write++ = X;
                *write++ = X;
                *write++ = X;

        #elif SRC_FORMAT == FORMAT_BAYERGRBG
            // PSMove output is in the following Bayer format (GRBG):
            //
            // G R G R G R
            // B G B G B G
            // G R G R G R
            // B G B G B G
            //
            // This is the normal Bayer pattern shifted left one place.
            
            if(i + DST_XOFF == SRC_WIDTH - 1)
                read_step *= -1;

            if(!evenrow && !evencol)
            {
                G = *read;
                read += read_step;
                R = *read;

                B = *read_second_line;
                read_second_line += read_step;
                G2 = *read_second_line;
            }
            else if(!evenrow && evencol)
            {
                R = *read;
                read += read_step;
                G = *read;

                G2 = *read_second_line;
                read_second_line += read_step;
                B = *read_second_line;
            }
            else if(evenrow && !evencol)
            {
                B = *read;
                read += read_step;
                G = *read;

                G2 = *read_second_line;
                read_second_line += read_step;
                R = *read_second_line;
            }
            else if(evenrow && evencol)
            {
                G = *read;
                read += read_step;
                B = *read;

                R = *read_second_line;
                read_second_line += read_step;
                G2 = *read_second_line;
            }

            #if DST_FORMAT == FORMAT_GRAY
                *write++ = (R * 77 + (G+G2)/2 * 151 + B * 28) >> 8;
            #elif DST_FORMAT == FORMAT_RGB
                *write++ = R;
                *write++ = (G + G2 / 2;
                *write++ = B;
            #endif

            evencol = !evencol;
        #endif  
    }
}