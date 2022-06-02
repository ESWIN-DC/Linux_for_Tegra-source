<!--- Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved. --->

@page tegra_se_sample AES-CMAC Hardware Key Definition Sample Application
@{

 - [Overview](#overview)
 - [Flow](#flow)
 - [Building and Running](#build_and_run)


- - - - - - - - - - - - - - -
<a name="overview">
## Overview ##

This sample demonstrates how to use the AES-CMAC KDF function
in the hardware SE (Security Engine) to implement
the counter-mode NIST-SP 800-108 key definition function (KDF).
The KDF is described in section 5.1, "KDF in Counter Mode," of
NIST Special Publication 800-108, [Recommendation for Key Derivation Using Pseudorandom Functions](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-108.pdf).
The internal pseudo-random function (PRF) of the KDF is AES-CMAC.

<a name="build_and_run">
- - - - - - - - - - - - - - -
The sample is integrated into the `hwkey-agent` TA of Trusty,
so there is no separate procedure to build and run it.

The source code for the sample is in the repo
3rdparty/trusty/app/nvidia-sample, at
./hwkey-agent/platform/tegra_se/tegra_se_aes.c.

<a name="flow">
- - - - - - - - - - - - - - -
## Flow
This pseudocode describes how the samples generates the derived key in counter mode.

    NIST-SP-800-108(KI, KO, L, context, label) {
      uint8_t count = 0x01;
      for (count=0x01; count<=L/128; count++) {
        AES-CMAC(key=KI, count || label_string || 0x00 || context_string || L, output=&KO[count*128]);
      }
    }

Where:

- `KI` is the input key, which must be loaded into an SE keyslot.
- `KO` is the output key.
- `L` is the bit length of the output key.
- `context` is a string that identifies the purpose for which the derived key
  is generated.
- `label` is a string containing information related to
  the derived keying material.

@}

