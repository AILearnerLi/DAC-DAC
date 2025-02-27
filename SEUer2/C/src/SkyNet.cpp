#include "SkyNet.h"

static layer config[layer_count] = {
{ "conv0", 320,160,32, 320,160,32, 0,0,0 },  //conv0
{ "conv1", 320,160,32, 320,160,32, 3,1,1 },  //conv1
{ "conv2", 320,160,32, 320,160,64, 1,1,0 },  //conv2
{ "pool1", 320,160,64, 160,80,64,  2,2,0 },  //pool1
{ "conv3", 160,80,64,  160,80,64,  3,1,1 },  //conv3
{ "conv4", 160,80,64,  160,80,96,  1,1,0 },  //conv4
{ "pool2", 160,80,96,  80,40,96,   2,2,0 },  //pool2
{ "conv5", 80,40,96,   80,40,96,   3,1,1 },  //conv5
{ "conv6", 80,40,96,   80,40,192,  1,1,0 },  //conv6
{ "reorg", 80,40,192,  40,20,768,  2,2,0 },  //reorg
{ "pool3", 80,40,192,  40,20,192,  2,2,0 },  //pool3
{ "conv7", 40,20,192,  40,20,192,  3,1,1 },  //conv7
{ "conv8", 40,20,192,  40,20,384,  1,1,0 },  //conv8
{ "conv9", 40,20,384,  40,20,384,  3,1,1 },  //conv9
{ "conv10",40,20,384,  40,20,512,  1,1,0 },  //conv10
{ "cat",   40,20,192,  40,20,1280, 0,0,0 },  //concat
{ "conv11",40,20,1280, 40,20,1280, 3,1,1 },  //conv11
{ "conv12",40,20,1280, 40,20,96,   1,1,0 },  //conv12
{ "conv13",40,20,96,   40,20,32,   1,1,0 },  //conv13
};

ADT FM1[32][43][83];
ADT FM2[32][43][83];
ADT FM3[32][43][83];
RDT FM4[32][43][83];

WDT WBUF[4][32][32];
BDT BBUF[3][32];
MDT MBUF[3][32];

void REORG(ADT32 *ifm, ADT IFM[32][43][83], int Cx, ap_uint<3> Rx)
{
#pragma HLS ARRAY_PARTITION variable=IFM dim=1 complete
    ap_uint<20> ifm_index;
    for (ap_uint<7> h = 1; h <= 40; h++)
    {
        for (ap_uint<7> w = 1; w <= 80; w++)
        {
#pragma HLS LOOP_FLATTEN
#pragma HLS PIPELINE II=1
            ap_uint<7> _h = h + (h >= 21);
            ap_uint<7> _w = w + (w >= 41);
            ap_uint<2> bias_h = (_h >= 22) + (!Rx[1]);
            ap_uint<2> bias_w = (_w >= 42) + (!Rx[0]);
            int h_ = 2 * _h - bias_h;
            int w_ = 2 * _w - bias_w;
            ap_uint<20> ifm_index = Cx * 83 * 163 + h_ * 163 + w_;
            ADT32 DATA = ifm[ifm_index];
            for (ap_uint<7> c = 0; c < 32; c++)
            {
#pragma HLS UNROLL
                IFM[c][_h][_w] = DATA.range(8*c+7, 8*c);
            }
        }
    }
}

BDT clamp_BDT(RDT x, BDT min, BDT max)
{
#pragma HLS INLINE
    BDT y;
    if(x<min) y=min;
    else if(x>max) y=max;
    else y = x;
    return y;
}

ap_int<18> MAC16(WDT w0,  ADT b0,
					  WDT w1,  ADT b1,
					  WDT w2,  ADT b2,
					  WDT w3,  ADT b3,
					  WDT w4,  ADT b4,
					  WDT w5,  ADT b5,
					  WDT w6,  ADT b6,
					  WDT w7,  ADT b7,
					  WDT w8,  ADT b8,
					  WDT w9,  ADT b9,
					  WDT w10, ADT b10,
					  WDT w11, ADT b11,
					  WDT w12, ADT b12,
					  WDT w13, ADT b13,
					  WDT w14, ADT b14,
					  WDT w15, ADT b15)
{
#pragma HLS INLINE off
	ap_int<14> mul0, mul1, mul2,  mul3,  mul4,  mul5,  mul6,  mul7;
	ap_int<14> mul8, mul9, mul10, mul11, mul12, mul13, mul14, mul15;
	ap_int<15> add0, add1, add2, add3, add4,  add5, add6, add7;
	ap_int<16> add8, add9, add10, add11;
    ap_int<17> add12, add13;
    ap_int<18> res;

    mul0 = w0 * b0;
    mul1 = w1 * b1;
    mul2 = w2 * b2;
    mul3 = w3 * b3;
    mul4 = w4 * b4;
    mul5 = w5 * b5;
    mul6 = w6 * b6;
    mul7 = w7 * b7;
    mul8 = w8 * b8;
    mul9 = w9 * b9;
    mul10 = w10 * b10;
    mul11 = w11 * b11;
    mul12 = w12 * b12;
    mul13 = w13 * b13;
    mul14 = w14 * b14;
    mul15 = w15 * b15;

    add0 = mul0 + mul1;
    add1 = mul2 + mul3;
    add2 = mul4 + mul5;
    add3 = mul6 + mul7;
    add4 = mul8 + mul9;
    add5 = mul10 + mul11;
    add6 = mul12 + mul13;
    add7 = mul14 + mul15;

    add8 = add0 + add1;
    add9 = add2 + add3;
    add10 = add4 + add5;
    add11 = add6 + add7;

    add12 = add8 + add9;
    add13 = add10 + add11;

    res = add12 + add13;

    return res;
 }

