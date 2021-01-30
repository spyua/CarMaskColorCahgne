#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define pattern_size 256
//#define color  0xFFFFFF     //Recovery
#define color  0xA9C8FC   //Blue

int file_exist(char *file_path){
	
    struct stat st;

	if(NULL == file_path) return -1;

	memset(&st,0,sizeof(struct stat));

	if(stat(file_path,&st) == -1)  // error
	{
		printf("File [%s] *NOT* Exist!\n",file_path);
		return -1;
	}
	return st.st_size;
}
char *safe_malloc(int size){

	if(size <=0) return NULL;

	char *mptr= (char*)malloc(size);
	
	if(mptr == NULL) {
		fprintf(stderr,"____>>%s: malloc fail!\n",__FUNCTION__);
		return NULL;
	}
	
	memset(mptr,0,size);
	return mptr;

}

int main(int argc, char *argv[])
{	
 
    char *car_bmp = "car_image.bmp";
	char *car_mask = "car_image_mask.bmp";
	char *car_ori_bmp = "car_image_ori.bmp";

	char *bmp_ptr=NULL;
	char *bmp_ori_ptr=NULL;
	char *bmp_copy=NULL;
	char *color_mask=NULL;
	
	int file_size;
	int file_size_bmp;

    // Get Target Bmp Info
	file_size = file_exist(car_bmp);
	file_size_bmp=file_size;
	int fd = open(car_bmp, O_RDONLY);
	bmp_ptr=safe_malloc(file_size);
	int ret_size=read(fd,bmp_ptr,file_size);
	close(fd);

	unsigned char bmp_hd[54];
	int img_width,img_height,img_start,flip_order;
	memcpy(bmp_hd,bmp_ptr,54);
	img_start  = bmp_hd[10] | bmp_hd[11] << 8 | bmp_hd[12] << 16 | bmp_hd[13] << 24;
	img_width  = bmp_hd[18] | bmp_hd[19] << 8 | bmp_hd[20] << 16 | bmp_hd[21] << 24;
	img_height = bmp_hd[22] | bmp_hd[23] << 8 | bmp_hd[24] << 16 | bmp_hd[25] << 24;
	printf("Image W:%d H%d\n",img_width,img_height);
	
	if(img_height < 0) {
        flip_order = 1;
		img_height = -img_height;
	}else 
		flip_order = 0;
	
	if(img_width % 2) 
		img_width++;

	printf("Adjust After Image W:%d H%d\n",img_width,img_height);

	// Get Ori Bmp
	file_size=file_exist(car_ori_bmp);
	if(file_size == -1)
		return 0;
	
	fd = open(car_ori_bmp, O_RDONLY);
	bmp_ori_ptr=safe_malloc(file_size);
	ret_size=read(fd,bmp_ori_ptr,file_size);
	close(fd);

	int bmp_factor=0;

	if(bmp_hd[28] ==16)
		bmp_factor=2;
	else if(bmp_hd[28] ==24) 
		bmp_factor=3;

	printf("hd[28]=%d, Factor=%d\n", bmp_hd[28], bmp_factor);

	int bmp_size=img_width * img_height *  bmp_factor + img_start;

	printf(" bmp_size= img_width * img_height *  bmp_factor + img_start =%d\n", bmp_size);

	bmp_copy=(char*)malloc(bmp_size);

	if(bmp_copy != NULL){
		memcpy(bmp_copy,bmp_ptr,bmp_size);
		fprintf(stderr,"===memcpy(bmp_copy,bmp_addr,bmp_size)===\n");
	}else{
		return 0;
	}	

	//Load Color Mask
	file_size=file_exist(car_mask);
	fd=open(car_mask,O_RDONLY);
    color_mask=safe_malloc(file_size);
	ret_size=read(fd,color_mask,file_size);

    // Create Palette
	int curr_palette[pattern_size];
	memset(curr_palette,-1,sizeof(curr_palette));		
	
	int bcRGB=0;
	int fcRGB=color;
	
	int fcR= (fcRGB & 0Xff0000)>>16;
	int fcG= (fcRGB & 0x00ff00)>>8;
	int fcB= (fcRGB & 0x0000ff);
    printf("fcR=%d, fcG=%d, fcB=%d\n",fcR, fcG, fcB);

    int bcR= (bcRGB & 0xff0000) >>16;
    int bcG= (bcRGB & 0x00ff00) >>8;
	int bcB= (bcRGB & 0x0000ff);
    printf("bcR=%d, bcG=%d, bcB=%d\n",bcR, bcG, fcB);

	float diffR = ((float)(bcR - fcR))/pattern_size;
	float diffG = ((float)(bcG - fcG))/pattern_size;
	float diffB = ((float)(bcB - fcB))/pattern_size;
    printf("diffR=%f, diffG=%f, diffB=%f\n",diffR, diffG, diffB);
	
    int i,j;

	for(i=0;i<pattern_size;i++){
		curr_palette[pattern_size-1-i]= ((fcR +(int)(diffR * i)) <<16);
		curr_palette[pattern_size-1-i] |= ((fcG +(int)(diffG * i)) <<8);
		curr_palette[pattern_size-1-i] |= (fcB + (int)(diffB * i)) ;
		curr_palette[pattern_size-1-i] =  curr_palette[pattern_size-1-i] ;
	}

    
    // Load Palette
	for(i=0;i<img_height;i++) {
	    for(j=0;j<img_width;j++) 
		{
		   int cur_off=i*img_width+j;
		   int cur_mask_value=(color_mask[1078+cur_off]);
           
           if(cur_mask_value<0){
               cur_mask_value = cur_mask_value*-1;
           }
	       
		   if(cur_mask_value==0)  continue;
		   
		   int curr_palette_value=curr_palette[cur_mask_value];
		   cur_off=i*img_width+j;
		   
		   int bmp_copy_offset=54+(bmp_factor*cur_off);

		  	bmp_copy[bmp_copy_offset]  =((curr_palette_value  & 0x0000FF));     //B
		   	bmp_copy[bmp_copy_offset+1]=((curr_palette_value  & 0X00FF00)>>8);  //G
		   	bmp_copy[bmp_copy_offset+2]=((curr_palette_value  & 0xFF0000)>>16); //R
		}
	}

	if(color==0xFFFFFF){
		printf("Recovery>>>>>>>>");
		memcpy(bmp_copy,bmp_ori_ptr,bmp_size);
	}	
		

	//Save Img
	close(fd);
	fd=open(car_bmp,O_CREAT|O_WRONLY);

	if(fd != -1) {
		int ret=write(fd,bmp_copy,file_size_bmp);
		if(ret != file_size_bmp)
		 	printf("____>>%s: ret,file size mismatch! %d != %d,fail!\n",__FUNCTION__,ret,file_size_bmp);
		else
		 	printf("____>>%s: keepAVMIMG [%s] done!\n",__FUNCTION__,car_bmp);
	}

	free(bmp_ptr);
	free(bmp_ori_ptr);
	free(bmp_copy);
	free(color_mask);
	close(fd);
	return 0;
}

