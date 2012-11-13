//! Test-Application to check c++11 support.
/*!
 * @file CX0TestApp.cpp
 * This tiny application test some features of the new c++11 standard. One can
 * compile and run this to check the c++11 support of the used compiler/system.
 * The features tested are: shared_pointer, raw strings, auto type's, lamdas,
 * threading and regular expressions.
*/

#include <iostream>
#include <memory>
#include <string>
#include <regex>
#include <algorithm>
#include <thread>

using namespace std;

static const int num_threads = 10;

//This function will be called from a thread

void call_from_thread(unsigned int tid) {
  cout << "Launched by thread " << tid << endl;
}

int main(int argc, char **argv) {

  //========= c++11 test ==========
  string hello = "say hello to c++11 Basics: ";
  int test = 7;
  auto neu = test;
  shared_ptr<int> test_ptr(new int(8));
  auto* auto_ptr(new int(9));
  cout << "String: "<< hello << endl;
  cout << "auto type: " << neu << endl;
  cout << "shared pointer:  " << *test_ptr << endl;
  cout << "auto pointer:  " << *auto_ptr << endl;
  cout << endl;


  //========= Raw Strings ==========
  cout << "Raw Strings: " << endl;
  string normal_str = "First line.\nSecond line.\nEnd of message.\n";
  string raw_str = R"(First line.\nSecond line.\nEnd of message.\n)";
  cout<<normal_str<<endl;
  cout<<raw_str<<endl;
  cout << endl;

  //=========  Lambdas  ==========
  cout << "Lamda-functions: " << endl;
  int n=10;
  vector<int> v(n);
  //initialize the vector with values from 10 to 1
  for(int i = n - 1, j = 0; i >= 0; i--, j++) v[j] = i + 1;
  //print the unsorted vector
  for(int i = 0; i < n; i++) cout << v[i] << " "; cout << endl;
  //sort the vector
  sort(v.begin(),v.end(),[](int i, int j) -> bool{ return (i < j);});
  //print the sorted vector
  for(int i = 0; i < n; i++) cout << v[i] << " "; cout << endl;
  cout << endl;

  //=========  Threads  ==========
  cout << "Threads: " << endl;
  thread t[num_threads];
  //Launch a group of threads
  for (int i = 0; i < num_threads; ++i) {
      t[i] = thread(call_from_thread, i);
  }
  cout << "Launched from the main\n";
  //Join the threads with the main thread
  for (int i = 0; i < num_threads; ++i) {
      t[i].join();
  }
  cout << endl;

  //========= Regular Expr. ==========
  DataIFTestApp.cpp  cout << "Regular Expressions: " << endl;
  cout << "Not supported by gcc up to 4.6 " << endl;
  string input;
  regex integer("(\\+|-)?[[:digit:]]+");
  //As long as the input is correct ask for another number
  while(true)
  {
    cout<<"Give me an integer!"<<endl;
    cin>>input;
    //Exit when the user inputs q
    if(input=="q")
      break;
    if(regex_match(input,integer))
      cout<<"integer"<<endl;
    else{
      cout<<"Invalid input"<<endl;
    }
  }
  cout << endl;

  return 0;
}