void LOAD_W1x1(WDT WBUF1x1[32][32], WDT W1x1[32][16], int CI)
{
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=WBUF1x1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=W1x1 complete dim=1
#pragma HLS LATENCY min=8 max=8
	for(int ci=0; ci<16; ci++){
#pragma HLS UNROLL
		for(int co=0; co<32; co++){
#pragma HLS UNROLL
			W1x1[co][ci] = WBUF1x1[co][ci+CI];
		}
	}
}


//modify this function, sharepe share weight buf, r-prarlle
void SHARECONV(const ADT IFM[32][43][83], RDT OFM[32][43][83], WDT WBUF[32][32], uint1 mode, uint1 CI)
{
#pragma HLS ARRAY_PARTITION variable=OFM dim=1 complete
#pragma HLS ARRAY_PARTITION variable=IFM dim=1 complete
#pragma HLS ARRAY_PARTITION variable=WBUF dim=1 complete

WDT W1x1[32][16];
#pragma HLS ARRAY_PARTITION variable=W1x1 dim=1 complete
#pragma HLS ARRAY_PARTITION variable=W1x1 dim=2 complete

    ADT window_buffer[32][3][3];
#pragma HLS ARRAY_PARTITION variable=window_buffer complete dim=0
    ADT line_buffer[32][3][43];
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=2
    WDT zero=0;
    ADT dzero=0;
if (mode==0){
	LOAD_W1x1(WBUF, W1x1, 0);
    for (int row_in = 0; row_in < 83; row_in++)
    {
        for (int col_in = 0; col_in < 43; col_in++)
        {
        #pragma HLS LOOP_FLATTEN
        #pragma HLS PIPELINE II=1
            for (int c = 0; c < 32; c++)
            {
            #pragma HLS UNROLL
                ADT read_in = IFM[c][col_in][row_in];
                line_buffer[c][row_in % 3][col_in] = read_in;

                window_buffer[c][2][2] = read_in;
                window_buffer[c][1][2] = line_buffer[c][(row_in + 2) % 3][col_in];
                window_buffer[c][0][2] = line_buffer[c][(row_in + 1) % 3][col_in];

                if (row_in >= 2 && col_in >= 2)
                {
                    ap_int<18> res = MAC16(
                    		W1x1[c][0], window_buffer[c][0][0],
							W1x1[c][3], window_buffer[c][0][1],
							W1x1[c][6], window_buffer[c][0][2],
							W1x1[c][1], window_buffer[c][1][0],
							W1x1[c][4], window_buffer[c][1][1],
							W1x1[c][7], window_buffer[c][1][2],
							W1x1[c][2], window_buffer[c][2][0],
							W1x1[c][5], window_buffer[c][2][1],
							W1x1[c][8], window_buffer[c][2][2],
						zero, dzero,
						zero, dzero,
						zero, dzero,
						zero, dzero,
						zero, dzero,
						zero, dzero,
						zero, dzero);

                    OFM[c][col_in - 1][row_in - 1] = res > rmax ? RDT(rmax) : res < rmin ? RDT(rmin) : RDT(res);
                }

                for (int r = 0; r < 3; r++)
                {
                #pragma HLS UNROLL
                    window_buffer[c][r][0] = window_buffer[c][r][1];
                    window_buffer[c][r][1] = window_buffer[c][r][2];
                }
            }
        }
    }
}
else{
  	for(int ci=0; ci<=CI; ci++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=2
		LOAD_W1x1(WBUF, W1x1, ci*16);
		for(int h=1; h<42; h++)
		{
			for(int w=1; w<82; w++)
			{
#pragma HLS PIPELINE II=1
				for(int co=0; co<32; co++)
				{
#pragma HLS UNROLL
					ap_int<20> res1 = OFM[co][h][w];
					res1 += MAC16(
									W1x1[co][0],   IFM[ci*16+0][h][w],
									W1x1[co][1],   IFM[ci*16+1][h][w],
									W1x1[co][2],   IFM[ci*16+2][h][w],
									W1x1[co][3],   IFM[ci*16+3][h][w],
									W1x1[co][4],   IFM[ci*16+4][h][w],
									W1x1[co][5],   IFM[ci*16+5][h][w],
									W1x1[co][6],   IFM[ci*16+6][h][w],
									W1x1[co][7],   IFM[ci*16+7][h][w],
									W1x1[co][8],   IFM[ci*16+8][h][w],
									W1x1[co][9],   IFM[ci*16+9][h][w],
									W1x1[co][10],  IFM[ci*16+10][h][w],
									W1x1[co][11],  IFM[ci*16+11][h][w],
									W1x1[co][12],  IFM[ci*16+12][h][w],
									W1x1[co][13],  IFM[ci*16+13][h][w],
									W1x1[co][14],  IFM[ci*16+14][h][w],
									W1x1[co][15],  IFM[ci*16+15][h][w]);
						OFM[co][h][w] = res1 > rmax ? RDT(rmax) : res1 < rmin ? RDT(rmin) : RDT(res1);
				}
			}
		}
	}
}
}


