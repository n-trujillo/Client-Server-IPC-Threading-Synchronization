#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <thread>

// new includes
#include <mutex>
#include <condition_variable>

using namespace std;

class BoundedBuffer
{
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	/* the queue of items in the buffer. Note
	that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length).
	While this would work, it is clearly more tedious */

	// add necessary synchronization variables and data structures 

	// lock (mutex)
	mutex m;

	// Condition Variables
	condition_variable empty;
	condition_variable full;

public:
	BoundedBuffer(int _cap){
		// set capacity
		cap = _cap;
	}

	~BoundedBuffer(){

	}

	bool is_empty() {
		return q.empty();
	}

	void push(char* data, int len){
		//1. Wait until there is room in the queue (i.e., queue lengh is less than cap)
		//2. Convert the incoming byte sequence given by data and len into a vector<char>
		//3. Then push the vector at the end of the queue


		// unique_lock will lock this mutex
		// once control leaves this function the mutex is unlocked
		unique_lock<mutex> ul(m);

		// this is a full condition variable
		// it will remain in this blocked state while it is "full"
		// it is no longer full when full is signaled by the pop function
		// this is because another thread has removed an element therefore it is not full
		// any time this thread is called to add an item to the full queue
		// it will unlock, block, and place itself back on the CV waitlist
		// it will only execute once full has been signaled by a pop thread
 		full.wait(ul, [this] {return (q.size() < cap);}); 

		// convert byte sequence (data, len) to vector<char>
		vector<char> vect(data, data + len);

		// push data to queue
		q.push(vect);

		// signal empty because queue is no longer empty
		empty.notify_one();
		ul.unlock();
		

		// once control leaves scope it is automatically unlocked
		// because of the use of unique_lock<mutex>
	}

	int pop(char* buf, int bufcap){
		//1. Wait until the queue has at least 1 item
		//2. pop the front item of the queue. The popped item is a vector<char>
		//3. Convert the popped vector<char> into a char*, copy that into buf, make sure that vector<char>'s length is <= bufcap
		//4. Return the vector's length to the caller so that he knows many bytes were popped

		// lock & unlock wrapper
		unique_lock<mutex> ul(m);

		// wait until atleast one element
		empty.wait(ul, [this] {return (q.size() > 0);});

		// grab pop
		vector<char> popped = q.front();
		q.pop();

		memcpy(buf, popped.data(), popped.size());

		full.notify_one();
		ul.unlock();
		

		// return vector length
		return popped.size();
	}
};

#endif /* BoundedBuffer_ */
