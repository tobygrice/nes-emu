#!/bin/bash
# Usage: ./cmpLogs.sh logs/expected.log logs/actual.log

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 expected.log actual.log"
  exit 1
fi

expected="$1"
actual="$2"

awk '
function norm(line) {
    # Remove the PPU section (e.g. "PPU:" followed by two numbers separated by a comma)
    gsub(/PPU:[[:space:]]*[0-9]+,[[:space:]]*[0-9]+/, "", line)
    return line
}
NR==FNR {
    a[NR] = norm($0)
    next
}
{
    if (norm($0) != a[FNR]) {
        print "Difference at line", FNR
        print "Actual:    " norm($0)
        print "Expected:  " a[FNR]
        exit
    }
}' "$expected" "$actual"