void ACTIVATION(RDT IFM[32][43][83], ADT OFM[32][43][83], BDT BBUF[32], MDT MBUF[32])
{
#pragma HLS INLINE off
#pragma HLS ARRAY_PARTITION variable=BBUF dim=1 complete
#pragma HLS ARRAY_PARTITION variable=MBUF dim=1 complete

    for (int h = 1; h < 42; h++)
    {
        for (int w = 1; w < 82; w+=2)
        {
#pragma HLS PIPELINE
            for (int c = 0; c < 32; c++)
            {
                ap_int<20> qy = IFM[c][h][w] + BBUF[c];
                ap_int<20> qy1 = IFM[c][h][w+1] + BBUF[c];

                if (qy < 0)
                {
                    qy = 0;
                }
                else
                {
                	qy = (qy * MBUF[c]) >> nm;
                }
                if (qy1 < 0)
                {
                    qy1 = 0;
                }
                else
                {
                	qy1 = (qy1 * MBUF[c]) >> nm;
                }				
                OFM[c][h][w] = qy < amin ? ADT(amin) : qy > amax ? ADT(amax) : ADT(qy);
				if (w<81){
                    OFM[c][h][w+1] = qy1 < amin ? ADT(amin) : qy1 > amax ? ADT(amax) : ADT(qy1);
				}

            }
        }
    }
}

void CACT(RDT IFM[32][43][83])
{
	for (int h = 1; h < 42; h++)
	{
		for (int w = 1; w < 82; w+=2)
		{
	#pragma HLS PIPELINE
			for (int c = 0; c < 32; c++)
			{
				IFM[c][h][w]=0;
				IFM[c][h][w+1]=0;
			}
		}
	}
}

//modify MAX function, the II of POOL function from 4 to 2
ADT MAX(ADT a, ADT b, ADT c, ADT d)
{
	ADT t1 = a > b ? a : b;
	ADT t2 = c > d ? c : d;
	return t1 > t2 ? t1 : t2;
}



// modify Load_WBUF3x3, reduce LUT and FF
void Load_WBUF3x3(WDT32* weight, WDT WBUF[32][32], int Mx)
{
    for(int m=0; m<9; m++)
    {

#pragma HLS PIPELINE II=1
		WDT32 DATA;
		DATA = weight[Mx*9+m];
		for(int c=0; c<32; c++)
		{
			WBUF[c][m] = DATA.range(8*c+7,8*c);
		}
	}
}

void Load_WBUF1x1(WDT32* weight, WDT WBUF1x1[32][32], int Mx, int Nx, int ic)
{
    for(int n=0; n<32; n++)
    {
#pragma HLS PIPELINE II=1
        WDT32 DATA;
        DATA = weight[Mx*ic+Nx*32+n];
        for(int m=0; m<32; m++)
        {
            WBUF1x1[m][n] = DATA.range(8*m+7,8*m);
        }
    }
}

void Load_BBUF(BDT16* bias, BDT BBUF[32], int Mx)
{
    for(int i=0; i<2; i++)
    {
        BDT16 DATA;
        DATA = bias[Mx*2+i];
#pragma HLS PIPELINE II=1
        for(int c=0; c<16; c++){
            BBUF[i*16+c] = DATA.range(16*c+15, 16*c);
        }
    }
}

void Load_FM(ADT32* ifm, ADT IFM[32][43][83], int Hx, int Wx, int Cx, int ow, int oh)
{
    int tile = ow/80;
    int h_o, w_o;
    if(tile)
    {
        h_o = Hx*40 + Hx/tile;
        w_o = Wx*80 + Wx/tile;
    }
    else
    {
        h_o = 0;
        w_o = 0;
    }
        
    for (int h=0; h<42; h++)
    {
        for (int w=0; w<82; w++)
        {
#pragma HLS PIPELINE II=1
            int ifm_index = Cx*(oh*2+3)*(ow*2+3) + (h+h_o)*(ow*2+3) + (w+w_o);
            ADT32 DATA;
            DATA = ifm[ifm_index];
            for (int c=0; c<32; c++)
            {
                IFM[c][h][w] = DATA.range(8*c+7,8*c);
            }
        }
    }
}

void POOL(ADT32* fm, ADT IFM[32][43][83], int Hx, int Wx, int Cx, int ow, int oh)
{
#pragma HLS ARRAY_PARTITION variable=IFM dim=1 complete
    int tile = ow/40;
    int h_o = Hx*20 + Hx/tile;
    int w_o = Wx*40 + Wx/tile;
    for (int h=1; h<=20; h++)
    {
        for (int w=1; w<=40; w++)
        {
#pragma HLS PIPELINE II=2
            int fm_index = Cx*(oh*2+3)*(ow*2+3) + (h+h_o)*(ow*2+3) + (w+w_o);
            ADT32 DATA;
            for (int c=0; c<32; c++)
            {
                DATA.range(8*c+7,8*c) = MAX(IFM[c][2*h-1][2*w-1],IFM[c][2*h-1][2*w],IFM[c][2*h][2*w-1],IFM[c][2*h][2*w]);
            }
            fm[fm_index] = DATA;
        }
    }
}

void POOL1(ADT OFM[32][43][83], ADT IFM[32][43][83], int Hx, int Wx)
{
#pragma HLS ARRAY_PARTITION variable=IFM dim=1 complete
    for (int h=1; h<=20; h++)
    {
        for (int w=1; w<=40; w++)
        {
#pragma HLS PIPELINE II=2
            for (int c=0; c<32; c++)
            {
                 OFM[c][21*Hx+h][41*Wx+w] = MAX(IFM[c][2*h-1][2*w-1],IFM[c][2*h-1][2*w],IFM[c][2*h][2*w-1],IFM[c][2*h][2*w]);
            }
        }
    }
}


void Load_FM1(ADT32* ifm, ADT IFM[32][43][83], int Cx)
{
    for (int h=0; h<43; h++)
    {
        for (int w=0; w<83; w++)
        {
#pragma HLS PIPELINE II=1
            int ifm_index = Cx*43*83 + h*83 + w;
            ADT32 DATA;
            DATA = ifm[ifm_index];
            for (int c=0; c<32; c++)
            {
                IFM[c][h][w] = DATA.range(8*c+7,8*c);
            }
        }
    }
}

