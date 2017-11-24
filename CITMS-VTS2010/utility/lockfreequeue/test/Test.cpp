// Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "../lib/LockFreeQueue.h"

#include <iostream>

using namespace std;

#ifdef _DEBUG
#pragma comment(lib, "../lib/lockfreequeued.lib")
#else
#pragma comment(lib, "../lib/lockfreequeue.lib")
#endif

CLockFreeQueue LockFreeQueue;

void enqueue()
{
    for (int i=0; i<10; i++)
    {
        fifocell* cell_ptr = new fifocell;
        cell_ptr->value[0] = i*2;
        LockFreeQueue.enqueue(cell_ptr);
    }
}

void dequeue()
{
    while (1)
    {
        fifocell* cell_ptr = LockFreeQueue.dequeue();
        if (!cell_ptr) break;

        cout << cell_ptr->value[0] << endl;
        delete cell_ptr;
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    cout << "Enqueue..." << endl;
    enqueue();
    cout << "Dequeue..." << endl;
    dequeue();
    cin.get();
	return 0;
}

