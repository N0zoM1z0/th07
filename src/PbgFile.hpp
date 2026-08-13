#pragma once

namespace th07
{
// The exact target vtable at 0x0049526C proves seven abstract operations before
// the virtual destructor.  Their parameter contracts remain to be recovered
// from CPbgFile, so retain neutral placeholders rather than inventing them in
// this base declaration.
class IPbgFile
{
  public:
    virtual void UnknownOperation0() = 0;
    virtual void UnknownOperation1() = 0;
    virtual void UnknownOperation2() = 0;
    virtual void UnknownOperation3() = 0;
    virtual void UnknownOperation4() = 0;
    virtual void UnknownOperation5() = 0;
    virtual void UnknownOperation6() = 0;
    virtual ~IPbgFile();
};
} // namespace th07