void Export_FM(ADT32* fm, ADT OFM[32][43][83], int Hx, int Wx, int Cx, int ow, int oh)
{
    int tile = ow/80;
    int h_o, w_o;
    if(tile)
    {
        h_o = Hx*40 + Hx/tile;
        w_o = Wx*80 + Wx/tile;
    }
    else
    {
        h_o = 0;
        w_o = 0;
    }
    for (int h=1; h<=40; h++)
    {
        for (int w=1; w<=80; w++)
        {
#pragma HLS PIPELINE II=1
            int fm_index = Cx*(oh*2+3)*(ow*2+3) + (h+h_o)*(ow*2+3) + (w+w_o);
            ADT32 DATA;
            for (int c=0; c<32; c++)
            {
                DATA.range(8*c+7,8*c) = OFM[c][h][w];
            }
            fm[fm_index] = DATA;
        }
    }
}

void Export_FM1(ADT32* fm, ADT OFM[32][43][83], int Cx)
{
    for (int h=0; h<43; h++)
    {
#pragma HLS PIPELINE II=1
        for (int c=0; c<32; c++)
        {
            OFM[c][h][41] = 0;
        }
    }

    for (int w=0; w<83; w++)
    {
#pragma HLS PIPELINE II=1
        for (int c=0; c<32; c++)
        {
            OFM[c][21][w] = 0;
        }
    }

    for (int h=1; h<42; h++)
    {
        for (int w=1; w<82; w++)
        {
#pragma HLS PIPELINE II=1
            int ofm_index = Cx*43*83 + h*83 + w;
            ADT32 DATA;
            for (int c=0; c<32; c++)
            {
                DATA.range(8*c+7,8*c) = OFM[c][h][w];
            }
            fm[ofm_index] = DATA;
        }
    }
}

void Export_BBOX(BDT16* bbox, BDT16 BBOX[4])
{
    for (int i=0; i<4; i++)
    {
#pragma HLS PIPELINE II=1
        bbox[i] = BBOX[i];
    }
}

void Compute_BBOX(RDT OFM[32][43][83], BDT MBUF[32], BDT16 BBOX[4])
{
    int H,W;
    DT conf[2];
    RDT max[2], h_max[2], w_max[2];
    RDT xs, ys, ws, hs, flag, x, y;

    for(int b=0; b<4; b++)
    {
        switch(b)
        {
            case 0: H=1; W=1; break;
            case 1: H=1; W=42; break;
            case 2: H=22; W=1; break;
            case 3: H=22; W=42; break;
        }
        max[0] = OFM[4][H][W];
        h_max[0] = H;
        w_max[0] = W;

        max[1] = OFM[9][H][W];
        h_max[1] = H;
        w_max[1] = W;

        for(int h=0; h<20; h++){
            for(int w=0; w<40; w++){
#pragma HLS PIPELINE II=1
                if(OFM[4][h+H][w+W]>max[0]){
                    max[0] = OFM[4][h+H][w+W];
                    h_max[0] = h+H;
                    w_max[0] = w+W;
                }
                if(OFM[9][h+H][w+W]>max[1]){
                    max[1] = OFM[9][h+H][w+W];
                    h_max[1] = h+H;
                    w_max[1] = w+W;
                }
            }
        }
        
        conf[0] = max[0]*MBUF[4];
        conf[1] = max[1]*MBUF[9];
        if(conf[1]>conf[0])
        {
            xs = OFM[5][h_max[1]][w_max[1]];
            ys = OFM[6][h_max[1]][w_max[1]];
            ws = OFM[7][h_max[1]][w_max[1]];
            hs = OFM[8][h_max[1]][w_max[1]];
            flag = 1;
            x = w_max[1]-W;
            y = h_max[1]-H;
        }
        else
        {
            xs = OFM[0][h_max[0]][w_max[0]];
            ys = OFM[1][h_max[0]][w_max[0]];
            ws = OFM[2][h_max[0]][w_max[0]];
            hs = OFM[3][h_max[0]][w_max[0]];
            flag = 0;
            x = w_max[0]-W;
            y = h_max[0]-H;
        }
        BBOX[b].range(15,0)  = clamp_BDT(xs, bmin, bmax);
        BBOX[b].range(31,16) = clamp_BDT(ys, bmin, bmax);
        BBOX[b].range(47,32) = clamp_BDT(ws, bmin, bmax);
        BBOX[b].range(63,48) = clamp_BDT(hs, bmin, bmax);
        BBOX[b].range(79,64) = clamp_BDT(flag, bmin, bmax);
        BBOX[b].range(95,80) = clamp_BDT(x, bmin, bmax);
        BBOX[b].range(112,96) = clamp_BDT(y, bmin, bmax);
        BBOX[b].range(255,113) = 0;
    }
}
// modify Load_IMG4 for S2C approach
void Load_IMG4(ADT16* img, ADT IFM[32][43][83], int Hx, int Wx)
{
    int h_o = Hx*40-1;
    int w_o = Wx*80-1;
    for (int h=0; h<42; h++)
    {
        for (int w=0; w<82; w++)
        {
#pragma HLS PIPELINE II=1
            ADT16 DATA = img[(h+h_o)*320 + (w+w_o)];
            for (int c=0; c<12; c++)
            {

            	if (h+h_o<0||w+w_o<0||h+h_o>159||w+w_o>319)
            		IFM[c][h][w] = 128;
            	else
            		IFM[c][h][w] = DATA.range(8*c+7,8*c);
            }
        }
    }
}

