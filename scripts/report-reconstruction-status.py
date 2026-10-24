#!/usr/bin/env python3
"""Report reviewed authored scope separately from linked runtime and SDK code."""
import argparse,json
from reconstruction_state import state
parser=argparse.ArgumentParser();parser.add_argument('--summary',action='store_true');parser.parse_args()
print(json.dumps(state(),indent=2))
