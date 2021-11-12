#include <stdio.h> 
#include <stdlib.h> 
#include <iostream> 
#include <math.h>
#include <thread>
#include <string>
using namespace std;

int number_in_circle;
int test_T = 10000000;

void pi() {
	for (int toss = 0; toss < test_T; toss++) {
		double x = (double)rand() / (RAND_MAX + 1.0);
		double y = (double)rand() / (RAND_MAX + 1.0);
		//cout<< x << endl;
		float distance_sqared = x * x + y * y;
		if (distance_sqared <= 1) {
			number_in_circle++;
		}
	}
}
void main() {
	//int number_in_circle = 0;
	
	//thread mThread(pi);
	pi();
	float total= 4.0 * (float)number_in_circle / (float)test_T;
	cout << total << endl;
	system("pause");
}