void Load_IMG16(ADT16* img, ADT IFM[32][43][83], int Hx, int Wx)
{
    int h_o = Hx*40-1;
    int w_o = Wx*80-1;
    for (int h=0; h<42; h++)
    {
        for (int w=0; w<21; w++)  //82/4+1
        {
#pragma HLS PIPELINE II=4
        	int ww=w*4;
        	//[160,320,16] -> [4,160,320,4]
            ADT16 DATA0 = img[0*320*160 + (h+h_o)*320 + (ww+w_o)];
            ADT16 DATA1 = img[1*320*160 + (h+h_o)*320 + (ww+w_o)];
            ADT16 DATA2 = img[2*320*160 + (h+h_o)*320 + (ww+w_o)];
            ADT16 DATA3 = img[3*320*160 + (h+h_o)*320 + (ww+w_o)];

            for (int wc=0; wc<4; wc++)
            {
            	for (int c=0; c<3; c++)
            	{
            		if(ww+wc<=81)
            		{
            			if (h+h_o<0||ww+w_o+wc<0||h+h_o>159||ww+w_o+wc>319)
            			{
            				IFM[c][h][ww+wc] = 128;
            				IFM[c+3][h][ww+wc] = 128;
            				IFM[c+6][h][ww+wc] = 128;
            				IFM[c+9][h][ww+wc] = 128;
            			}
            			else
            			{
            				IFM[c][h][ww+wc] = DATA0.range(8*(wc*3+c)+7,8*(wc*3+c));
            				IFM[c+3][h][ww+wc] = DATA1.range(8*(wc*3+c)+7,8*(wc*3+c));
            				IFM[c+6][h][ww+wc] = DATA2.range(8*(wc*3+c)+7,8*(wc*3+c));
            				IFM[c+9][h][ww+wc] = DATA3.range(8*(wc*3+c)+7,8*(wc*3+c));
            			}
            		}

//
//            	if(ww+2<=81){
//            		if (h+h_o<0||ww+w_o+2<0||h+h_o>159||ww+w_o+2>319)
//            			IFM[c][h][ww+2] = 128;
//            		else
//            			IFM[c][h][ww+2] = DATA2.range(8*c+7,8*c);
//            	}
//
//            	if(ww+3<=81){
//            		if (h+h_o<0||ww+w_o+3<0||h+h_o>159||ww+w_o+3>319)
//            			IFM[c][h][ww+3] = 128;
//            		else
//            			IFM[c][h][ww+3] = DATA3.range(8*c+7,8*c);
//            	}
            	}
            }
        }
    }
}

void beifen_Load_IMG16(ADT16* img, ADT IFM[32][43][83], int Hx, int Wx)
{
    int h_o = Hx*40-1;
    int w_o = Wx*80-1;
    for (int h=0; h<42; h++)
    {
        for (int w=0; w<21; w++)  //82/4+1
        {
#pragma HLS PIPELINE II=4
        	int ww=w*4;
        	//[160,320,16] -> [4,160,320,4]
            ADT16 DATA0 = img[(h+h_o)*320 + (ww+w_o)];
            ADT16 DATA1 = img[(h+h_o)*320 + (ww+w_o)+1];
            ADT16 DATA2 = img[(h+h_o)*320 + (ww+w_o)+2];
            ADT16 DATA3 = img[(h+h_o)*320 + (ww+w_o)+3];
            for (int c=0; c<12; c++)
            {

            	if (h+h_o<0||ww+w_o<0||h+h_o>159||ww+w_o>319)
            		IFM[c][h][ww] = 128;
            	else
            		IFM[c][h][ww] = DATA0.range(8*c+7,8*c);

            	if (h+h_o<0||ww+w_o+1<0||h+h_o>159||ww+w_o+1>319)
            		IFM[c][h][ww+1] = 128;
            	else
            		IFM[c][h][ww+1] = DATA1.range(8*c+7,8*c);

            	if(ww+2<=81){
            		if (h+h_o<0||ww+w_o+2<0||h+h_o>159||ww+w_o+2>319)
            			IFM[c][h][ww+2] = 128;
            		else
            			IFM[c][h][ww+2] = DATA2.range(8*c+7,8*c);
            	}

            	if(ww+3<=81){
            		if (h+h_o<0||ww+w_o+3<0||h+h_o>159||ww+w_o+3>319)
            			IFM[c][h][ww+3] = 128;
            		else
            			IFM[c][h][ww+3] = DATA3.range(8*c+7,8*c);
            	}
            }
        }
    }
}

void DFM(ADT src[32][43][83], ADT dest[32][43][83], int b)
{
	for(int w=0; w<43; w++){
		for (int h=0; h<82; h+=2){
#pragma HLS PIPELINE
			for (int c=0; c<3;c++){
				dest[c][w][h]=src[3*b+c][w][h];
				dest[c][w][h+1]=src[3*b+c][w][h+1];

			}
		}
	}
}

void CFM(ADT buf[32][43][83]){
	for(int w=1; w<43; w++){
		for (int h=0; h<82; h+=2){
#pragma HLS PIPELINE
			for (int c=3; c<32;c++){
				buf[c][w][h]=0;
				buf[c][w][h+1]=0;
			}
		}
	}
}

//void Padding(ADT buf[32][43][83])
//{
//    for (int h=0; h<43; h++)
//    {
//#pragma HLS PIPELINE II=1
//        for (int c=0; c<32; c++)
//        {
//        	buf[c][h][41] = 0;
//        }
//    }
//
//    for (int w=0; w<83; w++)
//    {
//#pragma HLS PIPELINE II=1
//        for (int c=0; c<32; c++)
//        {
//        	buf[c][21][w] = 0;
//        }
//    }
//}

