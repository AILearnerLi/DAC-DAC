#include "seuer.h"
#include <stdio.h>
#include <math.h>
#include <ap_fixed.h>
#include "hls_stream.h"

//mode = 0 3x3depthwise; mode = 1 1x1 conv2d
#define X1 1
#define X3 0


FIX_32_10 compute_engine_16(FIX_WT w0,  FIX_FM b0,
					  FIX_WT w1,  FIX_FM b1,
					  FIX_WT w2,  FIX_FM b2,
					  FIX_WT w3,  FIX_FM b3,
					  FIX_WT w4,  FIX_FM b4,
					  FIX_WT w5,  FIX_FM b5,
					  FIX_WT w6,  FIX_FM b6,
					  FIX_WT w7,  FIX_FM b7,
					  FIX_WT w8,  FIX_FM b8,
					  FIX_WT w9,  FIX_FM b9,
					  FIX_WT w10, FIX_FM b10,
					  FIX_WT w11, FIX_FM b11,
					  FIX_WT w12, FIX_FM b12,
					  FIX_WT w13, FIX_FM b13,
					  FIX_WT w14, FIX_FM b14,
					  FIX_WT w15, FIX_FM b15
					  )
{
	FIX_32_10 mul0, mul1, mul2,  mul3,  mul4,  mul5,  mul6,  mul7;
	FIX_32_10 mul8, mul9, mul10, mul11, mul12, mul13, mul14, mul15;
	FIX_32_10 add0, add1, add2, add3,  add4,  add5,  add6;
	FIX_32_10 add7, add8, add9, add10, add11, add12, add13, add14;

	mul0  = w0  * b0;
	mul1  = w1  * b1;
	mul2  = w2  * b2;
	mul3  = w3  * b3;
	mul4  = w4  * b4;
	mul5  = w5  * b5;
	mul6  = w6  * b6;
	mul7  = w7  * b7;
	mul8  = w8  * b8;
	mul9  = w9  * b9;

	mul10 = w10 * b10;
	mul11 = w11 * b11;
	mul12 = w12 * b12;
	mul13 = w13 * b13;
	mul14 = w14 * b14;
	mul15 = w15 * b15;

	add0 = mul0  + mul1;
	add1 = mul2  + mul3;
	add2 = mul4  + mul5;
	add3 = mul6  + mul7;

	add4 = mul8  + mul9;

	add5 = mul10 + mul11;
	add6 = mul12 + mul13;
	add7 = mul14 + mul15;

	add8  = add0 + add1;
	add9  = add2 + add3;
	add10 = add4 + add5;
	add11 = add6 + add7;

	add12 = add8  + add9;
	add13 = add10 + add11;

	add14 = add12 + add13;

	return add14;

}

void load_weights(FIX_WT weights[32][32], FIX_WT weight_buf[32][16], int CI)
{
	for(int ci = 0; ci < 16; ci++) {
		for(int co = 0; co < 32; co++) {
#pragma HLS pipeline
			weight_buf[co][ci] = weights[co][ci + CI];
		}
	}
}


