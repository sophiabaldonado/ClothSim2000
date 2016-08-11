//
//  main.cpp
//  ClothSim2000
//
//  Created by Sophia Baldonado on 8/9/16.
//  Copyright © 2016 Sophia Baldonado. All rights reserved.
//

#include <cassert>
#include <iostream>
using std::cout;
using std::endl;

void printStuff()
{
	// pointers
	// int var1 = 5;
	// int var2 = 10;
	//
	// int *pointer1 = &var1;
	// int *pointer2 = &var1;
	//
	// cout << *pointer1 << endl;
	// cout << *pointer2 << endl;
	//
	// pointer2 = &var2;
	// cout << *pointer2 << endl;
	//
	// *pointer2 = 23;
	//
	// cout << *pointer2 << endl;
	// cout << var2 << endl;
	//
	// cout << *pointer1 << endl;
	// cout << var1 << endl;
	//
	// pointer1 = pointer2;
	// cout << *pointer1 << endl;
	// cout << var1 << endl;

	// pointer arithmetic
	// int *pointer = new int[5];
	// cout << pointer << endl;
	// for (int i = 0; i < 5; i++)
	// 	pointer[i] = i * 2;
	//
	// cout << "\nfirst lopp" << endl;
	// for (int i = 0; i < 5; i++)
	// 	cout << pointer[i] << endl;
	//
	// int *p = pointer;
	// cout << p << endl;
	//
	// cout << "\nnext lopp" << endl;
	// for (int *p = pointer; p < pointer + 5; p++)
	// 	cout << *p << endl;
	// delete [] pointer;
}
void function2()
{
	// modify a data lol
	int var2;
	int *pointer = &var2;
	for (int i = 0; i < 30; i++)
	{
		// cout << *(pointer + i) << endl;
		// cout << "\n" << endl;
		cout << *(pointer + i) << endl;
		if (*(pointer + i) == 50)
		{
			cout << "found 50!\n" << endl;
			// *(pointer + i) = 8888;
		}
	}
}

void function1()
{
	int var1[5];
	var1[3] = 51;
	cout << "\nfirst var1" << endl;
	cout << var1 << endl;
	cout << "\nmod now\n";
	function2();
	cout << "\nsecond var1" << endl;
	cout << var1 << endl;
}


int main(int argc, const char * argv[]) {
    // insert code here...
    cout << "Hello, World!\n";
    printStuff();
		function1();
    return 0;
}
