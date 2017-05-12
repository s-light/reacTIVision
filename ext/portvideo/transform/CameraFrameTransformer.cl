#define FORMAT_GRAY     1
#define FORMAT_RGB      3
#define FORMAT_YUYV    10
#define FORMAT_BAYERGRBG 18

#if DST_FLIP_H
#define ROW_OFFSET (SRC_HEIGHT - DST_YOFF - row)
#else
#define ROW_OFFSET (DST_YOFF + row)
#endif
#if DST_FLIP_V
#define COL_OFFSET SRC_WIDTH - DST_XOFF
#define READ_STEP -1
#else
#define COL_OFFSET DST_XOFF
#define READ_STEP 1
#endif

#define SAT(c) \
if (c & (~255)) { if (c < 0) c = 0; else c = 255; }

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
    reader->is_evenrow = main_reader->is_evenrow;
    reader->is_evencol = main_reader->is_evencol;

    if(offset > 0)
    {
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
    }

#if SRC_FORMAT == FORMAT_BAYERGRBG
    //rbuf2 is always one line below
    reader->rbuf2 = reader->rbuf + SRC_WIDTH * SRC_FORMAT_PIXEL_SIZE;
#endif
}

void pt_init(pixel_reader* reader, __global uchar* buffer, int seek_rows, int seek_cols)
{
    reader->rbuf = buffer + (SRC_WIDTH * seek_rows + seek_cols) * SRC_FORMAT_PIXEL_SIZE;
    //first row is odd. is an odd numer of rows added the resulting row is even
    reader->is_evenrow = seek_rows != 0 && seek_rows % 2 != 0;
    reader->is_evencol = seek_cols != 0 && seek_cols % 2 != 0;

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

    __global uchar* write = dst + DST_WIDTH * row * DST_FORMAT_PIXEL_SIZE;

#if DMAP
    __global short* read_offset = dmap + row * DST_WIDTH;
#endif

    int X, R, G, B, G2, Y1, U, Y2, V, C;

#if SRC_FORMAT == FORMAT_YUYV
    for(int i = 0; i < DST_WIDTH/2; i++)
    {
#else
    for(int i = 0; i < DST_WIDTH; i++)
    {
#endif
        pixel_reader offset_reader;
#if DMAP
        pt_init_offset(&offset_reader, &reader, *read_offset);
#else
        pt_init_offset(&offset_reader, &reader, 0);
#endif

        #if SRC_FORMAT == DST_FORMAT

            for(int j = 0; j < SRC_FORMAT_PIXEL_SIZE; j++)
                write[j] = offset_reader.rbuf[j];

        #elif SRC_FORMAT == FORMAT_RGB && DST_FORMAT == FORMAT_GRAY

            R = offset_reader.rbuf[0];
            G = offset_reader.rbuf[1];
            B = offset_reader.rbuf[2];

            write[0] = (R * 77 + G * 151 + B * 28) >> 8;

        #elif SRC_FORMAT == FORMAT_GRAY && DST_FORMAT == FORMAT_RGB

            write[0] = offset_reader.rbuf[0];
            write[1] = offset_reader.rbuf[0];
            write[2] = offset_reader.rbuf[0];

        #elif SRC_FORMAT == FORMAT_YUYV && DST_FORMAT == FORMAT_GRAY
            // just a simpel test
            // this should not crash but give 'random' results..
            Y1 = offset_reader.rbuf[0];
            write[0] = Y1;

        #elif SRC_FORMAT == FORMAT_YUYV && DST_FORMAT == FORMAT_RGB
            // just a simpel test
            // this should not crash but give 'random' results..
            write[0] = offset_reader.rbuf[0];
            write[1] = offset_reader.rbuf[1];
            write[2] = offset_reader.rbuf[2];

        // #elif SRC_FORMAT == FORMAT_YUYV && DST_FORMAT == FORMAT_GRAY
        //     // based on CameraEngine.cpp line 221
        //     Y1 = offset_reader.rbuf[0];
        //     // U  = offset_reader.rbuf[1] - 128;
        //     Y2 = offset_reader.rbuf[2];
        //     // V  = offset_reader.rbuf[3] - 128;
        //
        //     write[0] = Y1;
        //     // write[1] = Y2;
        //
        // #elif SRC_FORMAT == FORMAT_YUYV && DST_FORMAT == FORMAT_RGB
        //     // based on CameraEngine.cpp line 255 and line 324
        //     Y1 = offset_reader.rbuf[0];
        //     U  = offset_reader.rbuf[1] - 128;
        //     Y2 = offset_reader.rbuf[2];
        //     V  = offset_reader.rbuf[3] - 128;
        //
        //     // subsample 1
        //     C = 298*(Y1 - 16);
        //     R = (C + 409*V + 128) >> 8;
        //     G = (C - 100*U - 208*V + 128) >> 8;
        //     B = (C + 516*U + 128) >> 8;
        //
        //     SAT(R);
        //     SAT(G);
        //     SAT(B);
        //
        //     write[0] = R;
        //     write[1] = G;
        //     write[2] = B;
        //
        //     // // subsample 2
        //     // C = 298*(Y2 - 16);
        //     // R = (C + 409*V + 128) >> 8;
        //     // G = (C - 100*U - 208*V + 128) >> 8;
        //     // B = (C + 516*U + 128) >> 8;
        //     //
        //     // SAT(R);
        //     // SAT(G);
        //     // SAT(B);
        //     //
        //     // write[3] = R;
        //     // write[4] = G;
        //     // write[5] = B;

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
                write[0] = (R * 77 + (G + G2)/2 * 151 + B * 28) >> 8;
            #elif DST_FORMAT == FORMAT_RGB
                write[0] = R;
                write[1] = (G + G2) / 2;
                write[2] = B;
            #endif

        #endif

        pt_next(&reader);
        write += DST_FORMAT_PIXEL_SIZE;

#if DMAP
        read_offset++;
#endif
    }
}
