#!/bin/sh

# TODO: fix this
[ -n "$BIN_DIR" ] || BIN_DIR=".."
[ -n "$SCRIPT_DIR" ] || SCRIPT_DIR="."

OUT=$(mktemp -t bpseek.XXXXXX)
trap 'rm -f "$OUT"' INT EXIT

test_output() {
    NAME="$1"
    shift

    # workaround for macos crap
    echo "$NAME: \c"

    "$BIN_DIR/bpseek" --color=never "$@" "$SCRIPT_DIR/$NAME.data" > "$OUT"

    if ! cmp -s "$OUT" "$SCRIPT_DIR/$NAME.out"; then
        echo "FAIL"
        diff -u "$OUT" "$SCRIPT_DIR/$NAME.out"
        exit 1
    fi

    echo "OK"
}

test_output short-unaligned -m 4 -x 6 -s 1 -X 8