void Conv2D(FIX_FM bottom[CHA][HEI][WID],
			  FIX_FM_acc top[CHA][HEI][WID],
			  FIX_WT weights[CHA][CHA],
			  FIX_FM ex_bottom[CHA][HEI][WID],
			  uint1 mode, uint1 relu, uint2 CI) //mode = 0 3x3depthwise; mode = 1 1x1 conv2d
{

#pragma HLS array_partition variable=top dim=1 complete
#pragma HLS array_partition variable=bottom dim=1 complete

#pragma HLS array_partition variable=ex_bottom dim=1 complete

#pragma HLS array_partition variable=weights dim=1 complete

FIX_WT weight_buf[32][16];
#pragma HLS array_partition variable=weight_buf dim=1 complete
#pragma HLS array_partition variable=weight_buf dim=2 complete

#pragma HLS ALLOCATION instances=compute_engine_16 limit=16 function
FIX_FM zero = 0;
FIX_FM_acc zero_acc = 0;

FIX_FM window[CHA][3][3];
#pragma HLS array_partition variable=window dim=0 complete

FIX_FM line_buffer[CHA][2][WID];
#pragma HLS array_partition variable=line_buffer dim=1 complete

#pragma HLS DEPENDENCE variable=line_buffer inter false

	if(mode==X3){
		load_weights(weights, weight_buf, 0);
		for(int h = 0; h < HEI; h++){
			for(int w = 0; w < WID; w++) {
#pragma HLS pipeline II=2
				for(int coo = 0; coo < CHA; coo++) {
#pragma HLS unroll
					for (int i=0;i<3;i++){
					window[coo][i][0] = window[coo][i][1];
					window[coo][i][1] = window[coo][i][2];
					}

					window[coo][0][2] = (line_buffer[coo][0][w]);
					window[coo][1][2] = (line_buffer[coo][0][w] = line_buffer[coo][1][w]);
					window[coo][2][2] = (line_buffer[coo][1][w] = ex_bottom[coo][h][w]);

					if ((2<=h) && (2<=w)){

						bottom[coo][h-1][w-1] = bottom[coo][h-1][w-1] + (FIX_FM)compute_engine_16(
								weight_buf[coo][0],   window[coo][0][0],
								weight_buf[coo][1],   window[coo][0][1],
								weight_buf[coo][2],   window[coo][0][2],
								weight_buf[coo][3],   window[coo][1][0],
								weight_buf[coo][4],   window[coo][1][1],
								weight_buf[coo][5],   window[coo][1][2],
								weight_buf[coo][6],   window[coo][2][0],
								weight_buf[coo][7],   window[coo][2][1],
								weight_buf[coo][8],   window[coo][2][2],
								weight_buf[coo][9],   zero,
								weight_buf[coo][10],  zero,
								weight_buf[coo][11],  zero,
								weight_buf[coo][12],  zero,
								weight_buf[coo][13],  zero,
								weight_buf[coo][14],  zero,
								weight_buf[coo][15],  zero);
						bottom[coo][h-1][w-1] = (relu==1 && bottom[coo][h-1][w-1]<0)?zero:bottom[coo][h-1][w-1];
					}
				}
			}
		}
	}
	else{
		for(int ci = 0; ci < CI; ci ++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=2

			load_weights(weights, weight_buf, ci*16);
			for(int h = 1; h <= (HEI-2); h++){
				for(int w = 1; w <= (WID-2); w++) {
#pragma HLS pipeline II=2
					for(int coo = 0; coo < CHA; coo++) {
#pragma HLS unroll
						top[coo][h][w] += compute_engine_16(
												weight_buf[coo][0],   bottom[ci*16+0][h][w],
												weight_buf[coo][1],   bottom[ci*16+1][h][w],
												weight_buf[coo][2],   bottom[ci*16+2][h][w],
												weight_buf[coo][3],   bottom[ci*16+3][h][w],
												weight_buf[coo][4],   bottom[ci*16+4][h][w],
												weight_buf[coo][5],   bottom[ci*16+5][h][w],
												weight_buf[coo][6],   bottom[ci*16+6][h][w],
												weight_buf[coo][7],   bottom[ci*16+7][h][w],
												weight_buf[coo][8],   bottom[ci*16+8][h][w],
												weight_buf[coo][9],   bottom[ci*16+9][h][w],
												weight_buf[coo][10],  bottom[ci*16+10][h][w],
												weight_buf[coo][11],  bottom[ci*16+11][h][w],
												weight_buf[coo][12],  bottom[ci*16+12][h][w],
												weight_buf[coo][13],  bottom[ci*16+13][h][w],
												weight_buf[coo][14],  bottom[ci*16+14][h][w],
												weight_buf[coo][15],  bottom[ci*16+15][h][w]);
					}
				}
			}
		}
	}
}
