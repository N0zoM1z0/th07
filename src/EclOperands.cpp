#include "inttypes.hpp"

namespace th07
{
namespace EclOperands
{

// These overlays record only target-observed byte offsets.  They are private
// because neither the complete TH07 Enemy layout nor the target global names
// have been recovered in this lane.
struct EnemyOverlay
{
    u8 bytes[1];

    f32 ResolveFloat(f32 operand);
};

struct Vector3
{
    f32 x;
    f32 y;
    f32 z;

    f32 Length() const;
};

struct TargetPlayerOverlay
{
    f32 AngleToPlayer(const Vector3 *position);
};

struct TargetRngOverlay
{
    u32 RandomU32();
    f32 RandomF32();
};

static __forceinline i32 *IntField(EnemyOverlay *enemy, i32 offset)
{
    return (i32 *)(enemy->bytes + offset);
}

static __forceinline f32 *FloatField(EnemyOverlay *enemy, i32 offset)
{
    return (f32 *)(enemy->bytes + offset);
}

// Target globals are kept as unresolved externals until their owning lanes
// attest their names and relocation mappings.
extern i32 g_TargetInt626280;
extern i32 g_TargetInt62F8A4;
extern u8 g_TargetByte62F647;
extern i32 g_TargetInt1347AA0;
extern i32 g_TargetInt1347AA4;
extern i32 g_TargetInt1347AA8;
extern i32 g_TargetInt1347AAC;
extern f32 g_TargetFloat4BE408;
extern f32 g_TargetFloat4BE40C;
extern f32 g_TargetFloat4BE410;
extern f32 g_TargetFloat1347AB0;
extern f32 g_TargetFloat1347AB4;
extern f32 g_TargetFloat1347AB8;
extern f32 g_TargetFloat1347ABC;
extern TargetPlayerOverlay g_TargetPlayer4BDAD8;
extern TargetRngOverlay g_TargetRng49FE20;

// Observed: target 0x0040E5B0 receives Enemy in ECX and the raw i32 operand
// in EDX, then resolves the ECL variable IDs 0x2710..0x2759.  Meanings of
// individual offsets remain intentionally unnamed pending their owner lanes.
i32 __fastcall ResolveInt(EnemyOverlay *enemy, i32 operand)
{
    Vector3 delta;
    Vector3 *position;
    u32 range;

    switch (operand)
    {
    case 0x2710: return *IntField(enemy, 0x6FC);
    case 0x2711: return *IntField(enemy, 0x700);
    case 0x2712: return *IntField(enemy, 0x704);
    case 0x2713: return *IntField(enemy, 0x708);
    case 0x272D: return *IntField(enemy, 0x744);
    case 0x272E: return *IntField(enemy, 0x748);
    case 0x272F: return *IntField(enemy, 0x74C);
    case 0x2730: return *IntField(enemy, 0x750);
    case 0x271C: return *IntField(enemy, 0x72C);
    case 0x271D: return *IntField(enemy, 0x730);
    case 0x271E: return *IntField(enemy, 0x734);
    case 0x271F: return *IntField(enemy, 0x738);
    case 0x2720: return g_TargetInt626280;
    case 0x2721: return g_TargetInt62F8A4;
    case 0x2729: return *IntField(enemy, 0x2BCC);
    case 0x272B: return *IntField(enemy, 0x2BB8);
    case 0x272C: return g_TargetByte62F647;
    case 0x2758: return (i32)*FloatField(enemy, 0x73C);
    case 0x2759: return (i32)*FloatField(enemy, 0x740);
    case 0x2714: return (i32)*FloatField(enemy, 0x70C);
    case 0x2715: return (i32)*FloatField(enemy, 0x710);
    case 0x2716: return (i32)*FloatField(enemy, 0x714);
    case 0x2717: return (i32)*FloatField(enemy, 0x718);
    case 0x2718: return (i32)*FloatField(enemy, 0x71C);
    case 0x2719: return (i32)*FloatField(enemy, 0x720);
    case 0x271A: return (i32)*FloatField(enemy, 0x724);
    case 0x271B: return (i32)*FloatField(enemy, 0x728);
    case 0x2731: return (i32)*FloatField(enemy, 0x754);
    case 0x2732: return (i32)*FloatField(enemy, 0x758);
    case 0x2733: return (i32)*FloatField(enemy, 0x75C);
    case 0x2734: return (i32)*FloatField(enemy, 0x760);
    case 0x2735: return g_TargetInt1347AA0;
    case 0x2736: return g_TargetInt1347AA4;
    case 0x2737: return g_TargetInt1347AA8;
    case 0x2738: return g_TargetInt1347AAC;
    case 0x2739: return (i32)g_TargetFloat1347AB0;
    case 0x273A: return (i32)g_TargetFloat1347AB4;
    case 0x273B: return (i32)g_TargetFloat1347AB8;
    case 0x273C: return (i32)g_TargetFloat1347ABC;
    case 0x2722: return (i32)*FloatField(enemy, 0x2B0C);
    case 0x2723: return (i32)*FloatField(enemy, 0x2B10);
    case 0x2724: return (i32)*FloatField(enemy, 0x2B14);
    case 0x2725: return (i32)g_TargetFloat4BE408;
    case 0x2726: return (i32)g_TargetFloat4BE40C;
    case 0x2727: return (i32)g_TargetFloat4BE410;
    case 0x2742: return (i32)*FloatField(enemy, 0x2B8C);
    case 0x2743: return (i32)*FloatField(enemy, 0x2B90);
    case 0x2744: return (i32)*FloatField(enemy, 0x2B94);
    case 0x274F: return (i32)*FloatField(enemy, 0x2B30);
    case 0x2750: return (i32)*FloatField(enemy, 0x2B34);
    case 0x2751: return (i32)*FloatField(enemy, 0x2B38);
    case 0x2752: return *IntField(enemy, 0x2EBC);
    case 0x2753: return *IntField(enemy, 0x2EC0);
    case 0x2754: return *IntField(enemy, 0x2EC4);
    case 0x2755: return *IntField(enemy, 0x2EC8);
    case 0x273D: return (i32)*FloatField(enemy, 0x2B54);
    case 0x273E: return (i32)*FloatField(enemy, 0x2B58);
    case 0x273F: return (i32)*FloatField(enemy, 0x2B64);
    case 0x2740: return (i32)*FloatField(enemy, 0x2B68);
    case 0x2741: return (i32)*FloatField(enemy, 0x2B6C);
    case 0x2745: return (i32)*FloatField(enemy, 0x2B5C);
    case 0x2746: return (i32)*FloatField(enemy, 0x2B60);
    case 0x2747: return g_TargetRng49FE20.RandomU32();
    case 0x2748:
        range = *IntField(enemy, 0x744);
        return *IntField(enemy, 0x748) + (range ? g_TargetRng49FE20.RandomU32() % range : 0);
    case 0x274D: return *IntField(enemy, 0x2E4C);
    case 0x274E: return *(u8 *)(enemy->bytes + 0x2E17);
    case 0x2756: return *IntField(enemy, 0x2E10);
    case 0x2757: return *IntField(enemy, 0x2BC0);
    case 0x2728: return (i32)g_TargetPlayer4BDAD8.AngleToPlayer((Vector3 *)(enemy->bytes + 0x2B0C));
    case 0x272A:
        position = (Vector3 *)(enemy->bytes + 0x2B0C);
        delta.z = g_TargetFloat4BE410 - position->z;
        delta.y = g_TargetFloat4BE40C - position->y;
        delta.x = g_TargetFloat4BE408 - position->x;
        return (i32)delta.Length();
    default: return operand;
    }
}

// Observed: target 0x0040EC00 first returns the raw operand pointer when a
// caller-provided bit is clear; otherwise it resolves writable integer IDs.
i32 *__fastcall ResolveIntLValue(EnemyOverlay *enemy, i32 *operand, u16 flags, i32 flagIndex)
{
    if (flagIndex >= 0 && !(flags & (1 << flagIndex)))
        return operand;

    switch (*operand)
    {
    case 0x2710: return IntField(enemy, 0x6FC);
    case 0x2711: return IntField(enemy, 0x700);
    case 0x2712: return IntField(enemy, 0x704);
    case 0x2713: return IntField(enemy, 0x708);
    case 0x272D: return IntField(enemy, 0x744);
    case 0x272E: return IntField(enemy, 0x748);
    case 0x272F: return IntField(enemy, 0x74C);
    case 0x2730: return IntField(enemy, 0x750);
    case 0x271C: return IntField(enemy, 0x72C);
    case 0x271D: return IntField(enemy, 0x730);
    case 0x271E: return IntField(enemy, 0x734);
    case 0x271F: return IntField(enemy, 0x738);
    case 0x2720: return &g_TargetInt626280;
    case 0x2721: return &g_TargetInt62F8A4;
    case 0x2729: return IntField(enemy, 0x2BCC);
    case 0x272B: return IntField(enemy, 0x2BB8);
    case 0x2756: return IntField(enemy, 0x2E10);
    case 0x2757: return IntField(enemy, 0x2BC0);
    case 0x2735: return &g_TargetInt1347AA0;
    case 0x2736: return &g_TargetInt1347AA4;
    case 0x2737: return &g_TargetInt1347AA8;
    case 0x2738: return &g_TargetInt1347AAC;
    default: return operand;
    }
}

// Observed: target 0x0040EDF0 receives Enemy in ECX and a raw float operand
// on the stack.  TH06 supports the interpretation as the float rvalue path.
f32 EnemyOverlay::ResolveFloat(f32 operand)
{
    Vector3 delta;
    Vector3 *position;

    switch ((i32)operand)
    {
    case 0x2710: return (f32)*(i32 *)(bytes + 0x6FC);
    case 0x2711: return (f32)*(i32 *)(bytes + 0x700);
    case 0x2712: return (f32)*(i32 *)(bytes + 0x704);
    case 0x2713: return (f32)*(i32 *)(bytes + 0x708);
    case 0x272D: return (f32)*(i32 *)(bytes + 0x744);
    case 0x272E: return (f32)*(i32 *)(bytes + 0x748);
    case 0x272F: return (f32)*(i32 *)(bytes + 0x74C);
    case 0x2730: return (f32)*(i32 *)(bytes + 0x750);
    case 0x271C: return (f32)*(i32 *)(bytes + 0x72C);
    case 0x271D: return (f32)*(i32 *)(bytes + 0x730);
    case 0x271E: return (f32)*(i32 *)(bytes + 0x734);
    case 0x271F: return (f32)*(i32 *)(bytes + 0x738);
    case 0x2720: return (f32)g_TargetInt626280;
    case 0x2721: return (f32)g_TargetInt62F8A4;
    case 0x2729: return (f32)*(i32 *)(bytes + 0x2BCC);
    case 0x272B: return (f32)*(i32 *)(bytes + 0x2BB8);
    case 0x272C: return (f32)g_TargetByte62F647;
    case 0x2756: return (f32)*(i32 *)(bytes + 0x2E10);
    case 0x2757: return (f32)*(i32 *)(bytes + 0x2BC0);
    case 0x2735: return (f32)g_TargetInt1347AA0;
    case 0x2736: return (f32)g_TargetInt1347AA4;
    case 0x2737: return (f32)g_TargetInt1347AA8;
    case 0x2738: return (f32)g_TargetInt1347AAC;
    case 0x2739: return g_TargetFloat1347AB0;
    case 0x273A: return g_TargetFloat1347AB4;
    case 0x273B: return g_TargetFloat1347AB8;
    case 0x273C: return g_TargetFloat1347ABC;
    case 0x2714: return *FloatField(this, 0x70C);
    case 0x2715: return *FloatField(this, 0x710);
    case 0x2716: return *FloatField(this, 0x714);
    case 0x2717: return *FloatField(this, 0x718);
    case 0x2718: return *FloatField(this, 0x71C);
    case 0x2719: return *FloatField(this, 0x720);
    case 0x271A: return *FloatField(this, 0x724);
    case 0x271B: return *FloatField(this, 0x728);
    case 0x2731: return *FloatField(this, 0x754);
    case 0x2732: return *FloatField(this, 0x758);
    case 0x2733: return *FloatField(this, 0x75C);
    case 0x2734: return *FloatField(this, 0x760);
    case 0x2722: return *FloatField(this, 0x2B0C);
    case 0x2723: return *FloatField(this, 0x2B10);
    case 0x2724: return *FloatField(this, 0x2B14);
    case 0x2725: return g_TargetFloat4BE408;
    case 0x2726: return g_TargetFloat4BE40C;
    case 0x2727: return g_TargetFloat4BE410;
    case 0x2758: return *FloatField(this, 0x73C);
    case 0x2759: return *FloatField(this, 0x740);
    case 0x2742: return *FloatField(this, 0x2B8C);
    case 0x2743: return *FloatField(this, 0x2B90);
    case 0x2744: return *FloatField(this, 0x2B94);
    case 0x2749: return *FloatField(this, 0x2B80);
    case 0x274A: return *FloatField(this, 0x2B84);
    case 0x274B: return *FloatField(this, 0x2B88);
    case 0x274F: return *FloatField(this, 0x2B30);
    case 0x2750: return *FloatField(this, 0x2B34);
    case 0x2751: return *FloatField(this, 0x2B38);
    case 0x2752: return (f32)*(i32 *)(bytes + 0x2EBC);
    case 0x2753: return (f32)*(i32 *)(bytes + 0x2EC0);
    case 0x2754: return (f32)*(i32 *)(bytes + 0x2EC4);
    case 0x2755: return (f32)*(i32 *)(bytes + 0x2EC8);
    case 0x2728: return g_TargetPlayer4BDAD8.AngleToPlayer((Vector3 *)(bytes + 0x2B0C));
    case 0x273D: return *FloatField(this, 0x2B54);
    case 0x273E: return *FloatField(this, 0x2B58);
    case 0x273F: return *FloatField(this, 0x2B64);
    case 0x2740: return *FloatField(this, 0x2B68);
    case 0x2741: return *FloatField(this, 0x2B6C);
    case 0x2745: return *FloatField(this, 0x2B5C);
    case 0x2746: return *FloatField(this, 0x2B60);
    case 0x2747: return g_TargetRng49FE20.RandomF32();
    case 0x2748: return g_TargetRng49FE20.RandomF32() * *FloatField(this, 0x754) + *FloatField(this, 0x758);
    case 0x274C: return g_TargetRng49FE20.RandomF32() * 6.2831855f - 3.1415927f;
    case 0x274E: return (f32)*(u8 *)(bytes + 0x2E17);
    case 0x274D: return (f32)*(i32 *)(bytes + 0x2E4C);
    case 0x272A:
        position = (Vector3 *)(bytes + 0x2B0C);
        delta.z = g_TargetFloat4BE410 - position->z;
        delta.y = g_TargetFloat4BE40C - position->y;
        delta.x = g_TargetFloat4BE408 - position->x;
        return delta.Length();
    default: return operand;
    }
}

// Observed: target 0x0040F3C0 is the analogous writable float path.  It
// shares the immediate-pointer guard with ResolveIntLValue.
f32 *__fastcall ResolveFloatLValue(EnemyOverlay *enemy, f32 *operand, u16 flags, i32 flagIndex)
{
    if (flagIndex >= 0 && !(flags & (1 << flagIndex)))
        return operand;

    switch ((i32)*operand)
    {
    case 0x2714: return FloatField(enemy, 0x70C);
    case 0x2715: return FloatField(enemy, 0x710);
    case 0x2716: return FloatField(enemy, 0x714);
    case 0x2717: return FloatField(enemy, 0x718);
    case 0x2718: return FloatField(enemy, 0x71C);
    case 0x2719: return FloatField(enemy, 0x720);
    case 0x271A: return FloatField(enemy, 0x724);
    case 0x271B: return FloatField(enemy, 0x728);
    case 0x2731: return FloatField(enemy, 0x754);
    case 0x2732: return FloatField(enemy, 0x758);
    case 0x2733: return FloatField(enemy, 0x75C);
    case 0x2734: return FloatField(enemy, 0x760);
    case 0x2722: return FloatField(enemy, 0x2B0C);
    case 0x2723: return FloatField(enemy, 0x2B10);
    case 0x2724: return FloatField(enemy, 0x2B14);
    case 0x2725: return &g_TargetFloat4BE408;
    case 0x2726: return &g_TargetFloat4BE40C;
    case 0x2727: return &g_TargetFloat4BE410;
    case 0x2758: return FloatField(enemy, 0x73C);
    case 0x2759: return FloatField(enemy, 0x740);
    case 0x2739: return &g_TargetFloat1347AB0;
    case 0x273A: return &g_TargetFloat1347AB4;
    case 0x273B: return &g_TargetFloat1347AB8;
    case 0x273C: return &g_TargetFloat1347ABC;
    case 0x2742: return FloatField(enemy, 0x2B8C);
    case 0x2743: return FloatField(enemy, 0x2B90);
    case 0x2744: return FloatField(enemy, 0x2B94);
    case 0x2749: return FloatField(enemy, 0x2B80);
    case 0x274A: return FloatField(enemy, 0x2B84);
    case 0x274B: return FloatField(enemy, 0x2B88);
    case 0x273D: return FloatField(enemy, 0x2B54);
    case 0x273E: return FloatField(enemy, 0x2B58);
    case 0x273F: return FloatField(enemy, 0x2B64);
    case 0x2740: return FloatField(enemy, 0x2B68);
    case 0x2741: return FloatField(enemy, 0x2B6C);
    case 0x2745: return FloatField(enemy, 0x2B5C);
    case 0x2746: return FloatField(enemy, 0x2B60);
    default: return operand;
    }
}

} // namespace EclOperands
} // namespace th07
