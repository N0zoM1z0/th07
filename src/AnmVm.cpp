#include "inttypes.hpp"

namespace th07 {

struct AnmVm {
    u8 unused_00[0xc8];
    i32 intVar0;
    i32 intVar1;
    i32 intVar2;
    i32 intVar3;
    float floatVar0;
    float floatVar1;
    float floatVar2;
    float floatVar3;
    i32 counterVar0;
    i32 counterVar1;

    float GetFloatVar(float varId);
    i32 GetIntVar(i32 varId);
    float *GetFloatVarPtr(float *varPtr, u16 varMask, u32 variableNumber);
    i32 *GetIntVarPtr(i32 *varPtr, u16 varMask, u32 variableNumber);
};

float AnmVm::GetFloatVar(float varId) {
    switch ((i32)varId) {
    case 0x2710:
        return this->intVar0;
    case 0x2711:
        return this->intVar1;
    case 0x2712:
        return this->intVar2;
    case 0x2713:
        return this->intVar3;
    case 0x2714:
        return this->floatVar0;
    case 0x2715:
        return this->floatVar1;
    case 0x2716:
        return this->floatVar2;
    case 0x2717:
        return this->floatVar3;
    case 0x2718:
        return this->counterVar0;
    case 0x2719:
        return this->counterVar1;
    default:
        return varId;
    }
}
i32 AnmVm::GetIntVar(i32 varId) {
    switch (varId) {
    case 0x2710:
        return this->intVar0;
    case 0x2711:
        return this->intVar1;
    case 0x2712:
        return this->intVar2;
    case 0x2713:
        return this->intVar3;
    case 0x2714:
        return (i32)this->floatVar0;
    case 0x2715:
        return (i32)this->floatVar1;
    case 0x2716:
        return (i32)this->floatVar2;
    case 0x2717:
        return (i32)this->floatVar3;
    case 0x2718:
        return this->counterVar0;
    case 0x2719:
        return this->counterVar1;
    default:
        return varId;
    }
}

float *AnmVm::GetFloatVarPtr(float *varPtr, u16 varMask, u32 variableNumber) {
    if ((varMask & (1 << variableNumber)) == 0) {
        return varPtr;
    }

    switch ((i32)*varPtr) {
    case 0x2714:
        return &this->floatVar0;
    case 0x2715:
        return &this->floatVar1;
    case 0x2716:
        return &this->floatVar2;
    case 0x2717:
        return &this->floatVar3;
    default:
        return varPtr;
    }
}

i32 *AnmVm::GetIntVarPtr(i32 *varPtr, u16 varMask, u32 variableNumber) {
    if ((varMask & (1 << variableNumber)) == 0) {
        return varPtr;
    }

    switch (*varPtr) {
    case 0x2710:
        return &this->intVar0;
    case 0x2711:
        return &this->intVar1;
    case 0x2712:
        return &this->intVar2;
    case 0x2713:
        return &this->intVar3;
    case 0x2718:
        return &this->counterVar0;
    case 0x2719:
        return &this->counterVar1;
    default:
        return varPtr;
    }
}

} // namespace th07
