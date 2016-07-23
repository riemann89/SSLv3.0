#include "Utilities.h"

void reverse(uint8_t originale[],int length){ 					 		//take as input an array of Bytes and its length,  after this function is called your original vector will be reversed
	uint8_t reversed[length];			                                                //array into which put a copy of the original one
	int i;
	for(i=0; i<length;  i++){
		reversed[i]=originale[i];														//copy original into reversed
	}
 for(i=0; i<length;   i++){ 
		originale[i]=reversed[length-i-1];										//substitute original starting from top to bottom	
	}
}

void  int_To_Bytes(uint32_t t, uint8_t *t_Bytes){ 						 			//take as input an integer and an array of bytes  after this function is called, the array will be the reversed Bytes representation of the integer
    int i;
	uint8_t *p=(uint8_t*) &t;                                                               //*p is a pointer uint8 representation  of the  input int t, but still reversed
	for(i=0;i<4;i++){
		t_Bytes[i]= *(p+i);                                                                        // copy *p into the array t_Bytes
	}
	reverse(t_Bytes,4);                                                                         // reverse t_Bytes, now it is the 4 bytes rappresentation of int t with leftmost significant bit.
}

uint32_t Bytes_To_Int(int len, uint8_t  *t_Bytes){								// convert a stream of max 4 byte into an unsigned int with leftmost significant bit 
	if(len > 4)
	{
		printf("error max length conversion = 4");
		return 1;
	}
	
	int res = 0;
	for (int i =0; i < len; i++)
	{
		res =  res*256 + t_Bytes[i];
	}
	return res;
}