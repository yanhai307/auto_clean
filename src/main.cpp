//
// Created by YANHAI on 2019/5/5.
//

#include <iostream>
using namespace std;

/* we need this to stringify the defines which are supplied at compiletime see:
   http://gcc.gnu.org/onlinedocs/gcc-3.4.1/cpp/Stringification.html#Stringification */
#define xstr(s) str(s)
#define str(s) #s

void printVersion()
{
    cout << "This is " << PROG_NAME << " version " << PROG_VER << " " << xstr(REVISION) << endl;
}

int main()
{
    printVersion();
    return 0;
}