#include <iostream>
#include<CL\cl.h>
#include <list>
#include <chrono>
#include <ctime>
#include <ratio>
#include<bitset>
#include<fstream>
#include<string>

using namespace std;
using namespace std::chrono;
#define MAX_SOURCE_SIZE (0x100000)
#define BLOCK_DIM 1024
#pragma warning(disable:4996)

int getRandNumb(long long int M, long long int x0) {
	long long int x1 = (x0*x0) % M;
	return x1;
}

int decypherRandNumb(long long int p, long long int q, long long int x1) {
	long long int x0a, x0b, x0;
	long long int s1 = 1, s2 = 1;

	bool znaleziony = true;
	x0a = (long long int)(pow(x1, (p + 1) / 4)) % p;
	x0b = (long long int)(pow(x1, (q + 1) / 4)) % q;
	
	while (znaleziony)
	{
		if ((q*s1) % p == 1)
			znaleziony = false;
		else
			s1++;
	}
	znaleziony = true;
	while (znaleziony)
	{
		if ((p*s2) % q == 1)
			znaleziony = false;
		else
			s2++;
	}
	x0 = (q*s1*x0a + p*s2*x0b) % (p*q);
	return x0;
}

list<char> encrypt(list<char> list_to_crypt, list<long long int> key_list)
{
	list<char> _crypted_list;
	list<long long int>::iterator it = key_list.begin();
	for (char element : list_to_crypt)
	{
		_crypted_list.push_back(element ^ *it);
		advance(it, 1);
	}
	return _crypted_list;
}