//add layer1_done for pre-load workflow
void SkyNet(ADT4* img, ADT32* fm, WDT32* weight, BDT16* biasm, int layer1_done)
{

#pragma HLS INTERFACE s_axilite register port=layer1_done //bundle=layer_done

#pragma HLS INTERFACE m_axi depth=204800 port=img    offset=slave bundle=img
#pragma HLS INTERFACE m_axi depth=628115 port=fm     offset=slave bundle=fm
#pragma HLS INTERFACE m_axi depth=13792  port=weight offset=slave bundle=wt
#pragma HLS INTERFACE m_axi depth=432    port=biasm  offset=slave bundle=wt
#pragma HLS INTERFACE s_axilite register port=return

#pragma HLS ALLOCATION instances=SHARECONV	   	limit=1 function
#pragma HLS ALLOCATION instances=REORG	    	limit=1 function
#pragma HLS ALLOCATION instances=POOL	    	limit=1 function
#pragma HLS ALLOCATION instances=ACTIVATION    	limit=1 function
#pragma HLS ALLOCATION instances=Load_FM    	limit=1 function
#pragma HLS ALLOCATION instances=Export_FM    limit=1 function
#pragma HLS ALLOCATION instances=Load_FM1    	limit=1 function
#pragma HLS ALLOCATION instances=Export_FM1   limit=1 function
#pragma HLS ALLOCATION instances=CFM   limit=1 function

    /*********************************DWCONV1+PWCONV1********************************/
    std::cout << "DWCONV1+PWCONV1" << std::endl;
    Load_WBUF3x3(weight + conv1_w, WBUF[0], 0);
    Load_BBUF(biasm + conv1_b, BBUF[0], 0);
    Load_BBUF(biasm + conv1_m, MBUF[0], 0);

    Load_WBUF1x1(weight + conv2_w, WBUF[1], 0, 0, config[2].ic);
    Load_WBUF1x1(weight + conv2_w, WBUF[2], 1, 0, config[2].ic);
    Load_BBUF(biasm + conv2_b, BBUF[1], 0);
    Load_BBUF(biasm + conv2_b, BBUF[2], 1);
    Load_BBUF(biasm + conv2_m, MBUF[1], 0);
    Load_BBUF(biasm + conv2_m, MBUF[2], 1);
    
    {
        for(int b=0; b<4; b++)
        {
            int H, W;
            switch(b)
            {
                case 0: H=0; W=0; break;
                case 1: H=0; W=4; break;
                case 2: H=4; W=0; break;
                case 3: H=4; W=4; break;
            }
            for(int Hx=0; Hx<4; Hx++)

            {
                Load_IMG(img, FM1, Hx, 0, b);
                for(int Wx=0; Wx<4; Wx++)
                {
                    if(Wx%2==0)
                    {
                        Load_IMG(img, FM2, Hx, Wx+1, b);
                        SHARECONV(FM1, FM4, WBUF[0], 0,0);
                        ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
                        for(int Mx=0; Mx<2; Mx++)
                        {
                        	SHARECONV(FM1, FM4,  WBUF[Mx+1],1,0);
                            ACTIVATION(FM4, FM3, BBUF[1+Mx], MBUF[1+Mx]);
                            POOL(fm + pool1_o, FM3, Hx+H, Wx+W, Mx, config[3].ow, config[3].oh);
                        }
                    }
                    else
                    {
                        Load_IMG(img, FM1, Hx, Wx+1, b);
                        SHARECONV(FM2, FM4, WBUF[0],0,0);
                        ACTIVATION(FM4, FM2, BBUF[0], MBUF[0]);
                        for(int Mx=0; Mx<2; Mx++)
                        {
                        	SHARECONV(FM2, FM4, WBUF[Mx+1],1,0);
                            ACTIVATION(FM4, FM3, BBUF[1+Mx], MBUF[1+Mx]);
                            POOL(fm + pool1_o, FM3, Hx+H, Wx+W, Mx, config[3].ow, config[3].oh);
                        }
                    }
                }
            }
        }
    }
  
    /*********************************DWCONV2+PWCONV2********************************/
    std::cout << "DWCONV2+PWCONV2" << std::endl;
    Load_WBUF3x3(weight + conv3_w, WBUF[0], 0);
    Load_WBUF3x3(weight + conv3_w, WBUF[1], 1);
    Load_BBUF(biasm + conv3_b, BBUF[0], 0);
    Load_BBUF(biasm + conv3_b, BBUF[1], 1);
    Load_BBUF(biasm + conv3_m, MBUF[0], 0);
    Load_BBUF(biasm + conv3_m, MBUF[1], 1);
    {
        for(int Hx=0; Hx<4; Hx++)
        {
            for(int Wx=0; Wx<4; Wx++)
            {
                Load_FM(fm + pool1_o, FM1, Hx, Wx, 0, config[4].ow, config[4].oh);
                SHARECONV(FM1, FM4, WBUF[0], 0, 0);
                ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
                CACT(FM4);

                Load_FM(fm + pool1_o, FM2, Hx, Wx, 1, config[4].ow, config[4].oh);
                SHARECONV(FM2, FM4, WBUF[1], 0, 0);
                ACTIVATION(FM4, FM2, BBUF[1], MBUF[1]);
                CACT(FM4);

                for(int Mx=0; Mx<3; Mx++)
                {
                    Load_WBUF1x1(weight + conv4_w, WBUF[2], Mx, 0, config[5].ic);
                    SHARECONV(FM1, FM4, WBUF[2],1, 1);
                    Load_WBUF1x1(weight + conv4_w, WBUF[2], Mx, 1, config[5].ic);
                    SHARECONV(FM2, FM4, WBUF[2],1, 0);

                    Load_BBUF(biasm + conv4_b, BBUF[2], Mx);
                    Load_BBUF(biasm + conv4_m, MBUF[2], Mx);
                    ACTIVATION(FM4, FM3, BBUF[2], MBUF[2]);
                    CACT(FM4);
                    POOL(fm + pool2_o, FM3, Hx, Wx, Mx, config[6].ow, config[6].oh);
                }
            }
        }
    }

    /*********************************DWCONV3********************************/
    std::cout << "DWCONV3" << std::endl;
    Load_WBUF3x3(weight + conv5_w, WBUF[0], 0);
    Load_WBUF3x3(weight + conv5_w, WBUF[1], 1);
    Load_WBUF3x3(weight + conv5_w, WBUF[2], 2);
    Load_BBUF(biasm + conv5_b, BBUF[0], 0);
    Load_BBUF(biasm + conv5_b, BBUF[1], 1);
    Load_BBUF(biasm + conv5_b, BBUF[2], 2);
    Load_BBUF(biasm + conv5_m, MBUF[0], 0);
    Load_BBUF(biasm + conv5_m, MBUF[1], 1);
    Load_BBUF(biasm + conv5_m, MBUF[2], 2);
    {
        for(int Hx=0; Hx<2; Hx++)
        {
            for(int Wx=0; Wx<2; Wx++)
            {
                Load_FM(fm + pool2_o, FM1, Hx, Wx, 0, config[7].ow, config[7].oh);
                SHARECONV(FM1, FM4, WBUF[0],0, 0);
                ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
                CACT(FM4);
                Export_FM(fm + conv5_o, FM1, Hx, Wx, 0, config[7].ow, config[7].oh);

                Load_FM(fm + pool2_o, FM1, Hx, Wx, 1, config[7].ow, config[7].oh);
                SHARECONV(FM1, FM4, WBUF[1],0, 0);
                ACTIVATION(FM4, FM1, BBUF[1], MBUF[1]);
                CACT(FM4);
                Export_FM(fm + conv5_o, FM1, Hx, Wx, 1, config[7].ow, config[7].oh);

                Load_FM(fm + pool2_o, FM1, Hx, Wx, 2, config[7].ow, config[7].oh);
                SHARECONV(FM1, FM4, WBUF[2],0, 0);
                ACTIVATION(FM4, FM1, BBUF[2], MBUF[2]);
                CACT(FM4);
                Export_FM(fm + conv5_o, FM1, Hx, Wx, 2, config[7].ow, config[7].oh);
            }
        }
    }

    /*********************************PWCONV3+DWCONV4********************************/
    std::cout << "PWCONV3+DWCONV4" << std::endl;
    {

    	for(int Mx=0; Mx<6; Mx++)
    	{
			for(int Hx=0; Hx<2; Hx++)
			{
				for(int Wx=0; Wx<2; Wx++)
				{
					for(int Nx=0; Nx<3; Nx++)
					{
						Load_FM(fm + conv5_o, FM1, Hx, Wx, Nx, config[7].ow, config[7].oh);
						Load_WBUF1x1(weight + conv6_w, WBUF[0], Mx, Nx, config[8].ic);
						SHARECONV(FM1, FM4, WBUF[0], 1, 1); //using ping pang not good
					}
					Load_BBUF(biasm + conv6_b, BBUF[0], Mx);
					Load_BBUF(biasm + conv6_m, MBUF[0], Mx);
					ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
					CACT(FM4);
					Export_FM(fm + conv6_o, FM1, Hx, Wx, Mx, config[8].ow, config[8].oh);
					//POOL(fm + pool3_o, FM1, Hx, Wx, Mx, config[10].ow, config[10].oh);
					POOL1(FM2,FM1,Hx,Wx);
				}
			}
			Load_WBUF3x3(weight + conv7_w, WBUF[0], Mx);
			Load_BBUF(biasm + conv7_b, BBUF[0], Mx);
			Load_BBUF(biasm + conv7_m, MBUF[0], Mx);

			//Padding(FM2);
			SHARECONV(FM2, FM4, WBUF[0],0, 0);

			ACTIVATION(FM4, FM3, BBUF[0], MBUF[0]);
			CACT(FM4);
			Export_FM1(fm + conv7_o, FM3, Mx);
		}
    }

    /*********************************PWCONV4+DWCCONV5********************************/
    std::cout << "PWCONV4+DWCCONV5" << std::endl;

//    {
//        for(int Nx=0; Nx<6; Nx++)
//        {
//            Load_WBUF3x3(weight + conv7_w, WBUF[0], Nx);
//            Load_BBUF(biasm + conv7_b, BBUF[0], Nx);
//            Load_BBUF(biasm + conv7_m, MBUF[0], Nx);
//
//            Load_FM1(fm + pool3_o, FM1, Nx);
//            SHARECONV(FM1, FM4, WBUF[0],0, 0);
//
//            ACTIVATION(FM4, FM3, BBUF[0], MBUF[0]);
//            CACT(FM4);
//            Export_FM1(fm + conv7_o, FM3, Nx);
//        }
//    }

    {
        Load_FM1(fm + conv7_o, FM3, 0);
        for(int Mx=0; Mx<12; Mx++)
        {
        	Load_WBUF1x1(weight + conv8_w, WBUF[1], Mx, 0, config[12].ic);
        	SHARECONV(FM3, FM4, WBUF[1],1, 1);

            Load_BBUF(biasm + conv8_b, BBUF[0], Mx);
            Load_BBUF(biasm + conv8_m, MBUF[0], Mx);

            Load_FM1(fm + conv7_o, FM2, 1);
            for(int Nx=1; Nx<6; Nx++)
            {
                Load_WBUF1x1(weight + conv8_w, WBUF[1], Mx, Nx, config[12].ic);
                if(Nx%2==0)
                {
                    Load_FM1(fm + conv7_o, FM2, Nx+1);
                    SHARECONV(FM1, FM4, WBUF[1],1, 1); //PWCONV4
                }
                else
                {
                    Load_FM1(fm + conv7_o, FM1, Nx+1);
                    SHARECONV(FM2, FM4, WBUF[1],1, 1);
                }
            }
            ACTIVATION(FM4, FM2, BBUF[0], MBUF[0]);//PWCONV4
            CACT(FM4);
            Load_WBUF3x3(weight + conv9_w, WBUF[2], Mx);
            Load_BBUF(biasm + conv9_b, BBUF[1], Mx);
            Load_BBUF(biasm + conv9_m, MBUF[1], Mx);
            //Padding(FM2);
            //Export_FM1(fm + conv8_o, FM3, Mx);
            SHARECONV(FM2, FM4, WBUF[2],0, 0);//DWCONV5
            ACTIVATION(FM4, FM1, BBUF[1], MBUF[1]);
            CACT(FM4);
            //Padding(FM3);
            Export_FM1(fm + conv9_o, FM1, Mx);
        }
    }

    /*********************************PWCONV5+DWCONV6********************************/
    std::cout << "PWCONV5+DWCONV6" << std::endl;
    {
    		Load_FM1(fm + conv9_o, FM3, 0);
            for(int Mx=0; Mx<16; Mx++)
            {
            	Load_WBUF1x1(weight + conv10_w, WBUF[0], Mx, 0, config[14].ic);
            	SHARECONV(FM3, FM4, WBUF[0],1, 1);
                Load_BBUF(biasm + conv10_b, BBUF[0], Mx);
                Load_BBUF(biasm + conv10_m, MBUF[0], Mx);

                Load_FM1(fm + conv9_o, FM2, 1);
                for(int Nx=1; Nx<12; Nx++)
                {
                    Load_WBUF1x1(weight + conv10_w, WBUF[0], Mx, Nx, config[14].ic);
                    if(Nx%2==0)
                    {
                        Load_FM1(fm + conv9_o, FM2, Nx+1);
                        SHARECONV(FM1, FM4, WBUF[0],1, 1);
                    }
                    else
                    {
                        Load_FM1(fm + conv9_o, FM1, Nx+1);
                        SHARECONV(FM2, FM4, WBUF[0],1, 1);  //pw5
                    }
                }
                ACTIVATION(FM4, FM2, BBUF[0], MBUF[0]);
                CACT(FM4);
               // Export_FM1(fm + conv10_o, FM2, Mx);
                Load_WBUF3x3(weight + conv11_w, WBUF[1], Mx+24);
                Load_BBUF(biasm + conv11_b, BBUF[1], Mx+24);
                Load_BBUF(biasm + conv11_m, MBUF[1], Mx+24);
                //Padding(FM2);
                SHARECONV(FM2, FM4, WBUF[1],0, 0);  //dw6
                ACTIVATION(FM4, FM1, BBUF[1], MBUF[1]);
                CACT(FM4);
                Export_FM1(fm + conv11_o, FM1, Mx+24);
            }
        }

    /*********************************REORG+CONCAT+DWCONV6********************************/
    std::cout << "REORG+DWCONV6" << std::endl;

    {
        for(int Nx=0; Nx<6; Nx++)
        {
            for(int Rx=0; Rx<4; Rx++)
            {
                REORG(fm + conv6_o, FM1, Nx, Rx);
                Load_WBUF3x3(weight + conv11_w, WBUF[0], Nx+6*Rx);
                Load_BBUF(biasm + conv11_b, BBUF[0], Nx+6*Rx);
                Load_BBUF(biasm + conv11_m, MBUF[0], Nx+6*Rx);
                SHARECONV(FM1, FM4, WBUF[0],0, 0);
                ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
                CACT(FM4);
                Export_FM1(fm + conv11_o, FM1, Nx+6*Rx);
            }
        }
    }

    layer1_done=0;
    /*********************************PWCONV6********************************/
    std::cout << "PWCONV6" << std::endl;
    {
    	Load_FM1(fm + conv11_o, FM3, 0);
        for(int Mx=0; Mx<3; Mx++)
        {
        	Load_WBUF1x1(weight + conv12_w, WBUF[0], Mx, 0, config[17].ic);
        	SHARECONV(FM3, FM4, WBUF[0],1, 1);
            Load_FM1(fm + conv11_o, FM2, 1);

            for(int Nx=1; Nx<40; Nx++)
            {
                Load_WBUF1x1(weight + conv12_w, WBUF[0], Mx, Nx, config[17].ic);
                if(Nx%2==0)
                {
                    Load_FM1(fm + conv11_o, FM2, Nx+1);
                    SHARECONV(FM1, FM4, WBUF[0],1, 1);
                }
                else
                {
                    Load_FM1(fm + conv11_o, FM1, Nx+1);
                    SHARECONV(FM2, FM4, WBUF[0],1, 1);
                }
            }
            Load_BBUF(biasm + conv12_b, BBUF[0], Mx);
            Load_BBUF(biasm + conv12_m, MBUF[0], Mx);
            ACTIVATION(FM4, FM1, BBUF[0], MBUF[0]);
            CACT(FM4);
            Export_FM1(fm + conv12_o, FM1, Mx);
        }
    }

    /*********************************CONV13********************************/
    std::cout << "CONV13" << std::endl;
    for(int Nx=0; Nx<3; Nx++)
    {
        Load_FM1(fm + conv12_o, FM1, Nx);
        Load_WBUF1x1(weight + conv13_w, WBUF[0], 0, Nx, config[18].ic);
        SHARECONV(FM1, FM4, WBUF[0],1, 1);
    }
    Load_BBUF(biasm + conv13_m, MBUF[0], 0);
    BDT16 BBOX[4];
    Compute_BBOX(FM4, MBUF[0], BBOX);
    Export_BBOX(biasm + bbox_o, BBOX);
}
