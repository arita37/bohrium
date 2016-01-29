#include <sstream>
#include <string>
#include <map>
#include "codegen.hpp"

using namespace std;
using namespace kp::core;

namespace kp{
namespace engine{
namespace codegen{

Block::Block(void) : _template_fn("block.tpl") {}
Block::Block(std::string template_fn) : _template_fn(template_fn) {}

void Block::prolog(string source)
{
    prolog_ << source;    
}

void Block::epilog(string source)
{
    epilog_ << source;    
}

void Block::pragma(string source)
{
    pragma_ << source;    
}

void Block::head(string source)
{
    head_ << source;    
}

void Block::body(string source)
{
    body_ << source;    
}

void Block::foot(string source)
{
    foot_ << source;    
}

std::string Block::prolog(void)
{
    return prolog_.str();
}

std::string Block::epilog(void)
{
    return epilog_.str();
}

std::string Block::pragma(void)
{
    return pragma_.str();
}

std::string Block::head(void)
{
    return head_.str();
}

std::string Block::body(void)
{
    return body_.str();
}

std::string Block::foot(void)
{
    return foot_.str();
}

std::string emit(void)
{
    std::map<string, string> subjects;

    subjects["PROLOG"] = prolog();
    subjects["EPILOG"] = epilog();
    subjects["PRAGMA"] = pragma();
    subjects["HEAD"] = head();
    subjects["BODY"] = body();
    subjects["FOOT"] = foot();

    return plaid_.fill(template_fn_, subjects);
}

}}}
