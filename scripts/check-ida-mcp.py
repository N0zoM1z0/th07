#!/usr/bin/env python3
"""Verify the registered IDA MCP protocol path and exact TH07 target."""

from __future__ import annotations

import argparse
import asyncio
import json

from ida_mcp_client import (
    DEFAULT_SERVER,
    IdaMcpError,
    REQUIRED_READ_TOOLS,
    call_json,
    load_target,
    open_session,
    parse_int,
    require_target,
)


async def check(server_name: str) -> dict[str, object]:
    async with open_session(server_name) as (session, initialized):
        names = {tool.name for tool in (await session.list_tools()).tools}
        missing = sorted(set(REQUIRED_READ_TOOLS) - names)
        if missing:
            raise IdaMcpError("missing_tool", f"IDA MCP missing required tools: {missing}")
        connection = await call_json(session, "check_connection", {})
        if not isinstance(connection, str) or not connection.startswith("Successfully connected"):
            raise IdaMcpError("unavailable", f"IDA plugin is not connected: {connection!r}")
        metadata = await require_target(session)
        entry = await call_json(session, "get_entry_points", {})
        wanted_entry = parse_int(load_target()["pe"]["entry_point"])
        if not isinstance(entry, dict) or parse_int(entry.get("address")) != wanted_entry:
            raise IdaMcpError("target_mismatch", f"unexpected entry point: {entry!r}")
        first = await call_json(session, "get_function_by_address", {"address": "0x00401000"})
        return {
            "ok": True,
            "protocol_version": initialized.protocolVersion,
            "mcp_server": initialized.serverInfo.name,
            "tool_count": len(names),
            "metadata": metadata,
            "entry_point": entry,
            "text_sentinel": first,
            "boundary_rule": "config/functions.csv plus target disassembly remain authoritative",
        }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", default=DEFAULT_SERVER)
    args = parser.parse_args()
    try:
        output = asyncio.run(check(args.server))
    except (IdaMcpError, TypeError, ValueError) as exc:
        reason = exc.reason if isinstance(exc, IdaMcpError) else "target_mismatch"
        print(json.dumps({"ok": False, "reason": reason, "error": str(exc)}, indent=2))
        raise SystemExit(1) from exc
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()

