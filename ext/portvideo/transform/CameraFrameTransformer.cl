#define FORMAT_GRAY     1
#define FORMAT_RGB      3
#define FORMAT_BAYERGRBG 18

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

typedef struct pixel_reader
{
    __global uchar* rbuf;
    __global uchar* rbuf2;

    bool is_evenrow;
    bool is_evencol;
} pixel_reader;

void pt_init_offset(pixel_reader* reader, pixel_reader* main_reader, int offset)
{
    reader->rbuf = main_reader->rbuf + offset;
    reader->is_evenrow = !main_reader->is_evenrow;
    reader->is_evencol = !main_reader->is_evencol;

    int rows_added = offset / SRC_WIDTH;
    if(rows_added != 0)
    {
        bool is_even_row_added = rows_added % 2 == 0;
        reader->is_evenrow = reader->is_evenrow == is_even_row_added;
    }

    int cols_added = offset % SRC_WIDTH;
    if(cols_added != 0)
    {
        bool is_even_cols_added = cols_added % 2 == 0;
        reader->is_evencol = reader->is_evencol == is_even_cols_added;
    }

#if SRC_FORMAT == FORMAT_BAYERGRBG
    //rbuf2 is always one line below
    reader->rbuf2 = reader->rbuf + SRC_WIDTH * SRC_FORMAT_PIXEL_SIZE;
#endif
}

void pt_init(pixel_reader* reader, __global uchar* buffer, int seek_rows, int seek_cols)
{
    reader->rbuf = buffer + (SRC_WIDTH * seek_rows + seek_cols) * SRC_FORMAT_PIXEL_SIZE;
    reader->is_evenrow = seek_rows != 0 && seek_rows % 2 == 0;
    reader->is_evencol = seek_cols != 0 && seek_cols % 2 == 0;

#if SRC_FORMAT == FORMAT_BAYERGRBG
    //rbuf2 is always one line below
    reader->rbuf2 = reader->rbuf + SRC_WIDTH * SRC_FORMAT_PIXEL_SIZE;
#endif
}

void pt_next(pixel_reader* reader)
{
    reader->rbuf += READ_STEP * SRC_FORMAT_PIXEL_SIZE;
    reader->is_evencol = !reader->is_evencol;

#if SRC_FORMAT == FORMAT_BAYERGRBG
    reader->rbuf2 += READ_STEP * SRC_FORMAT_PIXEL_SIZE;
#endif
}

__kernel void transform (__global uchar* src, __global uchar* dst, __global short* dmap)
{
    int row = get_global_id(0);

    pixel_reader reader;
    pt_init(&reader, src, ROW_OFFSET, COL_OFFSET);

    __global uchar* writer = dst + DST_WIDTH * row * DST_FORMAT_PIXEL_SIZE;

#if DMAP
    __global short* read_offset = dmap + row * DST_WIDTH;
#endif

    int X, R, G, B, G2;

    for(int i = 0; i < DST_WIDTH; i++)
    {
        pixel_reader offset_reader;
#if DMAP
        pt_init_offset(&offset_reader, &reader, *read_offset);
#else
        offset_reader = reader;
#endif

        #if SRC_FORMAT == DST_FORMAT

            for(int j = 0; j <= SRC_FORMAT_PIXEL_SIZE; j++)
                writer[0] = offset_reader.rbuf[0];

        #elif SRC_FORMAT == FORMAT_RGB && DST_FORMAT == FORMAT_GRAY

            R = offset_reader.rbuf[0];
            G = offset_reader.rbuf[1];
            B = offset_reader.rbuf[2];

            writer->wbuf[0] = (R * 77 + G * 151 + B * 28) >> 8;

        #elif SRC_FORMAT == FORMAT_GRAY && DST_FORMAT == FORMAT_RGB

            writer[0] = offset_reader.rbuf[0];
            writer[1] = offset_reader.rbuf[0];
            writer[2] = offset_reader.rbuf[0];

        #elif SRC_FORMAT == FORMAT_BAYERGRBG

            // PSMove output is in the following Bayer format (GRBG):
            //
            // G R G R G R
            // B G B G B G
            // G R G R G R
            // B G B G B G
            //
            // This is the normal Bayer pattern shifted left one place.

            if(!offset_reader.is_evenrow && !offset_reader.is_evencol)
            {
                G = offset_reader.rbuf[0];
                R = offset_reader.rbuf[1];

                B = offset_reader.rbuf2[0];
                G2 = offset_reader.rbuf2[1];
            }
            else if(!offset_reader.is_evenrow && offset_reader.is_evencol)
            {
                R = offset_reader.rbuf[0];
                G = offset_reader.rbuf[1];

                G2 = offset_reader.rbuf2[0];
                B = offset_reader.rbuf2[1];
            }
            else if(offset_reader.is_evenrow && !offset_reader.is_evencol)
            {
                B = offset_reader.rbuf[0];
                G = offset_reader.rbuf[1];

                G2 = offset_reader.rbuf2[0];
                R = offset_reader.rbuf2[1];
            }
            else if(offset_reader.is_evenrow && offset_reader.is_evencol)
            {
                G = offset_reader.rbuf[0];
                B = offset_reader.rbuf[1];

                R = offset_reader.rbuf2[0];
                G2 = offset_reader.rbuf2[1];
            }

            #if DST_FORMAT == FORMAT_GRAY
                writer[0] = (R * 77 + (G+G2)/2 * 151 + B * 28) >> 8;
            #elif DST_FORMAT == FORMAT_RGB
                writer[0] = R;
                writer[0] = (G + G2 / 2;
                writer[0] = B;
            #endif

        #endif  

        pt_next(&reader);
        writer += DST_FORMAT_PIXEL_SIZE;

#if DMAP
        read_offset++;
#endif
    }
}