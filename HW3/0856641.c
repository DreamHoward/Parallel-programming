#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef W
#define W 20                                    // Width
#endif
int main(int argc, char **argv) {
  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
  int *temp = malloc(L*W*sizeof(int));          // Current temperature
  int *next = malloc(L*W*sizeof(int));          // Next time step
  //My var
  int my_rank;    //rank of process
  int p;          //number of process
  int source;     //rank of sender
  int dest;       //rank of reciever
  int tag=0;      //tag for messages
  int left,right; //range for caculate
  int message[20];
  int rmessage[20];
  int message2[20];
  int rmessage2[20];
  MPI_Status status;
  MPI_Request request;
  
  int min,mainmin;
  
  for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
       temp[i*W+j] = random()>>3;
    }
  }
  
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&p);
  
  left = (L/p)*(my_rank);
  right = (L/p)*(my_rank+1);
  
  if(my_rank == p-1)   // Last process caculate remain row
    right += (L%p);
  
 
  
  /*for (int j = 0; j < W; j++) {   //initial message
    message[j] = temp[(right-1)*W+j];
    message2[j] = temp[left*W+j];
  }  */
      
      
  int count = 0, balance = 0;
  while (iteration--) {     // Compute with up, left, right, down points
    balance = 1;
    count++;
    
    if(my_rank == 0){                                         //first
    
      
      MPI_Isend(temp+(right-1)*W, W, MPI_INT, my_rank+1, iteration, MPI_COMM_WORLD,&request);   //pass message
      /*if (block) {            
        break;
      }*/
      MPI_Recv(temp+right*W, W, MPI_INT, my_rank+1, iteration, MPI_COMM_WORLD, &status);  //recieve message
      
      /*for(int a = 0;a < W; a++){
        temp[(right-1)*W+a] = rmessage2[a];
      }*/
      
      for (int i = left; i < right; i++) {
        for (int j = 0; j < W; j++) {
          float t = temp[i*W+j] / d;  
          t += temp[i*W+j] * -4;
          t += temp[(i - 1 <  0 ? 0 : i - 1) * W + j];
          t += temp[(i + 1 >= L ? i : i + 1)*W+j];
          t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
          t += temp[i*W+(j + 1 >= W ? j : j + 1)];
          t *= d;
          /*if(i == right - 1){         //save message
            message[j] = t;
          }*/
          
          
          next[i*W+j] = t ;
          if (next[i*W+j] != temp[i*W+j]) {
            balance = 0;
          }
        }
      }
      
        
      
    }
    else if(my_rank == p-1){                                   //Last
      
      
      MPI_Isend(temp+left*W, W, MPI_INT, my_rank-1, iteration, MPI_COMM_WORLD,&request);   //pass message
      /*if (block) {            
        break;
      }*/
      MPI_Recv(temp+(left-1)*W, W, MPI_INT, my_rank-1, iteration, MPI_COMM_WORLD, &status);  //recieve message
      
      /*for(int a = 0;a < W; a++){
        temp[left*W+a] = rmessage[a];  
      }*/
      for (int i = left; i < right; i++) {
        for (int j = 0; j < W; j++) {
          float t = temp[i*W+j] / d;  
          t += temp[i*W+j] * -4;
          t += temp[(i - 1 <  0 ? 0 : i - 1) * W + j];
          t += temp[(i + 1 >= L ? i : i + 1)*W+j];
          t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
          t += temp[i*W+(j + 1 >= W ? j : j + 1)];
          t *= d;
          /*if(i == left){         //save message
            message2[j] = t;
          }*/
          next[i*W+j] = t ;
          if (next[i*W+j] != temp[i*W+j]) {
            balance = 0;
          }
        }
      }
    }else{  
    
      MPI_Isend(temp+(right-1)*W, W, MPI_INT, my_rank+1, iteration, MPI_COMM_WORLD,&request);   //pass message
      MPI_Isend(temp+(left)*W, W, MPI_INT, my_rank-1, iteration, MPI_COMM_WORLD,&request);   //pass message
      /*if (block) {            
        break;
      }*/
      MPI_Recv(temp+right*W, W, MPI_INT, my_rank+1, iteration, MPI_COMM_WORLD, &status);  //recieve message
      MPI_Recv(temp+(left-1)*W, W, MPI_INT, my_rank-1, iteration, MPI_COMM_WORLD, &status);   //recieve message
      
      
      /*for(int a = 0;a < W; a++){
        temp[left*W+a] = rmessage[a];
        temp[(right-1)*W+a] = rmessage2[a];
      }*/
      
      
      for (int i = left; i < right; i++) {
        for (int j = 0; j < W; j++) {
          float t = temp[i*W+j] / d;  
          t += temp[i*W+j] * -4;
          t += temp[(i - 1 <  0 ? 0 : i - 1) * W + j];
          t += temp[(i + 1 >= L ? i : i + 1)*W+j];
          t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
          t += temp[i*W+(j + 1 >= W ? j : j + 1)];
          t *= d;
          /*if(i == right - 1){         //save message
            message[j] = t;
          }
          if(i == left){         //save message
            message2[j] = t;
          }*/
          next[i*W+j] = t ;
          if (next[i*W+j] != temp[i*W+j]) {
            balance = 0;
          }
        }
      }
             
      
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&balance,1,MPI_INT,0,MPI_COMM_WORLD);
    if (balance) {       
      break;
    }
    
    int *tmp = temp;
    temp = next;
    next = tmp;
  }
  if(my_rank != 0){
    min = temp[left*W];
    for (int i = left; i < right; i++) {
      for (int j = 0; j < W; j++) {
        if (temp[i*W+j] < min) {
          min = temp[i*W+j];
        }
      }
    }
    
    MPI_Send(&min, 1, MPI_INT, 0, 3, MPI_COMM_WORLD);
    //MPI_Recv(&mainmin, 1, MPI_INT, 0, 4, MPI_COMM_WORLD, &status);
    
  }else{
    mainmin = temp[left*W];
    for (int i = left; i < right; i++) {
      for (int j = 0; j < W; j++) {
        if (temp[i*W+j] < mainmin) {
          mainmin = temp[i*W+j];
        }
      }
    }
    for(source=1;source<p;source++){
      MPI_Recv(&min, 1, MPI_INT, source, 3, MPI_COMM_WORLD, &status);
      mainmin = (mainmin<min) ? mainmin : min;
    }
    printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, mainmin);
    /*for(dest=1;dest<p;dest++){
      MPI_Send(&mainmin, 1, MPI_INT, dest, 4, MPI_COMM_WORLD);
    }*/
  }
  //printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, mainmin);

  MPI_Finalize();
  return 0;
}
