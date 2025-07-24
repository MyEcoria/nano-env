#!/usr/bin/env bash

###################################################################################################

code_inspect()
{
    local SOURCE_ROOT_PATH=$1
    if [[ $SOURCE_ROOT_PATH == "" ]]; then
        echo "Missing the source code path" >&2
        return 1
    fi

    if [[ $(grep -rlP "^\s*assert \(" $SOURCE_ROOT_PATH/nano) ]]; then
        echo "Using assert is not permitted. Use debug_assert instead." >&2
        return 1
    fi

    return 0
}

###################################################################################################
