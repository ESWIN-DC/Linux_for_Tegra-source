#!/bin/bash

# This is default KEK2 root key for unfused board
echo "00000000000000000000000000000000" > kek2_key

# This is the default initial vector for EKB.
echo "bad66eb4484983684b992fe54a648bb8" > fv_ekb

# Generate user-defined symmetric key files
# openssl rand -rand /dev/urandom -hex 16 > sym.key
# openssl rand -rand /dev/urandom -hex 16 > sym2.key
echo "00000000000000000000000000000000" > sym.key
echo "00000000000000000000000000000000" > sym2.key

python3 gen_ekb.py -kek2_key kek2_key \
        -fv fv_ekb \
        -in_sym_key sym.key \
        -in_sym_key2 sym2.key \
        -out eks.img
