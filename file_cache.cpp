#include <fstream>
#include <string>
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "cache.h"
#include <sstream>
#ifdef PERFORMANCE_TEST
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>
struct rusage ruse;
extern int getrusage();
inline double cputime( void ) {
  getrusage( RUSAGE_SELF, &ruse );
	return ( ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec + 1e-6 * (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec ) );
}
inline void print_cputime( const char* msg, double run_time, unsigned long transactions = 0 ) {
	printf("  -> %s:  %7.3f seconds CPU time", msg, run_time );
	printf( "\n" );
}
#endif
#ifdef DEBUG
#define DBG_LOG(msg, ...) \
    std::cout << "Line : " << __LINE__ << "File :" << __FILE__ << "Function : " << __FUNCTION__ << " " << msg << std::endl;
#else
#define DBG_LOG(msg, ...) 
#endif
boost::mutex file_mutex;
#define  FILE_MUTEX  boost::lock_guard <boost::mutex> lock(file_mutex);

typedef Cache<int,float> cache;
cache *C;

static int get_line_count(const std::string& filename)
{
   DBG_LOG("Entering..."); 
   int index=0;
   std::ifstream in_file(filename.c_str());
   std::string s;
   
    while(in_file.peek()!=EOF) {
       getline(in_file, s);
       index++;
   }
   in_file.close();
   DBG_LOG("Exiting..."); 
   return index;
}
static std::string read_nth_line(const std::string& filename, int N)
{
   DBG_LOG("Entering..."); 
   std::ifstream in_file(filename.c_str());
   std::string s;

   for(int i = 1; i < N; ++i) {
       if(in_file.peek()==EOF) return "invalid";
       getline(in_file, s);
   }

   getline(in_file,s);
   in_file.close();
   DBG_LOG("Exiting..."); 
   return s; 
}

static void write_nth_line(const std::string& filename, int N,float value)
{
   DBG_LOG("Entering..."); 
   std::ifstream in_file(filename.c_str());
   std::ofstream out_file("temp.txt");
   std::string s;
   int index =1;

   while(in_file.peek()!=EOF) {
       getline(in_file, s);
       if (index != N)
          out_file << s << std::endl;
       else
          out_file << value << std::endl;
       index++;
   }
   in_file.close();
   out_file.close();
   remove(filename.c_str());
   rename("temp.txt",filename.c_str()); 
   DBG_LOG("Exiting..."); 
}
/* 
   @function - Reader thread callback.This handles cacche hit and cache miss 
               Cache hit :  read value from cache 
               Cache miss :  read from  database and update cache
   @design   - "Cache - Aside" read 
   @params   - reader file name and item file name
*/
static void reader_thread_fn(const std::string& filename, const std::string& base_filename)
{
   DBG_LOG("Entering..."); 

    std::ifstream in_file(filename.c_str());
    std::string out_filename;
    if(!in_file.is_open()) {
      std::cout << "Reader file :" << filename << "  open failed" << std::endl;
      return;
    }
    out_filename.append(filename,0,filename.length()-3);
    out_filename.append("out.txt");
    std::ofstream out_file(out_filename.c_str());
    std::string line = "";
    while (getline(in_file, line)){
       std::stringstream geek(line);
       int key = 0;
       geek >> key;
       if(C->exists(key)) {
          // Cache Hit
           out_file <<  C->get_value(key) << " cache" << std::endl;
       } else {
          // Cache misss
         FILE_MUTEX;
         std::string s = read_nth_line(base_filename,key);
         if (s == "invalid"){
            std::cout << "Invalid read position in Reader file : " << filename << std::endl;
            std::cout << "position : " << key << std::endl;
            continue; 
         }
         if(s.length() != 0){
             float value =0;
             std::stringstream geek(s);
             geek >> value;
             C->insert(key,value);
         }
         out_file <<  C->get_value(key) << " disk" << std::endl;
           
       }
   }
   in_file.close();
   out_file.close();
   DBG_LOG("Exiting..."); 
   return;
}
/* 
   @function - Writer thread callback.This handles writing value to cache and to database
   @design   - "Write through" cache
   @params   - writer file name and item file name
*/
static void writer_thread_fn(const std::string& filename, const std::string& base_filename)
{
    DBG_LOG("Entering..."); 

    std::ifstream in_file(filename.c_str());
    std::string line = "";
    if(!in_file.is_open()) {
      std::cout << "Writer file :" << filename << "  open failed" << std::endl;
      return;
    }
    while (getline(in_file, line)){
       std::vector<std::string> result; 
       boost::split(result, line, boost::is_any_of(" "));
       std::stringstream geek(result[0]);
       int key = 0;
       geek >> key;
       std::stringstream geek1(result[1]);
       float value= 0;
       geek1 >> value;
      
       C->insert(key,value);
       FILE_MUTEX;
       write_nth_line(base_filename,key,value);
    }
   in_file.close();
   DBG_LOG("Exiting..."); 
   return;
}

int main(int argc, char* argv[])
{ 
    DBG_LOG("Application started!!!"); 

    if (argc < 5 ) {
        std::cout << "Invalid arguments" << std::endl;
        std::cout << "Usage: <executable> <cache_size_in_integer> <reader_file_list> <writer_file_list> <item_file_list>" <<  std::endl; 
        return -1;
    }

    C = new cache(atoi(argv[1]));
    int num_reader=0,num_writer=0,index=0;

   //Check existence of item_file
    std::ifstream item_file(argv[3]); 
    if (!item_file.is_open() ||  !item_file.good()) {
        std::cout << "Failed to open file Item file!!!."<< std::endl;
        item_file.close();
        return -1;
    }
    item_file.close();

    num_reader = get_line_count(argv[2]);
    num_writer = get_line_count(argv[3]);
    boost::thread tid[num_reader];
    boost::thread tid_write[num_writer];

    if ((num_reader == 0) && (num_writer == 0)) {
       std::cout << "No Readers and Writers.Qutting application!!!" << std::endl;
       return -1;
    }

#ifdef PERFORMANCE_TEST
    double t0 = cputime();
#endif
    std::ifstream write_file(argv[3]); 
    if(num_writer) {
       DBG_LOG("Spawning Writer thread..."); 
       if (write_file.is_open() && write_file.good()) {
            std::string line = "";
            while (getline(write_file, line)){
               tid_write[index] = boost::thread(writer_thread_fn, line, argv[4]);
               index++;
            }
        
       } else {
           std::cout << "Failed to open Writer file!!!" << std::endl;
           write_file.close();
      }
    } else {
       std::cout << "No Writer present.No Writer thread spawned" << std::endl;
    }

    std::ifstream read_file(argv[2]); 
    if(num_reader) {
       DBG_LOG("Spawning Reader thread..."); 
       index=0;
       if (read_file.is_open() && read_file.good()) {
           std::string line = "";
           while (getline(read_file, line)){
	     tid[index]= boost::thread(&reader_thread_fn, line, argv[4]);
             index++; 
           }
        
       } else {
           std::cout << "Failed to open Reader file!!!";
           read_file.close();
       }
    } else {
       std::cout << "No Readers present.No Reader thread spawned" << std::endl;
    }

    for(index=0;index<num_writer;index++){
      tid_write[index].join();
    }
    for(index=0;index<num_reader;index++){
      tid[index].join();
    }
#ifdef PERFORMANCE_TEST
    double t1 = cputime();
    print_cputime( " Time taken by application", t1-t0, 0 );
#endif
    read_file.close();
    write_file.close();
    DBG_LOG("Application Exiting!!!"); 
    return 0;
}
