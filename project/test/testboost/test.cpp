#include <iostream>
#include <boost/bind.hpp>
#include <string>
using namespace std;

class Hellols
{
public:
    void say(string name)
    {
        cout << name << " say: hello world!" << endl;
    }
};

int main()
{
    Hellols h;
    auto func = boost::bind(&Hellols::say, &h, "liubei");
    func();
    return 0;
}
