# IDA-first analysis

The repository uses the registered `ida-pro-mcp` stdio bridge. The bridge is
preferred over directly reaching the plugin's loopback HTTP port because the
server runs in the Windows host environment.

```bash
codex mcp get ida-pro-mcp --json
python3 scripts/check-ida-mcp.py
```

The preflight performs MCP initialization, tool discovery, a live connection
check, and exact SHA-256, MD5, image-base, file-size, and entry-point checks.
It fails closed when another file is open in IDA.

For a focused read-only query:

```bash
python3 scripts/ida-mcp-call.py \
  --call get_function_by_address '{"address":"0x00401000"}' \
  --call decompile_function '{"address":"0x00401000"}'
```

Write-like tools are refused unless `--allow-write` is explicitly supplied,
and target byte patching is always refused. IDA function chunks are semantic
views rather than accepted comparison boundaries; reconcile them with exact
instructions before implementation.

