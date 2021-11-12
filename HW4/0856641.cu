/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265

void check_param(void);
void update (void);
void printfinal (void);

int nsteps,                 	 /* number of time steps */
    tpoints, 	     		         /* total points along string */
    rcode;                  	 /* generic return code */

float  values[MAXPOINTS+2], 	 /* values at time t */
       old_val[MAXPOINTS+2], 	 /* values at time (t-dt) */
       new_val[MAXPOINTS+2]; 	 /* values at time (t+dt) */


float  *device_value;

/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n",
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);
}


__global__ void parallel(float *device_value, int tpoints, int nsteps)
{
    // init_line() 
    int i, k;
    int blockSize = 1024;
    float value, x, fac, tmp, old_val;
    
    fac = 2.0 * PI;
    k = 1 + blockIdx.x * blockSize + threadIdx.x;
    tmp = tpoints - 1;
    x = (k - 1) / tmp;
    value = sin (fac * x);
    old_val = value;

    // do_math() 
    float dtime, c, dx, tau, sqtau;
    
    dtime = 0.3;
    c = 1.0;
    dx = 1.0;
    tau = (c * dtime / dx);
    sqtau = tau * tau;

    // update() 
    float new_val;
    
    if(k <= tpoints) {
      for (i = 1; i<= nsteps; i++) {
        if ((k == 1) || (k  == tpoints))
          new_val = 0.0;
        else
          new_val = (2.0 * value) - old_val + (sqtau * -2.0 * value);
        old_val = value;
        value = new_val;
      }
      device_value[k] = value;
    }
}

/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i % 10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
    sscanf(argv[1], "%d", &tpoints);
  	sscanf(argv[2], "%d", &nsteps);
    check_param();
    
    int size = (tpoints + 1) * sizeof(float);
    int blockSize = 1024;
    int block_num = tpoints / blockSize + !(tpoints % blockSize == 0);
    
    //cuda malloc
    cudaMalloc((void**) &device_value, size);

    printf("Initializing points on the line...\n");
  	printf("Updating all points for all time steps...\n");
    //parallel
    parallel<<<block_num, blockSize>>>(device_value, tpoints, nsteps);

    cudaMemcpy(values, device_value, size, cudaMemcpyDeviceToHost);
    cudaFree(device_value);

    printf("Printing final results...\n");
  	printfinal();
  	printf("\nDone.\n\n");

	return 0;
}