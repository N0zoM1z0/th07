"""Relocation-insensitive x86 instruction-shape diagnostics.

The strict comparator remains the acceptance authority.  This module is a
compiler-shaping aid: it separates address/constant differences from changes
to registers, operand widths, memory topology, and instruction order.
"""

from __future__ import annotations

from collections import Counter
from typing import Any

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_EBP


def _decoder() -> Cs:
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    return decoder


def _operand_shape(instruction: Any, operand: Any) -> tuple[Any, ...]:
    if operand.type == X86_OP_REG:
        return ("reg", instruction.reg_name(operand.reg), operand.size)
    if operand.type == X86_OP_IMM:
        return ("imm", operand.size)
    if operand.type == X86_OP_MEM:
        memory = operand.mem
        base = instruction.reg_name(memory.base) if memory.base else None
        index = instruction.reg_name(memory.index) if memory.index else None
        segment = instruction.reg_name(memory.segment) if memory.segment else None
        if base == "ebp":
            displacement_kind = "frame"
        elif base == "esp":
            displacement_kind = "stack"
        elif base is None and index is None:
            displacement_kind = "absolute"
        else:
            displacement_kind = "field"
        return (
            "mem",
            operand.size,
            segment,
            base,
            index,
            memory.scale,
            displacement_kind,
        )
    return ("other", operand.type, operand.size)


def instruction_shape(instruction: Any) -> tuple[Any, ...]:
    """Return topology while deliberately abstracting immediates/displacements."""
    return (
        instruction.mnemonic,
        tuple(_operand_shape(instruction, operand) for operand in instruction.operands),
    )


def _instruction_record(instruction: Any, base: int) -> dict[str, Any]:
    return {
        "offset": instruction.address - base,
        "address": f"0x{instruction.address:08X}",
        "size": instruction.size,
        "text": f"{instruction.mnemonic} {instruction.op_str}".rstrip(),
        "bytes": bytes(instruction.bytes).hex(" "),
    }


def compare_instruction_shapes(
    target: bytes, object_code: bytes, address: int
) -> dict[str, Any]:
    """Compare two function bodies by compiler-relevant instruction topology."""
    target_instructions = list(_decoder().disasm(target, address))
    object_instructions = list(_decoder().disasm(object_code, address))
    target_decoded = sum(item.size for item in target_instructions)
    object_decoded = sum(item.size for item in object_instructions)

    shared = min(len(target_instructions), len(object_instructions))
    prefix = 0
    size_differences: list[dict[str, int]] = []
    stack_pairs: Counter[tuple[int, int]] = Counter()
    for index in range(shared):
        target_instruction = target_instructions[index]
        object_instruction = object_instructions[index]
        if instruction_shape(target_instruction) != instruction_shape(object_instruction):
            break
        prefix += 1
        if target_instruction.size != object_instruction.size and len(size_differences) < 32:
            size_differences.append(
                {
                    "instruction_index": index,
                    "target_size": target_instruction.size,
                    "object_size": object_instruction.size,
                }
            )
        for target_operand, object_operand in zip(
            target_instruction.operands, object_instruction.operands
        ):
            if (
                target_operand.type == X86_OP_MEM
                and object_operand.type == X86_OP_MEM
                and target_operand.mem.base == X86_REG_EBP
                and object_operand.mem.base == X86_REG_EBP
            ):
                stack_pairs[
                    (int(target_operand.mem.disp), int(object_operand.mem.disp))
                ] += 1

    topology_exact = (
        prefix == len(target_instructions) == len(object_instructions)
        and target_decoded == len(target)
        and object_decoded == len(object_code)
    )
    first_mismatch = None
    if not topology_exact:
        first_mismatch = {
            "instruction_index": prefix,
            "target": (
                _instruction_record(target_instructions[prefix], address)
                if prefix < len(target_instructions)
                else None
            ),
            "object": (
                _instruction_record(object_instructions[prefix], address)
                if prefix < len(object_instructions)
                else None
            ),
        }

    return {
        "target_instruction_count": len(target_instructions),
        "object_instruction_count": len(object_instructions),
        "target_decoded_bytes": target_decoded,
        "object_decoded_bytes": object_decoded,
        "shared_shape_prefix": prefix,
        "topology_exact": topology_exact,
        "first_shape_mismatch": first_mismatch,
        "instruction_size_differences": size_differences,
        "stack_slot_pairs": [
            {
                "target_offset": target_offset,
                "object_offset": object_offset,
                "uses": count,
            }
            for (target_offset, object_offset), count in sorted(
                stack_pairs.items(), key=lambda item: (-item[1], item[0])
            )[:64]
        ],
    }

