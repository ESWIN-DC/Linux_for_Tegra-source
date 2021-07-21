**********************************************************************
                         Linux for Tegra
                    Building ATF and Trusty
                             README
**********************************************************************
This README provides instructions for building and verifying the ARM
Trusted Firmware (ATF) and Trusty.

----------------------------------------------------------------------
Exporting the Toolchain Paths
----------------------------------------------------------------------
Follow the instructions in "The L4T Toolchain" topic in the L4T
documentation to obtain the toolchain, and set the CROSS_COMPILE
environment variables. Set environment variable CROSS_COMPILE_AARCH64
to point to the AArch64 toolchain, and environment variable
CROSS_COMPILE_ARM to point at the armhf toolchain.

----------------------------------------------------------------------
Extracting the ATF and Trusty Source Code
----------------------------------------------------------------------
To extract the ATF and Trusty source code:
1. Navigate to the directory where the source package is extracted.
2. Create a directory for ATF and Trusty as follows.
   mkdir atf_and_trusty
   mkdir atf_and_trusty/atf
   mkdir atf_and_trusty/trusty
3. Execute this command to unpack the ATF source package.
   tar xpf atf_src.tbz2 -C atf_and_trusty/atf/
4. Execute this command to unpack the Trusty package.
   tar xpf trusty_src.tbz2 -C atf_and_trusty/trusty/

----------------------------------------------------------------------
Building the ATF Source Code
----------------------------------------------------------------------
To build the ATF source code:
1. Navigate to the ATF directory.
   cd atf_and_trusty/atf/arm-trusted-firmware
2. Execute these commands to build the source code.
   - Jetson TX2:
     make BUILD_BASE=./t186ref \
         CROSS_COMPILE="${CROSS_COMPILE_AARCH64}" \
         DEBUG=0 LOG_LEVEL=20 PLAT=tegra SPD=trusty TARGET_SOC=t186 V=0
   - Jetson Xavier:
     make BUILD_BASE=./t194ref \
         CROSS_COMPILE="${CROSS_COMPILE_AARCH64}" \
         DEBUG=0 LOG_LEVEL=20 PLAT=tegra SPD=trusty TARGET_SOC=t194 V=0

Once the build has completed successfully, the ATF image is generated as:
- Jetson TX2:
  ./t186ref/tegra/t186/release/bl31.bin
- Jetson Xavier:
  ./t194ref/tegra/t194/release/bl31.bin

----------------------------------------------------------------------
Building the Trusty Source Code
----------------------------------------------------------------------
To build the Trusty source code:
1. Navigate to the Trusty directory.
   cd atf_and_trusty/trusty/trusty
2. Execute these commands to build the source code.
   - Jetson TX2:
     make t186 PROJECT=t186 TARGET=t186 BUILDROOT=./t186ref \
         TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
         ARCH_arm_TOOLCHAIN_PREFIX="${CROSS_COMPILE_ARM}" \
         ARCH_arm64_TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
         DEBUG=0 DEBUG_LVL=0 DEFAULT_OTE_APP_DEBUGLEVEL=1 NOECHO=@ \
         TRUSTY_VARIANT=l4t-public TRUSTY_MULTI_GUEST_CONFIGURATION= \
         TARGET_SOC=t186
   - Jetson Xavier:
     make t186 PROJECT=t186 TARGET=t186 BUILDROOT=./t194ref \
         TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
         ARCH_arm_TOOLCHAIN_PREFIX="${CROSS_COMPILE_ARM}" \
         ARCH_arm64_TOOLCHAIN_PREFIX="${CROSS_COMPILE_AARCH64}" \
         DEBUG=0 DEBUG_LVL=0 DEFAULT_OTE_APP_DEBUGLEVEL=1 NOECHO=@ \
         TRUSTY_VARIANT=l4t-public TRUSTY_MULTI_GUEST_CONFIGURATION= \
         TARGET_SOC=t194

Once the build is completed successfully, the Trusty image is generated as:
- Jetson TX2:
  ./t186ref/build-t186/lk.bin
- Jetson Xavier:
  ./t194ref/build-t186/lk.bin

----------------------------------------------------------------------
Generating the tos.img with ATF and Trusty Images
----------------------------------------------------------------------
To generate the tos.img with ATF and Trusty images:
1. Unpack the L4T tarball from the BSP package.
   tar xpf Jetson_Linux_R<VERSION>_aarch64.tbz2
2. Copy the ATF image bl31.bin and Trusty image lk.bin to this directory.
   <Linux_for_Tegra>/nv_tegra/tos-scripts
3. Generate the tos.img with the commands:
   cd <Linux_for_Tegra>/nv_tegra/tos-scripts/
   - Jetson TX1 and Jetson TX2:
   ./gen_tos_part_img.py --monitor bl31.bin --os lk.bin tos.img
   - Jetson Xavier:
   ./gen_tos_part_img.py --monitor bl31.bin --os lk.bin tos_t194.img

----------------------------------------------------------------------
Verifying the Image
----------------------------------------------------------------------
To verify the image:
1. Replace the default TOS image file with the newly generated TOS
   image. The default TOS image file is located at:
   - Jetson TX1 and Jetson TX2:
   <Linux_for_Tegra>/bootloader/tos.img
   - Jetson Xavier:
   <Linux_for_Tegra>/bootloader/tos_t194.img
2. Perform either of these tasks:
   - Flash the system as normal.
     This is useful for flashing a new system or replacing the
     entire Operating System.
   - Re-flash the TOS image using these partition flash commands.
     - Jetson TX2:
       sudo ./flash.sh -k secure-os jetson-tx2 mmcblk0p1
     - Jetson Xavier:
       sudo ./flash.sh -k secure-os jetson-xavier mmcblk0p1
