__kernel void enkrypcja(__global char* bookArray, __global int* keyList, __global char* cryptArray,__const int length)
{
	for(int i = get_global_id(0); i<length;i +=get_global_size(0))
	{
		cryptArray[i] = (bookArray[i] ^ keyList[i]);
	}
}