int main()
{
	string hamlet_file = "shakespeare-hamlet.txt";
	string alice_file = "alice_in_wonderland.txt";
	string bible_file = "bible.txt";
	int p = 19;
	int q = 11;
	int s = 3;
	list<long long int> rand_list;
	list<long long int> decyphered_rand_list;
	list<char> book;
	ifstream myfile;

#pragma region WCZYTYWANIE_PLIKOW

	//WCZYTYWANIE PLIKOW
	myfile.open(bible_file);
	if (!myfile.is_open())
		exit(1);
	string line;
	while (getline(myfile, line))
	{
		for (char character : line)
		{
			book.push_back(character);
		}
	}
	myfile.close();

#pragma endregion

	long int cykle = book.size();	//141064- ALICE, 157959-HAMLET
	cout << "Ilosc znakow w ksiazce: "<<cykle << endl;
#pragma region CPU
	high_resolution_clock::time_point BBS_timer_start = high_resolution_clock::now();
	//GENEROWANIE BLUM BLUM SCHUB
	for (int i = 0; i < cykle; i++)
	{
		s = getRandNumb(p*q, s);
		rand_list.push_back(s);
	}
	//ODWARACANIE BLUM BLUM SCHUB
	long long int x0 = rand_list.back();
	for (long long int element : rand_list)
	{	
		decyphered_rand_list.push_front(x0);
		x0 = decypherRandNumb(p,q, x0);
	}
	high_resolution_clock::time_point BBS_timer_stop = high_resolution_clock::now();
	duration<double> BBS_timer = duration_cast<duration<double>>(BBS_timer_stop- BBS_timer_start);
	cout << "BBS took me " << BBS_timer.count() << " seconds." << endl << endl;

	//ENKRYPCJA I DEKRYPCJA
	high_resolution_clock::time_point CPU_ENCRYPT_start = high_resolution_clock::now();

	list<char>encrypted_list =  encrypt(book, rand_list);

	high_resolution_clock::time_point CPU_ENCRYPT_stop = high_resolution_clock::now();
	duration<double> CPU_ENCRYPT_timer = duration_cast<duration<double>>(CPU_ENCRYPT_stop- CPU_ENCRYPT_start);
	cout << "ENCRYPTION ON CPU took me " << CPU_ENCRYPT_timer.count() << " seconds." << endl;

	high_resolution_clock::time_point CPU_DECRYPT_start = high_resolution_clock::now();

	list<char>decrypted_list =  encrypt(encrypted_list, decyphered_rand_list);

	high_resolution_clock::time_point CPU_DECRYPT_stop = high_resolution_clock::now();
	duration<double> CPU_DECRYPT_timer = duration_cast<duration<double>>(CPU_DECRYPT_stop - CPU_DECRYPT_start);
	cout << "DECRYPTION ON CPU took me " << CPU_ENCRYPT_timer.count() << " seconds." << endl;

	duration<double> CPU_CRYPT = CPU_DECRYPT_timer + CPU_ENCRYPT_timer;
	cout << "WORK ON CPU took me " << CPU_CRYPT.count() << " seconds." << endl;



#pragma endregion

#pragma region GPU

	//ZMIENNE
	int dlugosc_tablicy = cykle;
	char *_book_array = (char*)malloc(sizeof(char)*dlugosc_tablicy);
	int *GPU_BBS_numbers = (int*)malloc(sizeof(int)*dlugosc_tablicy);
	char *GPU_ENcrypted_list = (char*)malloc(sizeof(char)*dlugosc_tablicy);
	char *GPU_DEcrypted_list = (char*)malloc(sizeof(char)*dlugosc_tablicy);

	int n = 0;
	for (char element : book)
	{
		_book_array[n] = element;
		n++;
	}
	n = 0;
	for (int element : rand_list)
	{
		GPU_BBS_numbers[n] = element;
		n++;
	}

	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("kernel.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int error = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1,
		&device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &error);

	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &error);

	cl_mem d_book_array = clCreateBuffer(context, CL_MEM_READ_ONLY,
		 sizeof(char) * dlugosc_tablicy, NULL, &error);
	cl_mem d_GPU_BBS_numbers = clCreateBuffer(context, CL_MEM_READ_ONLY,
		sizeof(int)*dlugosc_tablicy, NULL, &error);
	cl_mem d_GPU_ENcrypted_list = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		 sizeof(char) * dlugosc_tablicy, NULL, &error);


	error = clEnqueueWriteBuffer(command_queue, d_book_array, CL_TRUE, 0,
		sizeof(char) * dlugosc_tablicy, _book_array, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(command_queue, d_GPU_BBS_numbers, CL_TRUE, 0,
		sizeof(int) * dlugosc_tablicy, GPU_BBS_numbers, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1,
		(const char **)&source_str, (const size_t *)&source_size, &error);

	error = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	cl_kernel shared = clCreateKernel(program, "enkrypcja", &error);

	error = clSetKernelArg(shared, 0, sizeof(cl_mem), (void *)&d_book_array);
	error = clSetKernelArg(shared, 1, sizeof(cl_mem), (void *)&d_GPU_BBS_numbers);
	error = clSetKernelArg(shared, 2, sizeof(cl_mem), (void *)&d_GPU_ENcrypted_list);
	error = clSetKernelArg(shared, 3, sizeof(cl_mem), (void *)&dlugosc_tablicy);

	size_t global_item_size[3] = {BLOCK_DIM , 0 , 0}; 
	size_t local_item_size[3] = {BLOCK_DIM , 0 , 0 }; 

	high_resolution_clock::time_point GPU_ENCRYPT_start = high_resolution_clock::now();

	error = clEnqueueNDRangeKernel(command_queue, shared, 1, NULL,
		global_item_size, local_item_size, 0, NULL, NULL);

	high_resolution_clock::time_point GPU_ENCRYPT_stop = high_resolution_clock::now();

	error = clEnqueueReadBuffer(command_queue, d_GPU_ENcrypted_list, CL_TRUE, 0,
		sizeof(char) * dlugosc_tablicy, GPU_ENcrypted_list, 0, NULL, NULL);

	duration<double> GPU_ENCRYPT_timer = duration_cast<duration<double>>(GPU_ENCRYPT_stop - GPU_ENCRYPT_start);
	cout << "ENCRYPTION ON GPU took me " << GPU_ENCRYPT_timer.count() << " seconds." << endl;

	error = clReleaseMemObject(d_book_array);
	error = clReleaseMemObject(d_GPU_BBS_numbers);
	error = clReleaseMemObject(d_GPU_ENcrypted_list);

	 d_book_array = clCreateBuffer(context, CL_MEM_READ_ONLY,
		sizeof(char) * dlugosc_tablicy, NULL, &error);
	 d_GPU_BBS_numbers = clCreateBuffer(context, CL_MEM_READ_ONLY,
		sizeof(int)*dlugosc_tablicy, NULL, &error);
	cl_mem d_GPU_DEcrypted_list = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
		sizeof(char) * dlugosc_tablicy, NULL, &error);

	error = clEnqueueWriteBuffer(command_queue, d_book_array, CL_TRUE, 0,
		sizeof(char) * dlugosc_tablicy, GPU_ENcrypted_list, 0, NULL, NULL);
	error = clEnqueueWriteBuffer(command_queue, d_GPU_BBS_numbers, CL_TRUE, 0,
		sizeof(int) * dlugosc_tablicy, GPU_BBS_numbers, 0, NULL, NULL);

	error = clSetKernelArg(shared, 0, sizeof(cl_mem), (void *)&d_book_array);
	error = clSetKernelArg(shared, 1, sizeof(cl_mem), (void *)&d_GPU_BBS_numbers);
	error = clSetKernelArg(shared, 2, sizeof(cl_mem), (void *)&d_GPU_DEcrypted_list);
	error = clSetKernelArg(shared, 3, sizeof(cl_mem), (void *)&dlugosc_tablicy);

	high_resolution_clock::time_point GPU_DECRYPT_start = high_resolution_clock::now();

	error = clEnqueueNDRangeKernel(command_queue, shared, 1, NULL,
		global_item_size, local_item_size, 0, NULL, NULL);

	high_resolution_clock::time_point GPU_DECRYPT_stop = high_resolution_clock::now();

	duration<double> GPU_DECRYPT_timer = duration_cast<duration<double>>(GPU_DECRYPT_stop - GPU_DECRYPT_start);

	error = clEnqueueReadBuffer(command_queue, d_GPU_DEcrypted_list, CL_TRUE, 0,
		sizeof(char) * dlugosc_tablicy, GPU_DEcrypted_list, 0, NULL, NULL);

	duration<double> GPU_CRYPT = GPU_DECRYPT_timer + GPU_ENCRYPT_timer;
	cout << "DECRYPTION ON GPU took me " << GPU_DECRYPT_timer.count() << " seconds." << endl;
	cout << "WORK ON GPU took me " << GPU_CRYPT.count() << " seconds." << endl;

	duration<double> accceleration = CPU_CRYPT/GPU_CRYPT.count();

	cout << "Przyspieszenie " << accceleration.count() << endl;


