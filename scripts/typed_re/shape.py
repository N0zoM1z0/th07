"""Relocation-insensitive x86 instruction-shape diagnostics.

The strict comparator remains the acceptance authority.  This module is a
compiler-shaping aid: it separates address/constant differences from changes
to registers, operand widths, memory topology, and instruction order.
"""

from __future__ import annotations

from collections import Counter
from difflib import SequenceMatcher
from typing import Any

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_EBP


def _decoder() -> Cs:
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    return decoder


def trim_coff_alignment_padding(code: bytes, address: int) -> tuple[bytes, int]:
    """Remove only decoded post-RET COFF alignment bytes.

    VC7 COMDAT text sections commonly retain zero, NOP, or INT3 alignment after
    the function's final return.  The strict comparator must keep reporting the
    complete section tail, but topology diagnostics need the actual emitted
    function body when it is longer than the target extent.  Fail closed unless
    a linearly decoded RET is followed exclusively by conventional padding.
    """
    instructions = list(_decoder().disasm(code, address))
    for instruction in reversed(instructions):
        if not instruction.mnemonic.startswith("ret"):
            continue
        end = instruction.address - address + instruction.size
        padding = code[end:]
        if padding and all(byte in {0x00, 0x90, 0xCC} for byte in padding):
            return code[:end], len(padding)
        if not padding:
            return code, 0
    return code, 0


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


def _internal_branch_edges(instructions: list[Any]) -> dict[int, int]:
    """Map direct jump instruction indices to destination instruction indices."""
    address_to_index = {
        instruction.address: index for index, instruction in enumerate(instructions)
    }
    edges: dict[int, int] = {}
    for index, instruction in enumerate(instructions):
        if not instruction.mnemonic.startswith("j") or not instruction.operands:
            continue
        operand = instruction.operands[0]
        if operand.type != X86_OP_IMM:
            continue
        destination = int(operand.imm) & 0xFFFFFFFF
        if destination in address_to_index:
            edges[index] = address_to_index[destination]
    return edges


def _resynchronized_shape_blocks(
    target_instructions: list[Any], object_instructions: list[Any], address: int
) -> tuple[list[dict[str, Any]], int, bool]:
    """Find ordered exact-shape islands after local insertions/deletions.

    A single spill or rematerialized temporary can end the strict shared
    prefix even when later source phases return to the target topology.  The
    blocks are diagnostic only: instruction shapes abstract constants and
    displacements, and short repeated x86 sequences are not strong evidence.
    Requiring at least three instructions suppresses the noisiest one-opcode
    coincidences while retaining useful basic-block-sized anchors.
    """
    target_shapes = [instruction_shape(item) for item in target_instructions]
    object_shapes = [instruction_shape(item) for item in object_instructions]
    # Giant dispatchers contain thousands of repeated `mov`/`test` shapes;
    # treating every occurrence as an anchor can become quadratic and consume
    # a full core for little gain. Preserve the more precise alignment for
    # ordinary large callbacks and enable difflib's popularity filter above a
    # conservative 2500-instruction bound.
    bounded_autojunk = max(len(target_shapes), len(object_shapes)) > 2500
    matcher = SequenceMatcher(
        None, target_shapes, object_shapes, autojunk=bounded_autojunk
    )
    retained = [block for block in matcher.get_matching_blocks() if block.size >= 3]
    records = []
    for block in retained[:64]:
        records.append(
            {
                "target_instruction_index": block.a,
                "object_instruction_index": block.b,
                "instruction_count": block.size,
                "target_start": _instruction_record(
                    target_instructions[block.a], address
                ),
                "object_start": _instruction_record(
                    object_instructions[block.b], address
                ),
                "target_end": _instruction_record(
                    target_instructions[block.a + block.size - 1], address
                ),
                "object_end": _instruction_record(
                    object_instructions[block.b + block.size - 1], address
                ),
            }
        )
    return records, sum(block.size for block in retained), bounded_autojunk


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

    target_edges = _internal_branch_edges(target_instructions)
    object_edges = _internal_branch_edges(object_instructions)
    (
        resynchronized_blocks,
        resynchronized_count,
        resynchronized_popularity_filter,
    ) = _resynchronized_shape_blocks(target_instructions, object_instructions, address)
    branch_mismatches = []
    for index in sorted(set(target_edges) | set(object_edges)):
        target_destination = target_edges.get(index)
        object_destination = object_edges.get(index)
        if target_destination == object_destination:
            continue
        branch_mismatches.append(
            {
                "instruction_index": index,
                "target_destination_index": target_destination,
                "object_destination_index": object_destination,
                "target": (
                    _instruction_record(target_instructions[index], address)
                    if index < len(target_instructions)
                    else None
                ),
                "object": (
                    _instruction_record(object_instructions[index], address)
                    if index < len(object_instructions)
                    else None
                ),
            }
        )

    return {
        "target_instruction_count": len(target_instructions),
        "object_instruction_count": len(object_instructions),
        "target_decoded_bytes": target_decoded,
        "object_decoded_bytes": object_decoded,
        "shared_shape_prefix": prefix,
        "topology_exact": topology_exact,
        "first_shape_mismatch": first_mismatch,
        "resynchronized_shape_instruction_count": resynchronized_count,
        "resynchronized_target_coverage_percent": round(
            100.0 * resynchronized_count / len(target_instructions), 2
        ) if target_instructions else 100.0,
        "resynchronized_popularity_filter": resynchronized_popularity_filter,
        "resynchronized_shape_blocks": resynchronized_blocks,
        "target_internal_branch_count": len(target_edges),
        "object_internal_branch_count": len(object_edges),
        "branch_target_mismatch_count": len(branch_mismatches),
        "first_branch_target_mismatch": branch_mismatches[0] if branch_mismatches else None,
        "branch_target_mismatches": branch_mismatches[:32],
        "control_flow_exact": topology_exact and not branch_mismatches,
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
