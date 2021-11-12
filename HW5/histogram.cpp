#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <vector>
#include <stdint.h>
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

using namespace std;

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

cl_program load_program(cl_context context, const char* filename,cl_device_id did)
{
    std::ifstream in(filename, std::ios_base::binary);
    if(!in.good()) {
      return 0;
    }
    // get file length
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);
    
    // read program source
    std::vector<char> data(length + 1);
    in.read(&data[0], length);
    data[length] = 0;
    
    // create and build program 
    const char* source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    if(program == 0) {
      cout<<"program = 0" <<endl;
      return 0;
    }
    
    if(clBuildProgram(program, 1, &did, NULL, NULL, NULL) != CL_SUCCESS) {
      if(clBuildProgram(program, 0, NULL, NULL, NULL, NULL) != CL_SUCCESS){
        cout<<"build fail" <<endl;
        char buffer[2048];
        clGetProgramBuildInfo(program, did, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        cout<<"--- Build log ---\n "<<buffer<<endl;
        exit(1);
        }
    }
    return program;
}

int main(int argc, char *argv[])
{
    
  	
    
    char *filename;
    if (argc >= 2)
    {    
       
        int many_img = argc - 1;
        for (int i = 0; i < many_img; i++)
        {
            
            filename = argv[i + 1];
            Image *img = readbmp(filename);

            std::cout << img->weight << ":" << img->height << "\n";
            uint32_t w = img->weight;
            uint32_t h = img->height;
            
            uint32_t results[256 * 3];
            std::fill(results, results+(256*3), 0);  
            size_t max_items;
          	size_t max_work[3];
          	size_t local_work_size;
          	size_t global_work_size;
            cl_int err_device, err;
            cl_kernel kernel;
          	cl_context context;
          	cl_program program;
          	cl_command_queue cmd_queue;
            cl_device_id did;
          	cl_platform_id pid;
          	size_t offset;
          	cl_mem input; 
          	cl_mem output;
           
            unsigned int *image = new unsigned int[w*h*3];
            for(int i=0 ; i<w*h ; i++ ){
              image[i*3] = img->data[i].R;
              image[i*3+1] = img->data[i].G;
              image[i*3+2] = img->data[i].B;
            }
            
            
            clGetPlatformIDs(1, &pid, NULL);
            
            err_device = clGetDeviceIDs(pid, CL_DEVICE_TYPE_GPU, 1, &did, NULL);
          	if (err_device != CL_SUCCESS) {
          		printf("clGetDeviceIDs is error\n");
          		return EXIT_FAILURE;
          	}
          
          	err_device = clGetDeviceInfo(did, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work), &max_work, NULL);
          	if (err_device != CL_SUCCESS) {
          		printf("clGetDeviceInfo() is error\n");
          		return EXIT_FAILURE;
          	}
                     
          	max_items = max_work[0] * max_work[1] * max_work[2];
           
            context = clCreateContext(0, 1, &did, NULL, NULL, &err);
           	if (!context) {
           		printf("clCreateContext()is error\n");
           		return EXIT_FAILURE;
           	}
            
            cmd_queue = clCreateCommandQueue(context, did, 0, &err);
           	if (!cmd_queue) {
           		printf("clCreateCommandQueue()is error\n");
           		return EXIT_FAILURE;
           	}
            if (err != CL_SUCCESS) {
           		printf("clCreateCommandQueue()is error\n");
           		return EXIT_FAILURE;
           	}
            
            program = load_program(context, "histogram.cl", did);
           	if (!program) {
           		return EXIT_FAILURE;
           	}
           	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
            if (err != CL_SUCCESS) {
           		printf("clBuildProgram()is error\n");
           		return EXIT_FAILURE;
           	}    
            
            kernel = clCreateKernel(program, "histogram", &err);
          	if (!kernel || err != CL_SUCCESS) {
          		return EXIT_FAILURE;
          	}
           
           
          	input = clCreateBuffer(context, CL_MEM_READ_ONLY, (w * h * sizeof(uint32_t)*3), NULL, &err);
             if (err != CL_SUCCESS) { 
            		printf("clCreateBuffer(): input is error\n");
            		return EXIT_FAILURE;
            	}
           
          	output = clCreateBuffer(context, CL_MEM_READ_WRITE, 3 * 256 * sizeof(uint32_t), NULL, &err);
             if (err != CL_SUCCESS) {  
            		printf("clCreateBuffer(): output is error\n");
            		return EXIT_FAILURE;
            	}
           
           
            
            offset = 0;
          	err = clEnqueueWriteBuffer(cmd_queue, input, CL_TRUE, offset, (w * h * sizeof(uint32_t)*3), image, 0, NULL, NULL);
            
          	if (err != CL_SUCCESS) {
          		printf("clEnqueueWriteBuffer(): is error\n");
          		return EXIT_FAILURE;
          	}
            //clFinish(cmd_queue);
          
          	err = 0;
            err = clSetKernelArg(kernel, 0,  sizeof(cl_mem), &input);//RGB
            if (err != CL_SUCCESS) {
          		printf("clSetKernelArg(): input is error\n");
          		return EXIT_FAILURE;
          	}
           
          	err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
            if (err != CL_SUCCESS) {
          		printf("clSetKernelArg(): results is error\n");
          		return EXIT_FAILURE;
          	}
           
            uint32_t total_tasks =w * h;
            uint32_t task_per_thread = total_tasks / max_items + 1;
            //total_tasks =1;
            //task_per_thread = 1;
            
            err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &total_tasks);
            if (err != CL_SUCCESS) {
          		printf("clSetKernelArg(): total_tasks is error\n");
          		return EXIT_FAILURE;
            }
            err = clSetKernelArg(kernel, 3, sizeof(uint32_t), &task_per_thread);
            if (err != CL_SUCCESS) {
          		printf("clSetKernelArg(): task_per_thread is error\n");
          		return EXIT_FAILURE;
            }
          
           
            err = clGetKernelWorkGroupInfo(kernel, did, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local_work_size), &local_work_size, NULL);
          	if (err != CL_SUCCESS) {
          		printf("clGetKernelWorkGroupInfo()is error\n");
          		return EXIT_FAILURE;
          	}
          	global_work_size = max_items; 
           //global_work_size = 128; 
           //local_work_size = 64;
          	err = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
          	if (err != CL_SUCCESS) {
          		printf("clEnqueueNDRangeKernel(): is error\n");
          		return EXIT_FAILURE;
          	}
            
          	clFinish(cmd_queue);
          
          	err = clEnqueueReadBuffer(cmd_queue,
             output, 
             CL_TRUE, 
             offset, 
             3 * 256 * sizeof(uint32_t), 
             results, 
             0, 
             NULL, 
             NULL);
             
           
          	if (err != CL_SUCCESS) {
          		printf("clEnqueueReadBuffer(): is error\n");
          		return EXIT_FAILURE;
          	}
          
          	clReleaseMemObject(input);
          	clReleaseMemObject(output);
          	clReleaseProgram(program);
          	clReleaseKernel(kernel);
          	clReleaseCommandQueue(cmd_queue);
          	clReleaseContext(context);
                     
          	delete[] img;

           
            int max = 0;
            for(int i=0;i < 256*3 ;i++){
                max = results[i] > max ? results[i] : max;
            }

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];
            fill( &(ret->data[0].R), &(ret->data[256 * 256].B), 0);
            
            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){ 
                    if(results[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
                        
                    if(results[256 + j]*256/max > i)
                        ret->data[256*i+j].G = 255;
                        
                    if(results[512 + j]*256/max > i)
                        ret->data[256*i+j].B = 255;
                }
            }

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);
        }
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}