#pragma endregion

#pragma region ZAPIS_DO_PLIKU
	//ZAPIS DO PLIKU
	ofstream output_file_CPU;
	output_file_CPU.open("ENCRYPTED_FILE_CPU.txt");
	for (char i : encrypted_list)
	{
		output_file_CPU << i;
	}
	output_file_CPU.close();

	output_file_CPU.open("DECRYPTED_FILE_CPU.txt");
	for (char i : decrypted_list)
	{
		output_file_CPU << i;
	}
	output_file_CPU.close();
	
	ofstream output_file_GPU;
	output_file_GPU.open("ENCRYPTED_FILE_GPU.txt");
	for (int i = 0; i<dlugosc_tablicy;i++)
	{
		output_file_GPU << GPU_ENcrypted_list[i];
	}
	output_file_GPU.close();

	output_file_GPU.open("DECRYPTED_FILE_GPU.txt");
	for (int i = 0; i < dlugosc_tablicy; i++)
	{
		output_file_GPU << GPU_DEcrypted_list[i];
	}
	output_file_GPU.close();

#pragma endregion

	error = clReleaseMemObject(d_book_array);
	error = clReleaseMemObject(d_GPU_BBS_numbers);
	error = clReleaseMemObject(d_GPU_DEcrypted_list);
	error = clFinish(command_queue);
	error = clReleaseKernel(shared);
	error = clReleaseProgram(program);
	error = clReleaseCommandQueue(command_queue);
	error = clReleaseContext(context);
	cout << endl;
	system("pause");
	return 0;
}