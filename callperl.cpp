#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#include <string>
#include <stdexcept>
#include <iostream>

#include <EXTERN.h>
#include <perl.h>

using namespace tbb;
using namespace std;

EXTERN_C void xs_init (pTHX);

EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);

EXTERN_C void
xs_init(pTHX)
{
  const char *file = __FILE__;
  dXSUB_SYS;

  /* DynaLoader is a special case */
  newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

static const char *_sample = "10000";
static const char *_length = "1000000";
static const char *_rare = "0.01";
static const char *_permutation = "400000";
static const char *_control[] = { "500", "1000", "2000", "1500", "2050" };
static const char *_case[] = { "500", "1000", "2000", "1500", "2050" };
static const char *_target[] = { "15000", 
                                 "30000",
                                 "15000",
                                 "15000",
                                 "15000" };
static const char *_causal[] = { "10",
                                 "10",
                                 "5",
                                 "10",
                                 "10" };
static const char *_variance[] = { "0.001",
                                   "0.001",
                                   "0.001",
                                   "0.001",
                                   "0.0005" };
static const char *_prevalence[] = { "0.01",
                                     "0.01",
                                     "0.01",
                                     "0.05",
                                     "0.01" };

class PerlTask {
  const int _patternA;
  const int _patternB;
public:
  PerlTask(int patternA, int patternB): 
    _patternA(patternA-1), _patternB(patternB-1) {
    if(patternA < 1 || patternA > 5) 
      throw out_of_range("patternA must inside [1,5]");
    if(patternB < 1 || patternB > 5)
      throw out_of_range("patternB must inside [1,5]");
  }

  void run() const {
    PerlInterpreter *my_perl;
    my_perl = perl_alloc();
    perl_construct(my_perl);
      
    const char *perl_argv[] = { "", "c_alpha_test_2.plx",
                                    "--genome", "aho.txt",
                                    "--sample", _sample,
                                    "--length", _length,
                                    "--target", _target[_patternB],
                                    "--causal", _causal[_patternB],
                                    "--rare", _rare,
                                    "--case", _case[_patternA],
                                    "--control", _control[_patternA],
                                    "--variance", _variance[_patternB],
                                    "--prevalence", _prevalence[_patternB],
                                    "--permutation", _permutation };
    
    perl_parse(my_perl, xs_init, 24, const_cast<char**>(perl_argv), NULL);
    perl_run(my_perl);
    perl_destruct(my_perl);
    perl_free(my_perl);
  } 

  void operator()( const blocked_range<size_t>& r ) const {
    for ( int i=r.begin(); i!=r.end(); ++i ){
      run();
    }
  }
};

int main(int argc, char **argv, char **env)
{
  //PERL_SYS_INIT3(&argc, &argv, &env);
  //PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
  
  if (argc < 2 || argc > 3){
    cout << "Usage: callperl pattern_number [ num_of_repeat ]" << endl;
    cout << "  pattern_number = 1 : control = 500, case = 500" << endl;
    cout << "  pattern_number = 2 : control = 1000, case = 1000" << endl;
    cout << "  pattern_number = 3 : control = 2000, case = 2000" << endl;
    cout << "  pattern_number = 4 : control = 1500, case = 1500" << endl;
    cout << "  pattern_number = 5 : control = 2050, case = 2050" << endl;
    exit(1);
  }

  int patternA = atoi(argv[1]);
  
  int NUM = 100;
  if (argc == 3) NUM = atoi(argv[2]);
  
  for (int patternB = 1; patternB <=5; ++patternB) {
    PerlTask perlTask(patternA,patternB);
    tick_count t2 = tick_count::now();
    parallel_for( blocked_range<size_t>(0,NUM), perlTask, auto_partitioner() );
    tick_count t3 = tick_count::now();
    cout << "parallel" << endl;
    cout << "numoftimes: " << NUM << endl;
    cout << "total: " << (t3-t2).seconds() << endl;
    cout << "average: " << (t3-t2).seconds()/(float)NUM << endl;
    cout << endl;
  }
  PERL_SYS_TERM();
}
